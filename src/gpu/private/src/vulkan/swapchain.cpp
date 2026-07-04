#include "vulkan/vkgpu.hpp"
#include <atomic>

namespace gpu
{
auto Swapchain::info() const -> SwapchainInfo const&
{
	return __self().info;
}

auto Swapchain::valid() const -> bool
{
	return m_device && m_data && __self().handle != VK_NULL_HANDLE;
}

auto Swapchain::num_images() const -> uint32
{
	return static_cast<uint32>(__self().images.size());
}

auto Swapchain::current_image() const -> Image
{
	if (valid())
	{
		auto&& self = __self();
		return self.images[self.currentImageIndex];
	}
	return {};
}

auto Swapchain::current_image_index() const -> uint32
{
	return valid() ?__self().currentImageIndex : 0;
}

auto Swapchain::acquire_next_image() -> Image
{
	if (!valid())
	{
		return {};
	}

	auto& device = __device();
	auto& self = __self();

	int64 const maxFramesInFlight = static_cast<int64>(device.config().maxFramesInFlight);
	
	auto&& gpuTimelineFence = self.gpuTimeline;
	// Update swapchain's frame index for the next call to this function.
	uint64 const cpuElapsedFrames = self.cpuTimeline.load(std::memory_order_acquire);

	// We wait until the gpu elapsed frame count is behind our swapchain's elapsed frame count.
	gpuTimelineFence.wait_for_value(static_cast<uint64>(std::max(0ll, static_cast<int64>(cpuElapsedFrames) - maxFramesInFlight)));

	self.acquireSemaphoreIndex = static_cast<uint32>(cpuElapsedFrames) % static_cast<uint32>(maxFramesInFlight + 1);


	auto const& semaphore = impl_of(self.acquireSemaphore[self.acquireSemaphoreIndex]);

	VkResult result = vkAcquireNextImageKHR(
		device.device,
		self.handle,
		UINT64_MAX,
		semaphore.handle,
		nullptr,
		&self.currentImageIndex
	);

	// Do something with this.
	switch (result)
	{
	case VK_SUCCESS:
		break;
	case VK_NOT_READY:
		break;
	case VK_TIMEOUT:
		break;
	case VK_SUBOPTIMAL_KHR:
		break;
	default:
		break;
	}

	// Increment total swapchain elapsed frame count.
	self.cpuTimeline.fetch_add(1, std::memory_order_relaxed);

	return self.images[static_cast<size_t>(self.currentImageIndex)];
}

auto Swapchain::current_acquire_semaphore() const -> Semaphore
{
	auto const& self = __self();
	return self.acquireSemaphore[self.acquireSemaphoreIndex];
}

auto Swapchain::current_present_semaphore() const -> Semaphore
{
	auto const& self = __self();
	return self.presentSemaphore[self.currentImageIndex];
}

auto Swapchain::cpu_frame_count() const -> uint64
{
	auto const& self = __self();
	return self.cpuTimeline.load(std::memory_order_acquire);
}

auto Swapchain::gpu_fence() const -> Fence
{
	auto const& self = __self();
	return self.gpuTimeline;
}

auto Swapchain::image_format() const -> Format
{
	auto const& self = __self();
	Format format = Format::Undefined;

	if (!self.images.empty())
	{
		format = self.images[self.currentImageIndex].info().format;
	}

	return format;
}

auto Swapchain::resize(Extent2D dim) -> bool
{
	auto&& self = __self();
	auto&& device = __device();
	auto&& surface = *self.surface;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.gpu, surface.handle, &surface.capabilities);

	VkExtent2D const extent{
		.width	= std::min(dim.width, surface.capabilities.currentExtent.width),
		.height = std::min(dim.height, surface.capabilities.currentExtent.height)
	};
	uint32 imageCount{ std::min(std::max(self.info.imageCount, surface.capabilities.minImageCount), surface.capabilities.maxImageCount) };

	VkImageUsageFlags imageUsage = translate_image_usage_flags(self.info.imageUsage);
	VkPresentModeKHR presentationMode = translate_swapchain_presentation_mode(self.info.presentationMode);

	bool foundPreferred = false;
	VkSurfaceFormatKHR surfaceFormat = surface.availableColorFormats[0];

	for (Format format : self.info.surfaceInfo.preferredSurfaceFormats)
	{
		VkFormat preferredFormat = translate_format(format);

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
		.pQueueFamilyIndices = &device.mainQueue.familyIndex,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentationMode,
		.clipped = VK_TRUE,
		.oldSwapchain = self.handle
	};

	if (vkCreateSwapchainKHR(device.device, &swapchainCreateInfo, nullptr, &self.handle) != VK_SUCCESS)
	{
		return false;
	}

	self.info.dimension.width	= extent.width;
	self.info.dimension.height = extent.height;

	uint32 maxImageCount = (self.info.imageCount > MAX_FRAMES_IN_FLIGHT) ? MAX_FRAMES_IN_FLIGHT : self.info.imageCount;

	std::array<VkImage, MAX_FRAMES_IN_FLIGHT> vkImageHandles = {};

	for (uint32 i = 0; i < maxImageCount; ++i)
	{
		vkGetSwapchainImagesKHR(device.device, self.handle, &maxImageCount, vkImageHandles.data());
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

	for (size_t i = 0; auto&& image : self.images)
	{
		auto&& vkimage = impl_of(image);

		vkDestroyImageView(device.device, vkimage.imageView, nullptr);

		vkimage.handle = vkImageHandles[i];

		vkimage.info.dimension.width	= self.info.dimension.width;
		vkimage.info.dimension.height	= self.info.dimension.height;

		imageViewInfo.image = vkimage.handle;

		vkCreateImageView(device.device, &imageViewInfo, nullptr, &vkimage.imageView);

		++i;
	}

	return true;
}

auto Swapchain::from(Device& device, SwapchainInfo&& info, Swapchain previousSwapchain) -> Swapchain
{
	auto&& vkdevice = static_cast<DeviceImpl&>(device);

	auto create_surface = [&vkdevice](SurfaceInfo const& surfaceInfo) -> SwapchainImpl::surface_iterator
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
			return {};
		}
#elif __linux__
#endif

		auto it = vkdevice.gpuResourcePool.stores.surfaces.emplace();

		auto&& vksurface = *it;

		vksurface.handle = handle;

		uint32 count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(vkdevice.gpu, handle, &count, nullptr);

		vksurface.availableColorFormats.reserve(static_cast<size_t>(count));
		vksurface.availableColorFormats.resize(static_cast<size_t>(count));

		vkGetPhysicalDeviceSurfaceFormatsKHR(vkdevice.gpu, handle, &count, vksurface.availableColorFormats.data());

		/**
		* A resource will always have a count of 1 because it references itself.
		*/
		vksurface.refCount.fetch_add(1, std::memory_order_relaxed);

		return it;
	};

	if (!info.surfaceInfo.instance || !info.surfaceInfo.window)
	{
		return {};
	}

	SwapchainImpl::surface_iterator surface = {};
	SwapchainImpl* pOldSwapchain = nullptr;

	if (previousSwapchain.valid())
	{
		pOldSwapchain = &impl_of(previousSwapchain);
		surface = pOldSwapchain->surface;
	}
	else
	{
		surface = create_surface(info.surfaceInfo);
	}

	surface->refCount.fetch_add(1, std::memory_order_relaxed);

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkdevice.gpu, surface->handle, &surface->capabilities);

	VkExtent2D const extent{
		.width	= std::min(info.dimension.width, surface->capabilities.currentExtent.width),
		.height = std::min(info.dimension.height, surface->capabilities.currentExtent.height)
	};
	uint32 imageCount{ std::min(std::max(info.imageCount, surface->capabilities.minImageCount), surface->capabilities.maxImageCount) };

	VkImageUsageFlags imageUsage = translate_image_usage_flags(info.imageUsage);
	VkPresentModeKHR presentationMode = translate_swapchain_presentation_mode(info.presentationMode);

	[[maybe_unused]] VkPresentModeKHR presentationModes[10];
	[[maybe_unused]] uint32 numPresentModes = 0u;

	vkGetPhysicalDeviceSurfacePresentModesKHR(vkdevice.gpu, surface->handle, &numPresentModes, nullptr);
	vkGetPhysicalDeviceSurfacePresentModesKHR(vkdevice.gpu, surface->handle, &numPresentModes, presentationModes);

	bool foundPreferred = false;
	VkSurfaceFormatKHR surfaceFormat = surface->availableColorFormats[0];

	for (Format format : info.surfaceInfo.preferredSurfaceFormats)
	{
		VkFormat preferredFormat = translate_format(format);

		if (foundPreferred)
		{
			break;
		}

		for (VkSurfaceFormatKHR colorFormat : surface->availableColorFormats)
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
		.surface = surface->handle,
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

	// Update info to contain the updated values.
	if (!info.name.empty())
	{
		fenceInfo.name = fmt::format("<fence>:{}_gpu_timeline", info.name);
	}

	info.imageCount = imageCount;
	info.dimension.width = extent.width;
	info.dimension.height = extent.height;

	auto&& vkswapchain = *vkdevice.gpuResourcePool.stores.swapchains.emplace();

	vkswapchain.handle = handle;
	vkswapchain.surface = surface;
	vkswapchain.surfaceColorFormat = surfaceFormat;
	vkswapchain.info = std::move(info);
	vkswapchain.gpuTimeline = Fence::from(device, std::move(fenceInfo));

	size_t const maxFramesInFlight = static_cast<size_t>(device.config().maxFramesInFlight);
	size_t const numImages = vkswapchain.info.imageCount;

	vkswapchain.acquireSemaphore.reserve(maxFramesInFlight + 1);
	vkswapchain.presentSemaphore.reserve(numImages);

	bool const swapchainHasName = !vkswapchain.info.name.empty();
	
	std::string name;
	for (size_t i = 0; i < maxFramesInFlight + 1; ++i)
	{
		if (swapchainHasName)
		{
			name = fmt::format("<semaphore>:{}_acquire_{}", vkswapchain.info.name, i);
		}	
		
		vkswapchain.acquireSemaphore.push_back(Semaphore::from(device, { .name = std::move(name) }));
	}
	
	for (size_t i = 0; i < numImages; ++i)
	{
		if (swapchainHasName)
		{
			name = fmt::format("<semaphore>:{}_present_{}", vkswapchain.info.name, i);
		}
		
		vkswapchain.presentSemaphore.push_back(Semaphore::from(device, { .name = std::move(name) }));
	}

	vkswapchain.images = Image::from(vkdevice, vkswapchain);

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vkswapchain);
	}

	return Swapchain{ &vkswapchain, &vkdevice };
}

auto Swapchain::zombify(Device& dvc, ref_counted_base& resource) -> void
{
	auto&& device = static_cast<DeviceImpl&>(dvc);
	auto&& swapchain = static_cast<SwapchainImpl&>(resource);

	swapchain.gpuTimeline.destroy();

	for (auto&& acquireSemaphore : swapchain.acquireSemaphore)
	{
		acquireSemaphore.destroy();
	}

	for (auto&& presentSemaphore : swapchain.presentSemaphore)
	{
		presentSemaphore.destroy();
	}

	for (auto&& swapchainImages : swapchain.images)
	{
		swapchainImages.destroy();
	}

	std::lock_guard const lock{ device.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = device.cpu_timeline();

	device.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&swapchain](DeviceImpl& device) -> void
		{
			auto surface = swapchain.surface;
			vkDestroySwapchainKHR(device.device, swapchain.handle, nullptr);
			
			auto it = device.gpuResourcePool.stores.swapchains.get_iterator(&swapchain);
			device.gpuResourcePool.stores.swapchains.erase(it);

			/**
			* fetch_sub returns the previously held value of the atomic variable prior to the operation.
			* That means, if it returns 2, the current value of the atomic refCount is 1 and the only reference to the surface is itself.
			*/
			if (surface->refCount.fetch_sub(1, std::memory_order_acq_rel) == 2)
			{
				vkDestroySurfaceKHR(device.instance, surface->handle, nullptr);
				device.gpuResourcePool.stores.surfaces.erase(surface);
			}
		}
	);
}
}