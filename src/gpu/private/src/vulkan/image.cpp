#include "vulkan/vkgpu.hpp"

namespace gpu
{
Image::Image(Device& device) :
	DeviceResource{ device },
	m_info{}
{}

auto Image::info() const -> ImageInfo const&
{
	return m_info;
}

auto Image::valid() const -> bool
{
	auto const& self = to_impl(*this);
	
	return m_device != nullptr && self.handle != VK_NULL_HANDLE;
}

auto Image::is_swapchain_image() const -> bool
{
	auto const& self = to_impl(*this);

	/**
	* Swapchain images will not contain an allocation handle because they are retrieved from the driver instead.
	*/
	return !self.allocationBlock.valid();
}

auto Image::is_transient() const -> bool
{
	auto const& self = to_impl(*this);

	return self.allocationBlock.valid() && self.allocationBlock->aliased();
}

auto Image::memory_requirement(Device& device, ImageInfo const& info) -> MemoryRequirementInfo
{
	auto&& vkdevice = to_device(device);

	VkImageCreateInfo imgCreateInfo = vk::get_image_create_info(info);

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

auto Image::bind(ImageBindInfo const& info) const -> ImageBindInfo
{
	auto&& self = to_impl(*this);
	auto&& vkdevice = to_device(self.device());

	uint32 count = 0;
	std::array<VkWriteDescriptorSet, 2> descriptorSetWrites{};

	ImageInfo const& imageInfo = self.info();

	uint32 index = info.index;

	index = index % vkdevice.config().maxImages;

	VkDescriptorImageInfo descriptorSampledImageInfo{
		.sampler = VK_NULL_HANDLE,
		.imageView = self.imageView,
		.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
	};

	if (info.sampler.valid())
	{
		auto&& vksampler = to_impl(*info.sampler);
		descriptorSampledImageInfo.sampler = vksampler.handle;
	}

	VkDescriptorImageInfo descriptorStorageImageInfo{
		.sampler = VK_NULL_HANDLE,
		.imageView = self.imageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL
	};

	if ((imageInfo.imageUsage & ImageUsage::Sampled) != ImageUsage::None)
	{
		descriptorSetWrites[count++] = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = vkdevice.descriptorCache.descriptorSet,
			.dstBinding = info.sampler.valid() ? COMBINED_IMAGE_SAMPLER_BINDING : SAMPLED_IMAGE_BINDING,
			.dstArrayElement = index,
			.descriptorCount = 1,
			.descriptorType = info.sampler.valid() ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			.pImageInfo = &descriptorSampledImageInfo
		};
	}

	if ((imageInfo.imageUsage & ImageUsage::Storage) != ImageUsage::None)
	{
		descriptorSetWrites[count++] = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = vkdevice.descriptorCache.descriptorSet,
			.dstBinding = STORAGE_IMAGE_BINDING,
			.dstArrayElement = index,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.pImageInfo = &descriptorStorageImageInfo
		};
	}

	vkUpdateDescriptorSets(vkdevice.device, count, descriptorSetWrites.data(), 0, nullptr);

	return ImageBindInfo{ .sampler = info.sampler, .index = index };
}

auto Image::from(Device& device, ImageInfo&& info, Resource<MemoryBlock> memoryBlock) -> Resource<Image>
{
	auto&& vkdevice = to_device(device);

	uint32 queueFamilyIndices[] = {
		vkdevice.mainQueue.familyIndex,
		vkdevice.computeQueue.familyIndex,
		vkdevice.transferQueue.familyIndex
	};

	VkImageCreateInfo imgInfo = vk::get_image_create_info(info);

	if ((imgInfo.sharingMode & VK_SHARING_MODE_CONCURRENT) != 0)
	{
		imgInfo.queueFamilyIndexCount = 3u;
		imgInfo.pQueueFamilyIndices = queueFamilyIndices;
	}

	VkImage handle = VK_NULL_HANDLE;

	if (memoryBlock.valid())
	{
		auto const& memBlockImpl = to_impl(*memoryBlock);
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
		auto allocationFlags = vk::translate_memory_usage(info.memoryUsage);

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

		auto it = vkdevice.gpuResourcePool.memoryBlocks.emplace(vkdevice, false);

		auto&& vkmemoryblock = *it;

		vkmemoryblock.handle = allocation;
		vkmemoryblock.allocationInfo = std::move(allocationInfo);

		new (&memoryBlock) Resource<MemoryBlock>{ vkmemoryblock, vk::to_id(it) };
	}

	auto it = vkdevice.gpuResourcePool.images.emplace(vkdevice);

	auto&& vkimage = *it;

	if (!info.name.empty())
	{
		info.name.format("<image>:{}", info.name.c_str());
	}

	vkimage.handle = handle;
	vkimage.allocationBlock = memoryBlock;
	vkimage.m_info = std::move(info);

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
		.viewType = vk::translate_image_view_type(vkimage.m_info.type),
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

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vkimage);
	}

	return Resource<Image>{ vkimage, vk::to_id(it) };
}

auto Image::from(Swapchain& swapchain) -> lib::array<Resource<Image>>
{
	auto&& vkdevice = to_device(swapchain.device());
	auto&& vkswapchain = *static_cast<vk::SwapchainImpl*>(&swapchain);

	auto&& swapchainInfo = swapchain.info();

	// Ideally should be const but vkGetSwapchainImagesKHR does not accept a const uint32* on pSwapchainImageCount.
	uint32 maxImageCount = (swapchainInfo.imageCount > MAX_FRAMES_IN_FLIGHT) ? MAX_FRAMES_IN_FLIGHT : swapchainInfo.imageCount;

	lib::array<Resource<Image>> images{ static_cast<size_t>(maxImageCount) };
	std::array<VkImage, MAX_FRAMES_IN_FLIGHT> vkImageHandles = {};
	std::array<VkImageView, MAX_FRAMES_IN_FLIGHT> vkImageViewHandles = {};

	for (uint32 i = 0; i < maxImageCount; ++i)
	{
		vkGetSwapchainImagesKHR(vkdevice.device, vkswapchain.handle, &maxImageCount, vkImageHandles.data());
	}

	VkImageViewCreateInfo imageViewInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = vkswapchain.surfaceColorFormat.format,
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
			.format = static_cast<Format>(vkswapchain.surfaceColorFormat.format),
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

		if (swapchainInfo.name.size())
		{
			imageInfo.name = lib::format("<image>:{}_{}", swapchainInfo.name.c_str(), i);
		}

		auto it = vkdevice.gpuResourcePool.images.emplace(vkdevice);

		auto&& vkimage = *it;

		vkimage.handle = vkImageHandles[i];
		vkimage.imageView = vkImageViewHandles[i];
		vkimage.m_info = std::move(imageInfo);

		if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
		{
			vkdevice.setup_debug_name(vkimage);
		}

		images.emplace_back(vkimage, vk::to_id(it));
	}

	return images;
}

auto Image::destroy(Image& resource, Id id) -> void
{
	/*
	* At this point, the ref count on the resource is only 1 which means ONLY the resource has a reference to itself and can be safely deleted.
	*/
	auto&& vkdevice = to_device(resource.m_device);
	auto&& vkimage = to_impl(resource);

	/*
	* Release ownership of it's memory allocation.
	*/
	vkimage.allocationBlock.destroy();

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();

	vkdevice.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&vkimage, id](vk::DeviceImpl& device) -> void
		{
			if (!vkimage.is_transient())
			{
				vkDestroyImageView(device.device, vkimage.imageView, nullptr);

				if (!vkimage.is_swapchain_image())
				{
					using iterator = typename lib::hive<vk::MemoryBlockImpl>::iterator;

					auto&& block = to_impl(*vkimage.allocationBlock);
			
					auto const it = vk::to_hive_it<iterator>(vkimage.allocationBlock.id());
	
					vmaDestroyImage(device.allocator, vkimage.handle, block.handle);

					device.gpuResourcePool.memoryBlocks.erase(it);
				}
			}
			else
			{
				vkDestroyImageView(device.device, vkimage.imageView, nullptr);
				vkDestroyImage(device.device, vkimage.handle, nullptr);
			}

			using iterator = typename lib::hive<vk::ImageImpl>::iterator;

			device.gpuResourcePool.images.erase(vk::to_hive_it<iterator>(id));
		}
	);
}

namespace vk
{
ImageImpl::ImageImpl(DeviceImpl& device) :
	Image{ device }
{}
}

auto format_texel_info(Format format) -> FormatTexelInfo
{
	using enum Format;

	auto const vkFormat = vk::translate_format(format);

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