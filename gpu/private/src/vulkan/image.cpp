#include "vulkan/vkgpu.h"

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
	return self.allocationBlock.valid();
}

auto Image::is_transient() const -> bool
{
	auto const& self = to_impl(*this);

	return self.allocationBlock->aliased();
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
			return null_resource;
		}

		if (vkCreateImage(vkdevice.device, &imgInfo, nullptr, &handle) != VK_SUCCESS)
		{
			return null_resource;
		}

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

		if (vmaCreateImage(vkdevice.allocator, &imgInfo, &allocInfo, &handle, &allocation, &allocationInfo) != VK_SUCCESS)
		{
			return null_resource;
		}

		auto&& [memoryBlockId, vkmemoryblock] = vkdevice.gpuResourcePool.memoryBlocks.emplace(vkdevice, false);

		vkmemoryblock.handle = allocation;
		vkmemoryblock.allocationInfo = std::move(allocationInfo);

		new (&memoryBlock) Resource<MemoryBlock>{ memoryBlockId.to_uint64(), vkmemoryblock };
	}

	auto&& [id, vkimage] = vkdevice.gpuResourcePool.images.emplace(vkdevice);

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

	if constexpr (ENABLE_DEBUG_RESOURCE_NAMES)
	{
		vkdevice.setup_debug_name(vkimage);
	}

	return Resource<Image>{ id.to_uint64(), vkimage };
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
				.depth = 0u
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

		auto&& [index, vkimage] = vkdevice.gpuResourcePool.images.emplace(vkdevice);

		vkimage.handle = vkImageHandles[i];
		vkimage.imageView = vkImageViewHandles[i];
		vkimage.m_info = std::move(imageInfo);

		if constexpr (ENABLE_DEBUG_RESOURCE_NAMES)
		{
			vkdevice.setup_debug_name(vkimage);
		}

		images.emplace_back(index.to_uint64(), vkimage);
	}

	return images;
}

auto Image::destroy(Image& resource, uint64 id) -> void
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

	vkdevice.gpuResourcePool.zombies.emplace_back(cpuTimelineValue, id, vk::ResourceType::Image);
}

namespace vk
{
ImageImpl::ImageImpl(DeviceImpl& device) :
	Image{ device }
{}
}
}