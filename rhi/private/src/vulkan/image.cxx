module;

//#include <cmath>
#include <mutex>

#include "vulkan/vk.h"

#include "lib/string.h"

module forge;

namespace frg
{
auto Image::info() const -> ImageInfo const&
{
	return m_info;
}

auto Image::valid() const -> bool
{
	return m_impl.handle != VK_NULL_HANDLE;
}

auto Image::is_swapchain_image() const -> bool
{
	/**
	 * For Image's that belong to the swapchain, we know it is one when the VmaAllocation handle is null.
	 */
	return m_impl.allocation == VK_NULL_HANDLE;
}


auto Image::from(Device& device, ImageInfo&& info) -> Resource<Image>
{
	auto&& [index, image] = device.m_gpuResourcePool.images.emplace(device);
	constexpr uint32 MAX_IMAGE_MIP_LEVEL = 4u;

	VkFormat format = api::translate_format(info.format);
	VkImageUsageFlags usage = api::translate_image_usage_flags(info.imageUsage);

	VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	if ((usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0)
	{
		switch (format)
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

	uint32 depth{ 1 };

	if (info.dimension.depth > 1)
	{
		depth = info.dimension.depth;
	}

	uint32 mipLevels{ 1 };

	if (info.mipLevel > 1)
	{
		const float32 width = static_cast<const float32>(info.dimension.width);
		const float32 height = static_cast<const float32>(info.dimension.height);

		mipLevels = static_cast<uint32>(std::floorf(std::log2f(std::max(width, height)))) + 1u;
		mipLevels = std::min(mipLevels, MAX_IMAGE_MIP_LEVEL);
	}


	VkImageCreateInfo imgInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = api::translate_image_type(info.type),
		.format = format,
		.extent = {
			.width = info.dimension.width,
			.height = info.dimension.height,
			.depth = depth
		},
		.mipLevels = mipLevels,
		.arrayLayers = 1,
		.samples = api::translate_sample_count(info.samples),
		.tiling = api::translate_image_tiling(info.tiling),
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	info.name.format("<image>:{}", info.name.c_str());

	auto allocationFlags = api::translate_memory_usage(info.memoryUsage);

	if ((allocationFlags & VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT) != 0 ||
		(allocationFlags & VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT) != 0)
	{
		allocationFlags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
	}

	VmaAllocationCreateInfo allocInfo{
		.flags = allocationFlags,
		.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
	};
	VmaAllocationInfo allocationInfo = {};

	VkResult result = vmaCreateImage(device.m_apiContext.allocator, &imgInfo, &allocInfo, &image.m_impl.handle, &image.m_impl.allocation, &allocationInfo);
	
	if (result != VK_SUCCESS)
	{
		device.m_gpuResourcePool.images.erase(index);

		return null_resource;
	}

	VkImageViewCreateInfo imgViewInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image.m_impl.handle,
		.viewType = api::translate_image_view_type(info.type),
		.format = format,
		.subresourceRange = {
			.aspectMask = aspectFlags,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	vkCreateImageView(device.m_apiContext.device, &imgViewInfo, nullptr, &image.m_impl.imageView);

	return Resource{ index, image };
}

auto Image::from(Swapchain& swapchain) -> lib::array<Resource<Image>>
{
	Device& device = swapchain.device();
	auto&& swapchainInfo = swapchain.info();

	// Ideally should be const but vkGetSwapchainImagesKHR does not accept a const uint32* on pSwapchainImageCount.
	uint32 maxImageCount = (swapchainInfo.imageCount > MAX_FRAMES_IN_FLIGHT) ? MAX_FRAMES_IN_FLIGHT : swapchainInfo.imageCount;

	lib::array<Resource<Image>> images{ static_cast<size_t>(maxImageCount) };
	std::array<VkImage, MAX_FRAMES_IN_FLIGHT> vkImageHandles = {};
	std::array<VkImageView, MAX_FRAMES_IN_FLIGHT> vkImageViewHandles = {};

	for (uint32 i = 0; i < maxImageCount; ++i)
	{
		vkGetSwapchainImagesKHR(device.m_apiContext.device, swapchain.m_impl.handle, &maxImageCount, vkImageHandles.data());
	}

	VkImageViewCreateInfo imageViewInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = swapchain.m_impl.surfaceColorFormat.format,
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
		vkCreateImageView(device.m_apiContext.device, &imageViewInfo, nullptr, &vkImageViewHandles[i]);
	}

	for (uint32 i = 0; i < maxImageCount; ++i)
	{
		ImageInfo imageInfo{
			.name = lib::format("<image>:{}_swapchain_image_{}", swapchain.m_info.name.c_str(), i),
			.type = ImageType::Image_2D,
			.format = static_cast<Format>(swapchain.m_impl.surfaceColorFormat.format),
			.samples = SampleCount::Sample_Count_1,
			.tiling = ImageTiling::Optimal,
			.imageUsage = swapchain.m_info.imageUsage,
			.memoryUsage = MemoryUsage::Dedicated,
			.dimension = { 
				.width = swapchain.m_info.dimension.width, 
				.height = swapchain.m_info.dimension.height, 
				.depth = 0u 
			},
			.clearValue = { 
				.color = { 
					.f32 = { 0.f, 0.f, 0.f, 1.f } 
				} 
			},
			.mipLevel = 0
		};

		auto&& [index, image] = device.m_gpuResourcePool.images.emplace(device);

		image.m_info = std::move(imageInfo);
		image.m_impl.handle = vkImageHandles[i];
		image.m_impl.imageView = vkImageViewHandles[i];
		image.m_impl.allocation = VK_NULL_HANDLE;
		image.m_impl.allocationInfo = {};
		
		images.emplace_back(index.to_uint64(), &image);
	}

	return images;
}

auto Image::destroy(Image const& resource, id_type id) -> void
{
	/*
	 * At this point, the ref count on the resource is only 1 which means ONLY the resource has a refernece to itself and can be safely deleted.
	 */

	Device& device = resource.device();

	uint64 const cpuTimelineValue = device.cpu_timeline();

	std::lock_guard const lock{ device.m_gpuResourcePool.zombieMutex };

	device.m_gpuResourcePool.zombies.push_back({ .timeline = cpuTimelineValue, .id = id.to_uint64(), .type = resource_type_id_v<Image> });
}

Image::Image(Device& device) :
	RefCountedDeviceResource{ device },
	m_info{},
	m_impl{}
{}

}