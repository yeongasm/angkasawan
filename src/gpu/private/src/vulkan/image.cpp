#include "vulkan/vkgpu.hpp"

namespace gpu
{
auto Image::info() const -> ImageInfo const&
{
	/**
	* Leaving out the non null check is intentional for all types of resources.
	* Resources shouldn't be used once they are freed and preventing a crash by having the non null check would make this error less prevalent.
	*/
	return __self().info;
}

auto Image::valid() const -> bool
{	
	return m_device && m_data && __self().handle != VK_NULL_HANDLE;
}

auto Image::is_swapchain_image() const -> bool
{
	/**
	* Swapchain images will not contain an allocation handle because they are retrieved from the driver instead.
	*/
	return !__self().memoryBlock.valid();
}

auto Image::is_transient() const -> bool
{
	return m_device && __self().memoryBlock.valid() && __self().memoryBlock.aliased();
}

auto Image::id() const -> resource_id_t
{
	return __self().id;
}

auto Image::memory_requirement(Device& device, ImageInfo const& info) -> MemoryRequirementInfo
{
	auto&& vkdevice = static_cast<DeviceImpl&>(device);

	VkImageCreateInfo imgCreateInfo = get_image_create_info(info);

	VkDeviceImageMemoryRequirements imageMemReq{
		.sType = VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS,
		.pCreateInfo = &imgCreateInfo
	};

	VkMemoryRequirements2 memReq{
		.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2
	};

	vkGetDeviceImageMemoryRequirements(vkdevice.device, &imageMemReq, &memReq);

	return MemoryRequirementInfo{
		.size = memReq.memoryRequirements.size,
		.alignment = memReq.memoryRequirements.alignment,
		.memoryTypeBits = memReq.memoryRequirements.memoryTypeBits
	};
}

auto Image::from(Device& device, ImageInfo&& info, MemoryBlock memoryBlock) -> Image
{
	auto&& vkdevice = static_cast<DeviceImpl&>(device);

	uint32 queueFamilyIndices[] = {
		vkdevice.mainQueue.familyIndex,
		vkdevice.computeQueue.familyIndex,
		vkdevice.transferQueue.familyIndex
	};

	VkImageCreateInfo imgInfo = get_image_create_info(info);

	if ((imgInfo.sharingMode & VK_SHARING_MODE_CONCURRENT) != 0)
	{
		imgInfo.queueFamilyIndexCount = 3u;
		imgInfo.pQueueFamilyIndices = queueFamilyIndices;
	}

	VkImage handle = VK_NULL_HANDLE;

	if (memoryBlock.valid())
	{
		auto const& memBlockImpl = impl_of(memoryBlock);
		MemoryRequirementInfo const memReq = Image::memory_requirement(device, info);

		if ((memReq.memoryTypeBits & memBlockImpl.allocationInfo.memoryType) != memReq.memoryTypeBits ||
			memReq.size > memBlockImpl.allocationInfo.size)
		{
			return {};
		}

		CHECK_OP(vkCreateImage(vkdevice.device, &imgInfo, nullptr, &handle))

		vmaBindImageMemory(vkdevice.allocator, memBlockImpl.handle, handle);
	}
	else
	{
		auto allocationFlags = translate_memory_usage(info.memoryUsage);

		if ((allocationFlags & VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT) != 0 ||
			(allocationFlags & VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT) != 0)
		{
			allocationFlags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
		}

		VmaAllocationCreateInfo allocInfo{
			.flags = allocationFlags,
			.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
		};

		VmaAllocation allocation = VK_NULL_HANDLE;
		VmaAllocationInfo allocationInfo = {};

		CHECK_OP(vmaCreateImage(vkdevice.allocator, &imgInfo, &allocInfo, &handle, &allocation, &allocationInfo))

		auto&& vkmemoryblock = *vkdevice.gpuResourcePool.stores.memoryBlocks.emplace();

		vkmemoryblock.handle = allocation;
		vkmemoryblock.allocationInfo = std::move(allocationInfo);

		new (&memoryBlock) MemoryBlock{ &vkmemoryblock, &vkdevice };
	}

	auto&& vkimage = *vkdevice.gpuResourcePool.stores.images.emplace();

	reflect::_ResourceMeta meta{ 
		.id 	= vkdevice.gpuResourcePool.idCounter.images++ % vkdevice.config().maxImages,
		.type 	= reflect::type_id_v<ImageImpl>, 
	};

	vkimage.handle = handle;
	vkimage.memoryBlock = memoryBlock;
	vkimage.info = std::move(info);
	vkimage.id = std::bit_cast<uint64>(meta);

	VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

	if ((imgInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0)
	{
		switch (imgInfo.format)
		{
		case VK_FORMAT_S8_UINT:
			aspectFlags = VK_IMAGE_ASPECT_STENCIL_BIT;
			break;
		case VK_FORMAT_D16_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
			aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			break;
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_D32_SFLOAT:
		default:
			aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
			break;
		}
	}

	VkImageViewCreateInfo imgViewInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = vkimage.handle,
		.viewType = translate_image_view_type(vkimage.info.type),
		.format = imgInfo.format,
		.subresourceRange = {
			.aspectMask = aspectFlags,
			.baseMipLevel = 0,
			.levelCount = imgInfo.mipLevels,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};

	vkCreateImageView(vkdevice.device, &imgViewInfo, nullptr, &vkimage.imageView);

	vkdevice.bind(vkimage, meta.id);

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vkimage);
	}

	return Image{ &vkimage, &vkdevice, };
}

auto Image::from(Device& device, SwapchainImpl& swapchain) -> lib::array<Image>
{
	auto&& vkdevice = static_cast<DeviceImpl&>(device);

	auto&& swapchainInfo = swapchain.info;

	// Ideally should be const but vkGetSwapchainImagesKHR does not accept a const uint32* on pSwapchainImageCount.
	uint32 maxImageCount = (swapchainInfo.imageCount > MAX_FRAMES_IN_FLIGHT) ? MAX_FRAMES_IN_FLIGHT : swapchainInfo.imageCount;

	lib::array<Image> images{ static_cast<size_t>(maxImageCount) };
	std::array<VkImage, MAX_FRAMES_IN_FLIGHT> vkImageHandles = {};
	std::array<VkImageView, MAX_FRAMES_IN_FLIGHT> vkImageViewHandles = {};

	vkGetSwapchainImagesKHR(vkdevice.device, swapchain.handle, &maxImageCount, vkImageHandles.data());

	VkImageViewCreateInfo imageViewInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = swapchain.surfaceColorFormat.format,
		.components = {
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
	};

	for (uint32 i = 0; i < maxImageCount; ++i)
	{
		imageViewInfo.image = vkImageHandles[i];
		vkCreateImageView(vkdevice.device, &imageViewInfo, nullptr, &vkImageViewHandles[i]);
	}

	for (uint32 i = 0; i < maxImageCount; ++i)
	{
		ImageInfo imageInfo{
			.type = ImageType::Image_2D,
			.format = static_cast<Format>(swapchain.surfaceColorFormat.format),
			.samples = SampleCount::Sample_Count_1,
			.tiling = ImageTiling::Optimal,
			.imageUsage = swapchainInfo.imageUsage,
			.memoryUsage = MemoryUsage::Dedicated,
			.dimension = {
				.width = swapchainInfo.dimension.width,
				.height = swapchainInfo.dimension.height,
				.depth = 1u
			},
			.clearValue = {
				.color = {
					.f32 = { 0.f, 0.f, 0.f, 1.f }
				}
			},
			.mipLevel = 0
		};

		auto&& vkimage = *vkdevice.gpuResourcePool.stores.images.emplace();

		reflect::_ResourceMeta meta{ 
			.id 	= vkdevice.gpuResourcePool.idCounter.images++ % vkdevice.config().maxImages,
			.type 	= reflect::type_id_v<ImageImpl>
		};

		vkimage.handle = vkImageHandles[i];
		vkimage.imageView = vkImageViewHandles[i];
		vkimage.info = std::move(imageInfo);
		vkimage.id = std::bit_cast<uint64>(meta);

		if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
		{
			vkdevice.setup_debug_name(vkimage);
		}

		images.emplace_back(&vkimage, &vkdevice);
	}

	return images;
}

auto Image::zombify(Device& dvc, ref_counted_base& resource) -> void
{
	DeviceImpl& device = static_cast<DeviceImpl&>(dvc);
	ImageImpl& image = static_cast<ImageImpl&>(resource);

	bool imageIsTransient = image.memoryBlock.valid() && image.memoryBlock.aliased();
	/*
	* Release ownership of it's memory allocation.
	*/
	image.memoryBlock.destroy();

	std::lock_guard const lock{ device.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = device.cpu_timeline();

	device.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&image, imageIsTransient](DeviceImpl& device) -> void
		{
			// If it is not transient.
			if (!imageIsTransient)
			{
				auto&& block = shared_base::impl_of(image.memoryBlock);
				
				vkDestroyImageView(device.device, image.imageView, nullptr);

				// If it is not a swapchain image.
				if (image.memoryBlock.valid())
				{
					vmaDestroyImage(device.allocator, image.handle, block.handle);

					auto it = device.gpuResourcePool.stores.memoryBlocks.get_iterator(&block);
					device.gpuResourcePool.stores.memoryBlocks.erase(it);
				}
			}
			else
			{
				vkDestroyImageView(device.device, image.imageView, nullptr);
				vkDestroyImage(device.device, image.handle, nullptr);
			}

			auto it = device.gpuResourcePool.stores.images.get_iterator(&image);
			device.gpuResourcePool.stores.images.erase(it);
		}
	);
}

auto format_texel_info(Format format) -> FormatTexelInfo
{
	using enum Format;

	auto const vkFormat = translate_format(format);

	switch (vkFormat)
	{
	case VK_FORMAT_R4G4_UNORM_PACK8:
	case VK_FORMAT_R8_UNORM:
	case VK_FORMAT_R8_SNORM:
	case VK_FORMAT_R8_USCALED:
	case VK_FORMAT_R8_SSCALED:
	case VK_FORMAT_R8_UINT:
	case VK_FORMAT_R8_SINT:
	case VK_FORMAT_R8_SRGB:
	case VK_FORMAT_S8_UINT:
		return FormatTexelInfo{ .size = 1ull, .blockDimension { .width = 1u, .height = 1u, .depth = 1u }, .texelPerBlock = 1u };
	//case VK_FORMAT_A1B5G5R5_UNORM_PACK16:
	case VK_FORMAT_R10X6_UNORM_PACK16:
	case VK_FORMAT_R12X4_UNORM_PACK16:
	case VK_FORMAT_A4R4G4B4_UNORM_PACK16:
	case VK_FORMAT_A4B4G4R4_UNORM_PACK16:
	case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
	case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
	case VK_FORMAT_R5G6B5_UNORM_PACK16:
	case VK_FORMAT_B5G6R5_UNORM_PACK16:
	case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
	case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
	case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
	case VK_FORMAT_R8G8_UNORM:
	case VK_FORMAT_R8G8_SNORM:
	case VK_FORMAT_R8G8_USCALED:
	case VK_FORMAT_R8G8_SSCALED:
	case VK_FORMAT_R8G8_UINT:
	case VK_FORMAT_R8G8_SINT:
	case VK_FORMAT_R8G8_SRGB:
	case VK_FORMAT_R16_UNORM:
	case VK_FORMAT_R16_SNORM:
	case VK_FORMAT_R16_USCALED:
	case VK_FORMAT_R16_SSCALED:
	case VK_FORMAT_R16_UINT:
	case VK_FORMAT_R16_SINT:
	case VK_FORMAT_R16_SFLOAT:
	case VK_FORMAT_D16_UNORM:
		return FormatTexelInfo{ .size = 2ull, .blockDimension { .width = 1u, .height = 1u, .depth = 1u }, .texelPerBlock = 1u };
	case VK_FORMAT_R8G8B8_UNORM:
	case VK_FORMAT_R8G8B8_SNORM:
	case VK_FORMAT_R8G8B8_USCALED:
	case VK_FORMAT_R8G8B8_SSCALED:
	case VK_FORMAT_R8G8B8_UINT:
	case VK_FORMAT_R8G8B8_SINT:
	case VK_FORMAT_R8G8B8_SRGB:
	case VK_FORMAT_B8G8R8_UNORM:
	case VK_FORMAT_B8G8R8_SNORM:
	case VK_FORMAT_B8G8R8_USCALED:
	case VK_FORMAT_B8G8R8_SSCALED:
	case VK_FORMAT_B8G8R8_UINT:
	case VK_FORMAT_B8G8R8_SINT:
	case VK_FORMAT_B8G8R8_SRGB:
	case VK_FORMAT_D16_UNORM_S8_UINT:
	case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM:
		return FormatTexelInfo{ .size = 3ull, .blockDimension { .width = 1u, .height = 1u, .depth = 1u }, .texelPerBlock = 1u };
	case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
	case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
	case VK_FORMAT_R8G8B8A8_UNORM:
	case VK_FORMAT_R8G8B8A8_SNORM:
	case VK_FORMAT_R8G8B8A8_USCALED:
	case VK_FORMAT_R8G8B8A8_SSCALED:
	case VK_FORMAT_R8G8B8A8_UINT:
	case VK_FORMAT_R8G8B8A8_SINT:
	case VK_FORMAT_R8G8B8A8_SRGB:
	case VK_FORMAT_B8G8R8A8_UNORM:
	case VK_FORMAT_B8G8R8A8_SNORM:
	case VK_FORMAT_B8G8R8A8_USCALED:
	case VK_FORMAT_B8G8R8A8_SSCALED:
	case VK_FORMAT_B8G8R8A8_UINT:
	case VK_FORMAT_B8G8R8A8_SINT:
	case VK_FORMAT_B8G8R8A8_SRGB:
	case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
	case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
	case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
	case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
	case VK_FORMAT_A8B8G8R8_UINT_PACK32:
	case VK_FORMAT_A8B8G8R8_SINT_PACK32:
	case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
	case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
	case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
	case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
	case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
	case VK_FORMAT_A2R10G10B10_UINT_PACK32:
	case VK_FORMAT_A2R10G10B10_SINT_PACK32:
	case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
	case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
	case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
	case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
	case VK_FORMAT_A2B10G10R10_UINT_PACK32:
	case VK_FORMAT_A2B10G10R10_SINT_PACK32:
	case VK_FORMAT_R16G16_UNORM:
	case VK_FORMAT_R16G16_SNORM:
	case VK_FORMAT_R16G16_USCALED:
	case VK_FORMAT_R16G16_SSCALED:
	case VK_FORMAT_R16G16_UINT:
	case VK_FORMAT_R16G16_SINT:
	case VK_FORMAT_R16G16_SFLOAT:
	case VK_FORMAT_R32_UINT:
	case VK_FORMAT_R32_SINT:
	case VK_FORMAT_R32_SFLOAT:
	case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
	case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
	case VK_FORMAT_X8_D24_UNORM_PACK32:
	case VK_FORMAT_D32_SFLOAT:
	case VK_FORMAT_D24_UNORM_S8_UINT:
		return FormatTexelInfo{ .size = 4ull, .blockDimension { .width = 1u, .height = 1u, .depth = 1u }, .texelPerBlock = 1u };
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		return FormatTexelInfo{ .size = 5ull, .blockDimension { .width = 1u, .height = 1u, .depth = 1u }, .texelPerBlock = 1u };
	case VK_FORMAT_R16G16B16_UNORM:
	case VK_FORMAT_R16G16B16_SNORM:
	case VK_FORMAT_R16G16B16_USCALED:
	case VK_FORMAT_R16G16B16_SSCALED:
	case VK_FORMAT_R16G16B16_UINT:
	case VK_FORMAT_R16G16B16_SINT:
	case VK_FORMAT_R16G16B16_SFLOAT:
	case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
	case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
	case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
	case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
	case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
	case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
	case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
	case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
	case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
	case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
	case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
	case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
	case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
	case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
	case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
	case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16:
	case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16:
	case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM:
		return FormatTexelInfo{ .size = 6ull, .blockDimension { .width = 1u, .height = 1u, .depth = 1u }, .texelPerBlock = 1u };
	case VK_FORMAT_R16G16B16A16_UNORM:
	case VK_FORMAT_R16G16B16A16_SNORM:
	case VK_FORMAT_R16G16B16A16_USCALED:
	case VK_FORMAT_R16G16B16A16_SSCALED:
	case VK_FORMAT_R16G16B16A16_UINT:
	case VK_FORMAT_R16G16B16A16_SINT:
	case VK_FORMAT_R16G16B16A16_SFLOAT:
	case VK_FORMAT_R32G32_UINT:
	case VK_FORMAT_R32G32_SINT:
	case VK_FORMAT_R32G32_SFLOAT:
	case VK_FORMAT_R64_UINT:
	case VK_FORMAT_R64_SINT:
	case VK_FORMAT_R64_SFLOAT:
	case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
	case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
		return FormatTexelInfo{ .size = 8ull, .blockDimension { .width = 1u, .height = 1u, .depth = 1u }, .texelPerBlock = 1u };
	case VK_FORMAT_R32G32B32_UINT:
	case VK_FORMAT_R32G32B32_SINT:
	case VK_FORMAT_R32G32B32_SFLOAT:
		return FormatTexelInfo{ .size = 12ull, .blockDimension { .width = 1u, .height = 1u, .depth = 1u }, .texelPerBlock = 1u };
	case VK_FORMAT_R32G32B32A32_UINT:
	case VK_FORMAT_R32G32B32A32_SINT:
	case VK_FORMAT_R32G32B32A32_SFLOAT:
	case VK_FORMAT_R64G64_UINT:
	case VK_FORMAT_R64G64_SINT:
	case VK_FORMAT_R64G64_SFLOAT:
		return FormatTexelInfo{ .size = 16ull, .blockDimension { .width = 1u, .height = 1u, .depth = 1u }, .texelPerBlock = 1u };
	case VK_FORMAT_R64G64B64_UINT:
	case VK_FORMAT_R64G64B64_SINT:
	case VK_FORMAT_R64G64B64_SFLOAT:
		return FormatTexelInfo{ .size = 24ull, .blockDimension { .width = 1u, .height = 1u, .depth = 1u }, .texelPerBlock = 1u };
	case VK_FORMAT_R64G64B64A64_UINT:
	case VK_FORMAT_R64G64B64A64_SINT:
	case VK_FORMAT_R64G64B64A64_SFLOAT:
		return FormatTexelInfo{ .size = 32ull, .blockDimension { .width = 1u, .height = 1u, .depth = 1u }, .texelPerBlock = 1u };
	case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
	case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
	case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
	case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
	case VK_FORMAT_BC4_UNORM_BLOCK:
	case VK_FORMAT_BC4_SNORM_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
	case VK_FORMAT_EAC_R11_UNORM_BLOCK:
	case VK_FORMAT_EAC_R11_SNORM_BLOCK:
		return FormatTexelInfo{ .size = 8ull, .blockDimension { .width = 4u, .height = 4u, .depth = 1u }, .texelPerBlock = 16u };
	case VK_FORMAT_BC2_UNORM_BLOCK:
	case VK_FORMAT_BC2_SRGB_BLOCK:
	case VK_FORMAT_BC3_UNORM_BLOCK:
	case VK_FORMAT_BC3_SRGB_BLOCK:
	case VK_FORMAT_BC5_UNORM_BLOCK:
	case VK_FORMAT_BC5_SNORM_BLOCK:
	case VK_FORMAT_BC6H_UFLOAT_BLOCK:
	case VK_FORMAT_BC6H_SFLOAT_BLOCK:
	case VK_FORMAT_BC7_UNORM_BLOCK:
	case VK_FORMAT_BC7_SRGB_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
	case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
	case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
	case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
	case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK:
	case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
	case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
		return FormatTexelInfo{ .size = 16ull, .blockDimension { .width = 4u, .height = 4u, .depth = 1u }, .texelPerBlock = 16u };
	case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK:
	case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
	case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
		return FormatTexelInfo{ .size = 16ull, .blockDimension { .width = 5u, .height = 4u, .depth = 1u }, .texelPerBlock = 20u };
	case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK:
	case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
	case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
		return FormatTexelInfo{ .size = 16ull, .blockDimension { .width = 5u, .height = 5u, .depth = 1u }, .texelPerBlock = 25u };
	case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK:
	case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
	case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
		return FormatTexelInfo{ .size = 16ull, .blockDimension { .width = 6u, .height = 5u, .depth = 1u }, .texelPerBlock = 30u };
	case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK:
	case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
	case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
		return FormatTexelInfo{ .size = 16ull, .blockDimension { .width = 6u, .height = 6u, .depth = 1u }, .texelPerBlock = 36u };
	case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK:
	case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
	case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
		return FormatTexelInfo{ .size = 16ull, .blockDimension { .width = 8u, .height = 5u, .depth = 1u }, .texelPerBlock = 40u };
	case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK:
	case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
	case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
		return FormatTexelInfo{ .size = 16ull, .blockDimension { .width = 8u, .height = 6u, .depth = 1u }, .texelPerBlock = 48u };
	case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK:
	case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
	case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
		return FormatTexelInfo{ .size = 16ull, .blockDimension { .width = 8u, .height = 8u, .depth = 1u }, .texelPerBlock = 64u };
	case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK:
	case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
	case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
		return FormatTexelInfo{ .size = 16ull, .blockDimension { .width = 10u, .height = 5u, .depth = 1u }, .texelPerBlock = 50u };
	case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK:
	case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
	case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
		return FormatTexelInfo{ .size = 16ull, .blockDimension { .width = 10u, .height = 6u, .depth = 1u }, .texelPerBlock = 60u };
	case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK:
	case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
	case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
		return FormatTexelInfo{ .size = 16ull, .blockDimension { .width = 10u, .height = 8u, .depth = 1u }, .texelPerBlock = 80u };
	case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK:
	case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
	case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
		return FormatTexelInfo{ .size = 16ull, .blockDimension { .width = 10u, .height = 10u, .depth = 1u }, .texelPerBlock = 100u };
	case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK:
	case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
	case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
		return FormatTexelInfo{ .size = 16ull, .blockDimension { .width = 12u, .height = 10u, .depth = 1u }, .texelPerBlock = 120u };
	case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK:
	case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
	case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
		return FormatTexelInfo{ .size = 16ull, .blockDimension { .width = 12u, .height = 12u, .depth = 1u }, .texelPerBlock = 144u };
	case VK_FORMAT_G8B8G8R8_422_UNORM:
	case VK_FORMAT_B8G8R8G8_422_UNORM:
		return FormatTexelInfo{ .size = 4ull, .blockDimension { .width = 2u, .height = 1u, .depth = 1u }, .texelPerBlock = 1u };
	case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
	case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
	case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
	case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
	case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
		return FormatTexelInfo{ .size = 3ull, .blockDimension { .width = 1u, .height = 1u, .depth = 1u }, .texelPerBlock = 1u };
	case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
	case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
	case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
	case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
	case VK_FORMAT_G16B16G16R16_422_UNORM:
	case VK_FORMAT_B16G16R16G16_422_UNORM:
		return FormatTexelInfo{ .size = 8ull, .blockDimension { .width = 2u, .height = 1u, .depth = 1u }, .texelPerBlock = 1u };
	case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
	case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
	case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
	case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
		return FormatTexelInfo{ .size = 8ull, .blockDimension { .width = 8u, .height = 4u, .depth = 1u }, .texelPerBlock = 1u };
	case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
	case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
	case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
	case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
		return FormatTexelInfo{ .size = 8ull, .blockDimension { .width = 4u, .height = 4u, .depth = 1u }, .texelPerBlock = 1u };
	case VK_FORMAT_UNDEFINED:
	default:
		return FormatTexelInfo{};
	}
}
}