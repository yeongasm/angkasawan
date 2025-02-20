#include "vulkan/vkgpu.hpp"

namespace gpu
{
Swapchain::Swapchain(Device& device) :
	DeviceResource{ device },
	m_info{},
	m_state{},
	m_images{},
	m_gpuElapsedFrames{},
	m_acquireSemaphore{},
	m_presentSemaphore{},
	m_colorSpace{ ColorSpace::Srgb_Non_Linear },
	m_cpuElapsedFrames{},
	m_currentFrameIndex{},
	m_nextImageIndex{}
{}

auto Swapchain::info() const -> SwapchainInfo const&
{
	return m_info;
}

auto Swapchain::valid() const -> bool
{
	auto const& self = to_impl(*this);

	return m_device != nullptr && self.handle != VK_NULL_HANDLE;
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
	if (valid())
	{
		return m_images[m_nextImageIndex];
	}
	return {};
}

auto Swapchain::current_image_index() const -> uint32
{
	return valid() ? m_nextImageIndex : 0;
}

auto Swapchain::acquire_next_image() -> Resource<Image>
{
	if (!valid())
	{
		return {};
	}

	auto&& vkdevice = to_device(m_device);
	auto const& self = to_impl(*this);

	int64 const maxFramesInFlight = static_cast<int64>(vkdevice.config().maxFramesInFlight);
	// We wait until the gpu elapsed frame count is behind our swapchain's elapsed frame count.
	//[[maybe_unused]] uint64 currentValue = m_gpu_elapsed_frames.value();
	m_gpuElapsedFrames->wait_for_value(static_cast<uint64>(std::max(0ll, static_cast<int64>(m_cpuElapsedFrames))));

	// Update swapchain's frame index for the next call to this function.
	m_previousFrameIndex = m_currentFrameIndex;
	m_currentFrameIndex = (m_currentFrameIndex + 1) % maxFramesInFlight;

	vk::SemaphoreImpl const& vksemaphore = to_impl(*m_acquireSemaphore[m_currentFrameIndex]);

	VkResult result = vkAcquireNextImageKHR(
		vkdevice.device,
		self.handle,
		UINT64_MAX,
		vksemaphore.handle,
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
	m_cpuElapsedFrames.fetch_add(1, std::memory_order_relaxed);

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
	return m_cpuElapsedFrames.load(std::memory_order_acquire);
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

	if (!m_images.empty())
	{
		format = m_images[0]->info().format;
	}

	return format;
}

auto Swapchain::color_space() const -> ColorSpace
{
	return m_colorSpace;
}

auto Swapchain::resize(Extent2D dim) -> bool
{
	auto&& self = to_impl(*this);
	auto&& vkdevice = to_device(m_device);

	auto&& surface = *self.pSurface;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkdevice.gpu, surface.handle, &surface.capabilities);

	VkExtent2D const extent{
		.width	= std::min(dim.width, surface.capabilities.currentExtent.width),
		.height = std::min(dim.height, surface.capabilities.currentExtent.height)
	};
	uint32 imageCount{ std::min(std::max(m_info.imageCount, surface.capabilities.minImageCount), surface.capabilities.maxImageCount) };

	VkImageUsageFlags imageUsage = vk::translate_image_usage_flags(m_info.imageUsage);
	VkPresentModeKHR presentationMode = vk::translate_swapchain_presentation_mode(m_info.presentationMode);

	bool foundPreferred = false;
	VkSurfaceFormatKHR surfaceFormat = surface.availableColorFormats[0];

	for (Format format : m_info.surfaceInfo.preferredSurfaceFormats)
	{
		VkFormat preferredFormat = vk::translate_format(format);

		if (foundPreferred)
		{
			break;
		}

		for (VkSurfaceFormatKHR colorFormat : surface.availableColorFormats)
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
		.surface = surface.handle,
		.minImageCount = imageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = imageUsage,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &vkdevice.mainQueue.familyIndex,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentationMode,
		.clipped = VK_TRUE,
		.oldSwapchain = self.handle
	};

	if (vkCreateSwapchainKHR(vkdevice.device, &swapchainCreateInfo, nullptr, &self.handle) != VK_SUCCESS)
	{
		return false;
	}

	m_info.dimension.width	= extent.width;
	m_info.dimension.height = extent.height;

	uint32 maxImageCount = (self.m_info.imageCount > MAX_FRAMES_IN_FLIGHT) ? MAX_FRAMES_IN_FLIGHT : self.m_info.imageCount;

	std::array<VkImage, MAX_FRAMES_IN_FLIGHT> vkImageHandles = {};

	for (uint32 i = 0; i < maxImageCount; ++i)
	{
		vkGetSwapchainImagesKHR(vkdevice.device, self.handle, &maxImageCount, vkImageHandles.data());
	}

	VkImageViewCreateInfo imageViewInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = self.surfaceColorFormat.format,
		.components = {
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
	};

	for (size_t i = 0; auto&& image : m_images)
	{
		auto&& vkimage = to_impl(*image);

		vkDestroyImageView(vkdevice.device, vkimage.imageView, nullptr);

		vkimage.handle = vkImageHandles[i];

		vkimage.m_info.dimension.width	= m_info.dimension.width;
		vkimage.m_info.dimension.height	= m_info.dimension.height;

		imageViewInfo.image = vkimage.handle;

		vkCreateImageView(vkdevice.device, &imageViewInfo, nullptr, &vkimage.imageView);

		++i;
	}

	return true;
}

auto Swapchain::from(Device& device, SwapchainInfo&& info, Resource<Swapchain> previousSwapchain) -> Resource<Swapchain>
{
	auto&& vkdevice = to_device(device);

	auto create_surface = [&vkdevice](SurfaceInfo const& surfaceInfo) -> vk::Surface*
	{
		VkSurfaceKHR handle = VK_NULL_HANDLE;

#ifdef _WIN64
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.hinstance = static_cast<HINSTANCE>(surfaceInfo.instance),
			.hwnd = static_cast<HWND>(surfaceInfo.window)
		};

		VkResult result = vkCreateWin32SurfaceKHR(vkdevice.instance, &surfaceCreateInfo, nullptr, &handle);

		if (result != VK_SUCCESS)
		{
			return nullptr;
		}
#elif __linux__
#endif

		auto&& [id, vksurface] = vkdevice.gpuResourcePool.surfaces.emplace();

		vksurface.handle = handle;
		vksurface.id = id;

		uint32 count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(vkdevice.gpu, handle, &count, nullptr);

		vksurface.availableColorFormats.reserve(static_cast<size_t>(count));
		vksurface.availableColorFormats.resize(static_cast<size_t>(count));

		vkGetPhysicalDeviceSurfaceFormatsKHR(vkdevice.gpu, handle, &count, vksurface.availableColorFormats.data());

		/**
		* A resource will always have a count of 1 because it references itself.
		*/
		vksurface.refCount.fetch_add(1, std::memory_order_relaxed);

		return &vksurface;
	};

	if (!info.surfaceInfo.instance || !info.surfaceInfo.window)
	{
		return {};
	}

	vk::Surface* pSurface = nullptr;
	vk::SwapchainImpl* pOldSwapchain = nullptr;

	if (previousSwapchain.valid())
	{
		pOldSwapchain = &to_impl(*previousSwapchain);
		pSurface = pOldSwapchain->pSurface;
	}
	else
	{
		pSurface = create_surface(info.surfaceInfo);
	}

	pSurface->refCount.fetch_add(1, std::memory_order_relaxed);

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkdevice.gpu, pSurface->handle, &pSurface->capabilities);

	VkExtent2D const extent{
		.width	= std::min(info.dimension.width, pSurface->capabilities.currentExtent.width),
		.height = std::min(info.dimension.height, pSurface->capabilities.currentExtent.height)
	};
	uint32 imageCount{ std::min(std::max(info.imageCount, pSurface->capabilities.minImageCount), pSurface->capabilities.maxImageCount) };

	VkImageUsageFlags imageUsage = vk::translate_image_usage_flags(info.imageUsage);
	VkPresentModeKHR presentationMode = vk::translate_swapchain_presentation_mode(info.presentationMode);

	bool foundPreferred = false;
	VkSurfaceFormatKHR surfaceFormat = pSurface->availableColorFormats[0];

	for (Format format : info.surfaceInfo.preferredSurfaceFormats)
	{
		VkFormat preferredFormat = vk::translate_format(format);

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
		.pQueueFamilyIndices = &vkdevice.mainQueue.familyIndex,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentationMode,
		.clipped = VK_TRUE,
		.oldSwapchain = (pOldSwapchain != nullptr) ? pOldSwapchain->handle : nullptr
	};

	VkSwapchainKHR handle = VK_NULL_HANDLE;

	CHECK_OP(vkCreateSwapchainKHR(vkdevice.device, &swapchainCreateInfo, nullptr, &handle))

	FenceInfo fenceInfo{ .initialValue = 0 };

	auto&& [id, vkswapchain] = vkdevice.gpuResourcePool.swapchains.emplace(vkdevice);

	vkswapchain.handle = handle;
	vkswapchain.pSurface = pSurface;
	vkswapchain.surfaceColorFormat = surfaceFormat;
	vkswapchain.m_colorSpace = vk::vk_to_rhi_color_space(surfaceFormat.colorSpace);

	// Update info to contain the updated values.
	if (!info.name.empty())
	{
		info.name.format("<swapchain>:{}", info.name.c_str());
		info.surfaceInfo.name.format("<surface>:{}", info.surfaceInfo.name.c_str());
		fenceInfo.name = lib::format("<fence>:{}_gpu_timeline", info.name.c_str());
	}

	info.imageCount = imageCount;
	info.dimension.width = extent.width;
	info.dimension.height = extent.height;

	vkswapchain.m_info = std::move(info);

	vkswapchain.m_gpuElapsedFrames = Fence::from(device, std::move(fenceInfo));

	size_t const maxFramesInFlight = static_cast<size_t>(device.config().maxFramesInFlight);

	vkswapchain.m_acquireSemaphore.reserve(maxFramesInFlight);
	vkswapchain.m_presentSemaphore.reserve(maxFramesInFlight);

	for (size_t i = 0; i < maxFramesInFlight; ++i)
	{
		lib::string acqSemaphoreName, presentSemaphoreName;

		if (!vkswapchain.m_info.name.empty())
		{
			acqSemaphoreName = lib::format("<semaphore>:{}_acquire_{}", vkswapchain.m_info.name.c_str(), i);
			presentSemaphoreName = lib::format("<semaphore>:{}_present_{}", vkswapchain.m_info.name.c_str(), i);
		}	

		vkswapchain.m_acquireSemaphore.push_back(Semaphore::from(device, { .name = std::move(acqSemaphoreName) }));
		vkswapchain.m_presentSemaphore.push_back(Semaphore::from(device, { .name = std::move(presentSemaphoreName) }));
	}

	vkswapchain.m_images = Image::from(vkswapchain);

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vkswapchain);
	}

	return Resource<Swapchain>{ id.to_uint64(), vkswapchain };
}

auto Swapchain::destroy(Swapchain& resource, uint64 id) -> void
{
	/*
	 * At this point, the ref count on the resource is only 1 which means ONLY the resource has a reference to itself and can be safely deleted.
	 */
	auto&& vkdevice = to_device(resource.m_device);

	resource.m_gpuElapsedFrames.destroy();

	for (auto&& acquireSemaphore : resource.m_acquireSemaphore)
	{
		acquireSemaphore.destroy();
	}

	for (auto&& presentSemaphore : resource.m_presentSemaphore)
	{
		presentSemaphore.destroy();
	}

	for (auto&& swapchainImages : resource.m_images)
	{
		swapchainImages.destroy();
	}

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();

	vkdevice.gpuResourcePool.zombies.emplace_back(cpuTimelineValue, id, vk::ResourceType::Swapchain);
}

namespace vk
{
SwapchainImpl::SwapchainImpl(DeviceImpl& device) :
	Swapchain{ device }
{}
}
}