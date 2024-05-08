module;

#include <mutex>

#include "vulkan/vk.h"
#include "lib/string.h"

module forge;

namespace frg
{
auto Swapchain::info() const -> SwapchainInfo const&
{
	return m_info;
}

auto Swapchain::valid() const -> bool
{
	return m_impl.handle != VK_NULL_HANDLE;
}

auto Swapchain::state() const -> SwapchainState
{
	return m_state;
}

auto Swapchain::num_images() const -> uint32
{
	return static_cast<uint32>(m_images.size());
}

auto Swapchain::current_image() const -> Resource<Image>
{
	if (m_device != nullptr &&
		m_impl.handle != VK_NULL_HANDLE)
	{
		return m_images[m_nextImageIndex];
	}
	return null_resource;
}

auto Swapchain::acquire_next_image() -> Resource<Image>
{
	if (m_device == nullptr ||
		m_impl.handle == VK_NULL_HANDLE)
	{
		return null_resource;
	}

	int64 const maxFramesInFlight = static_cast<int64>(m_device->m_config.maxFramesInFlight);
	// We wait until the gpu elapsed frame count is behind our swapchain's elapsed frame count.
	//[[maybe_unused]] uint64 currentValue = m_gpu_elapsed_frames.value();
	m_gpuElapsedFrames->wait_for_value(static_cast<uint64>(std::max(0ll, static_cast<int64>(m_cpuElapsedFrames))));

	// Update swapchain's frame index for the next call to this function.
	m_currentFrameIndex = (m_currentFrameIndex + 1) % maxFramesInFlight;

	Resource<Semaphore>& semaphore = m_acquireSemaphore[m_currentFrameIndex];

	VkResult result = vkAcquireNextImageKHR(
		m_device->m_apiContext.device,
		m_impl.handle,
		UINT64_MAX,
		semaphore->m_impl.handle,
		nullptr,
		&m_nextImageIndex
	);
	switch (result)
	{
	case VK_SUCCESS:
		m_state = SwapchainState::Ok;
		break;
	case VK_NOT_READY:
		m_state = SwapchainState::Not_Ready;
		break;
	case VK_TIMEOUT:
		m_state = SwapchainState::Timed_Out;
		break;
	case VK_SUBOPTIMAL_KHR:
		m_state = SwapchainState::Suboptimal;
		break;
	default:
		m_state = SwapchainState::Error;
		break;
	}

	// Increment total swapchain elapsed frame count.
	m_cpuElapsedFrames += 1;

	return m_images[static_cast<size_t>(m_nextImageIndex)];
}

auto Swapchain::current_acquire_semaphore() const -> Resource<Semaphore>
{
	return m_acquireSemaphore[m_currentFrameIndex];
}

auto Swapchain::current_present_semaphore() const -> Resource<Semaphore>
{
	return m_presentSemaphore[m_currentFrameIndex];
}

auto Swapchain::cpu_frame_count() const -> uint64
{
	return m_cpuElapsedFrames;
}

auto Swapchain::gpu_frame_count() const -> uint64
{
	return m_gpuElapsedFrames->value();
}

auto Swapchain::get_gpu_fence() const -> Resource<Fence>
{
	return m_gpuElapsedFrames;
}

auto Swapchain::image_format() const -> Format
{
	Format format = Format::Undefined;
	if (m_images.size())
	{
		format = m_images[0]->info().format;
	}
	return format;
}

auto Swapchain::color_space() const -> ColorSpace
{
	return m_colorSpace;
}

auto Swapchain::from(Device& device, SwapchainInfo&& info, Resource<Swapchain> previousSwapchain) -> Resource<Swapchain>
{
	auto create_surface = [&device](SurfaceInfo const& surfaceInfo) -> api::Surface*
	{
		std::uintptr_t const key = reinterpret_cast<std::uintptr_t>(surfaceInfo.instance) | reinterpret_cast<std::uintptr_t>(surfaceInfo.window);
		VkSurfaceKHR handle = VK_NULL_HANDLE;

#ifdef PLATFORM_OS_WINDOWS
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{
			.sType		= VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.hinstance	= static_cast<HINSTANCE>(surfaceInfo.instance),
			.hwnd		= static_cast<HWND>(surfaceInfo.window)
		};

		VkResult result = vkCreateWin32SurfaceKHR(device.m_apiContext.instance, &surfaceCreateInfo, nullptr, &handle);

		if (result != VK_SUCCESS)
		{
			return nullptr;
		}
#elif PLATFORM_OS_LINUX
#endif

		auto&& [k, pSurface] = device.m_apiContext.surfaceCache.emplace(key, std::make_unique<api::Surface>(handle));

		uint32 count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device.m_apiContext.gpu, handle, &count, nullptr);

		pSurface->availableColorFormats.reserve(static_cast<size_t>(count));
		pSurface->availableColorFormats.resize(static_cast<size_t>(count));

		vkGetPhysicalDeviceSurfaceFormatsKHR(device.m_apiContext.gpu, handle, &count, pSurface->availableColorFormats.data());
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.m_apiContext.gpu, handle, &pSurface->capabilities);

		return pSurface.get();
	};

	if (!info.surfaceInfo.instance ||
		!info.surfaceInfo.window)
	{
		return null_resource;
	}

	std::uintptr_t surfaceAddress = reinterpret_cast<std::uintptr_t>(info.surfaceInfo.instance) | reinterpret_cast<std::uintptr_t>(info.surfaceInfo.window);
	auto cachedSurface = device.m_apiContext.surfaceCache.at(surfaceAddress);

	api::Surface* pSurface			= nullptr;
	api::Swapchain* pOldSwapchain	= nullptr;

	if (previousSwapchain.valid())
	{
		pOldSwapchain = &previousSwapchain->m_impl;
	}

	if (cachedSurface.is_null())
	{
		pSurface = create_surface(info.surfaceInfo);
	}
	else
	{
		pSurface = cachedSurface->second.get();
	}

	auto&& [index, swapchain] = device.m_gpuResourcePool.swapchains.emplace(device);

	VkExtent2D const extent{
		.width  = std::min(info.dimension.width, pSurface->capabilities.currentExtent.width),
		.height = std::min(info.dimension.height, pSurface->capabilities.currentExtent.height)
	};
	uint32 imageCount{ std::min(std::max(info.imageCount, pSurface->capabilities.minImageCount), pSurface->capabilities.maxImageCount) };

	VkImageUsageFlags imageUsage		= api::translate_image_usage_flags(info.imageUsage);
	VkPresentModeKHR presentationMode	= api::translate_swapchain_presentation_mode(info.presentationMode);

	bool foundPreferred = false;
	VkSurfaceFormatKHR surfaceFormat = pSurface->availableColorFormats[0];

	for (Format format : info.surfaceInfo.preferredSurfaceFormats)
	{
		VkFormat preferredFormat = api::translate_format(format);

		if (foundPreferred)
		{
			break;
		}

		for (VkSurfaceFormatKHR colorFormat : pSurface->availableColorFormats)
		{
			if (preferredFormat == colorFormat.format)
			{
				surfaceFormat = colorFormat;
				foundPreferred = true;
				break;
			}
		}
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = pSurface->handle,
		.minImageCount = imageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = imageUsage,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &device.m_apiContext.mainQueue.familyIndex,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentationMode,
		.clipped = VK_TRUE,
		.oldSwapchain = (pOldSwapchain != nullptr) ? pOldSwapchain->handle : nullptr
	};

	VkResult result = vkCreateSwapchainKHR(device.m_apiContext.device, &swapchainCreateInfo, nullptr, &swapchain.m_impl.handle);

	if (result != VK_SUCCESS)
	{
		device.m_gpuResourcePool.swapchains.erase(index);

		return null_resource;
	}

	swapchain.m_impl.pSurface = pSurface;
	swapchain.m_impl.surfaceColorFormat = surfaceFormat;
	swapchain.m_colorSpace = api::vk_to_rhi_color_space(surfaceFormat.colorSpace);

	// Update info to contain the updated values.
	info.name.format("<swapchain>:{}", info.name.c_str());
	info.surfaceInfo.name.format("<surface>:{}", info.surfaceInfo.name.c_str());
	info.imageCount = imageCount;
	info.dimension.width = extent.width;
	info.dimension.height = extent.height;

	swapchain.m_info = std::move(info);
	
	swapchain.m_gpuElapsedFrames = Fence::from(
		device, 
		{ 
			.name = lib::format("<fence>:{}_gpu_timeline", swapchain.m_info.name.c_str()), 
			.initialValue = 0 
		}
	);

	size_t const maxFramesInFlight = static_cast<size_t>(device.config().maxFramesInFlight);

	swapchain.m_acquireSemaphore.reserve(maxFramesInFlight);
	swapchain.m_presentSemaphore.reserve(maxFramesInFlight);

	for (size_t i = 0; i < maxFramesInFlight; ++i)
	{
		swapchain.m_acquireSemaphore.push_back(Semaphore::from(device, { .name = lib::format("<semaphore>:{}_acquire_{}", swapchain.m_info.name.c_str(), i) }));
		swapchain.m_presentSemaphore.push_back(Semaphore::from(device, { .name = lib::format("<semaphore>:{}_present_{}", swapchain.m_info.name.c_str(), i) }));
	}

	swapchain.m_images = Image::from(swapchain);

	return Resource{ index, swapchain };
}

auto Swapchain::destroy(Swapchain const& resource, id_type id) -> void
{
	/*
	 * At this point, the ref count on the resource is only 1 which means ONLY the resource has a refernece to itself and can be safely deleted.
	 */
	Device& device = resource.device();

	uint64 const cpuTimelineValue = device.cpu_timeline();

	std::lock_guard const lock{ device.m_gpuResourcePool.zombieMutex };

	device.m_gpuResourcePool.zombies.push_back({ .timeline = cpuTimelineValue, .id = id.to_uint64(), .type = resource_type_id_v<Swapchain> });
}

Swapchain::Swapchain(Device& device) :
	RefCountedDeviceResource{ device },
	m_info{},
	m_impl{},
	m_state{ SwapchainState::Error },
	m_images{},
	m_gpuElapsedFrames{ null_resource },
	m_acquireSemaphore{},
	m_presentSemaphore{},
	m_colorSpace{ ColorSpace::Srgb_Non_Linear },
	m_cpuElapsedFrames{},
	m_currentFrameIndex{},
	m_nextImageIndex{}
{}
}