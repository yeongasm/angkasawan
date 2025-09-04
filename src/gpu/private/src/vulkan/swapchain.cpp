#include "vulkan/vk.h"
#include "vulkan/vkgpu.hpp"
#include <atomic>

namespace gpu
{
namespace vk
{
SwapchainImpl::SwapchainImpl(DeviceImpl& device) :
	vkdevice{ &device }
{}
}

auto Swapchain::info() const -> SwapchainInfo const&
{
	return m_info;
}

auto Swapchain::valid() const -> bool
{
	auto const& self = to_impl(*this);

	return self.handle != VK_NULL_HANDLE;
}

auto Swapchain::state() const -> SwapchainState
{
	return m_state;
}

auto Swapchain::num_images() const -> uint32
{
	return static_cast<uint32>(m_images.size());
}

auto Swapchain::current_image() const -> Image::handle_type
{
	if (valid())
	{
		return m_images[m_currentImageIndex];
	}
	return {};
}

auto Swapchain::current_image_index() const -> uint32
{
	return valid() ? m_currentImageIndex : 0;
}

auto Swapchain::acquire_next_image() -> Image::handle_type
{
	if (!valid())
	{
		return {};
	}

	auto const& self = to_impl(*this);

	int64 const maxFramesInFlight = static_cast<int64>(self.vkdevice->config().maxFramesInFlight);
	
	auto&& gpuTimelineFence = *self.gpu_fence();
	
	// We wait until the gpu elapsed frame count is behind our swapchain's elapsed frame count.
	gpuTimelineFence.wait_for_value(static_cast<uint64>(std::max(0ll, static_cast<int64>(m_cpuElapsedFrames) - maxFramesInFlight)));

	// Update swapchain's frame index for the next call to this function.
	uint32 const cpuElapsedFrameCount = m_cpuElapsedFrames.load(std::memory_order_acquire);
	m_acquireSemaphoreIndex = cpuElapsedFrameCount % (maxFramesInFlight + 1);

	vk::SemaphoreImpl const& vksemaphore = to_impl(*m_acquireSemaphore[m_acquireSemaphoreIndex]);

	VkResult result = vkAcquireNextImageKHR(
		self.vkdevice->device,
		self.handle,
		UINT64_MAX,
		vksemaphore.handle,
		nullptr,
		&m_currentImageIndex
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

	return m_images[static_cast<size_t>(m_currentImageIndex)];
}

auto Swapchain::current_acquire_semaphore() const -> Semaphore::handle_type
{
	return m_acquireSemaphore[m_acquireSemaphoreIndex];
}

auto Swapchain::current_present_semaphore() const -> Semaphore::handle_type
{
	return m_presentSemaphore[m_currentImageIndex];
}

auto Swapchain::cpu_frame_count() const -> uint64
{
	return m_cpuElapsedFrames.load(std::memory_order_acquire);
}

auto Swapchain::gpu_fence() const -> Fence::handle_type
{
	return m_gpuElapsedFrames;
}

auto Swapchain::image_format() const -> Format
{
	Format format = Format::Undefined;

	if (!m_images.empty())
	{
		format = (*m_images[0]).info().format;
	}

	return format;
}

auto Swapchain::resize(Extent2D dim) -> bool
{
	auto&& self = to_impl(*this);

	auto&& surface = *self.surface;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(self.vkdevice->gpu, surface.handle, &surface.capabilities);

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
		.pQueueFamilyIndices = &self.vkdevice->mainQueue.familyIndex,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentationMode,
		.clipped = VK_TRUE,
		.oldSwapchain = self.handle
	};

	if (vkCreateSwapchainKHR(self.vkdevice->device, &swapchainCreateInfo, nullptr, &self.handle) != VK_SUCCESS)
	{
		return false;
	}

	m_info.dimension.width	= extent.width;
	m_info.dimension.height = extent.height;

	uint32 maxImageCount = (self.m_info.imageCount > MAX_FRAMES_IN_FLIGHT) ? MAX_FRAMES_IN_FLIGHT : self.m_info.imageCount;

	std::array<VkImage, MAX_FRAMES_IN_FLIGHT> vkImageHandles = {};

	for (uint32 i = 0; i < maxImageCount; ++i)
	{
		vkGetSwapchainImagesKHR(self.vkdevice->device, self.handle, &maxImageCount, vkImageHandles.data());
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

		vkDestroyImageView(self.vkdevice->device, vkimage.imageView, nullptr);

		vkimage.handle = vkImageHandles[i];

		vkimage.m_info.dimension.width	= m_info.dimension.width;
		vkimage.m_info.dimension.height	= m_info.dimension.height;

		imageViewInfo.image = vkimage.handle;

		vkCreateImageView(self.vkdevice->device, &imageViewInfo, nullptr, &vkimage.imageView);

		++i;
	}

	return true;
}

auto Swapchain::from(Device& device, SwapchainInfo&& info, handle_type previousSwapchain) -> handle_type
{
	auto&& vkdevice = to_device(device);

	auto create_surface = [&vkdevice](SurfaceInfo const& surfaceInfo) -> vk::SwapchainImpl::surface_iterator
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

	vk::SwapchainImpl::surface_iterator surface = {};
	vk::SwapchainImpl* pOldSwapchain = nullptr;

	if (previousSwapchain.valid())
	{
		pOldSwapchain = &to_impl(*previousSwapchain);
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

	VkImageUsageFlags imageUsage = vk::translate_image_usage_flags(info.imageUsage);
	VkPresentModeKHR presentationMode = vk::translate_swapchain_presentation_mode(info.presentationMode);

	[[maybe_unused]] VkPresentModeKHR presentationModes[10];
	[[maybe_unused]] uint32 numPresentModes = 0u;

	vkGetPhysicalDeviceSurfacePresentModesKHR(vkdevice.gpu, surface->handle, &numPresentModes, nullptr);
	vkGetPhysicalDeviceSurfacePresentModesKHR(vkdevice.gpu, surface->handle, &numPresentModes, presentationModes);

	bool foundPreferred = false;
	VkSurfaceFormatKHR surfaceFormat = surface->availableColorFormats[0];

	for (Format format : info.surfaceInfo.preferredSurfaceFormats)
	{
		VkFormat preferredFormat = vk::translate_format(format);

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

	auto it = vkdevice.gpuResourcePool.stores.swapchains.emplace(vkdevice);
	
	vk::_ResourceMeta meta{
		.type 	= vk::detail::type_id_v<vk::SwapchainImpl>,
		.id 	= ++vkdevice.gpuResourcePool.idCounter.others
	};

	auto id = std::bit_cast<uint64>(meta);

	vkdevice.gpuResourcePool.caches.swapchain.emplace(id, it);
	vkdevice.begin_referencing(id);

	auto&& vkswapchain = *it;

	vkswapchain.handle = handle;
	vkswapchain.surface = surface;
	vkswapchain.surfaceColorFormat = surfaceFormat;

	// Update info to contain the updated values.
	if (!info.name.empty())
	{
		fenceInfo.name = fmt::format("<fence>:{}_gpu_timeline", info.name);
	}

	info.imageCount = imageCount;
	info.dimension.width = extent.width;
	info.dimension.height = extent.height;

	vkswapchain.m_info = std::move(info);

	vkswapchain.m_gpuElapsedFrames = Fence::from(device, std::move(fenceInfo));

	size_t const maxFramesInFlight = static_cast<size_t>(device.config().maxFramesInFlight);
	size_t const numImages = vkswapchain.m_info.imageCount;

	vkswapchain.m_acquireSemaphore.reserve(maxFramesInFlight + 1);
	vkswapchain.m_presentSemaphore.reserve(numImages);

	bool const swapchainHasName = !vkswapchain.m_info.name.empty();
	std::string name;

	for (size_t i = 0; i < maxFramesInFlight + 1; ++i)
	{
		if (swapchainHasName)
		{
			name = fmt::format("<semaphore>:{}_acquire_{}", vkswapchain.m_info.name, i);
		}	
		
		vkswapchain.m_acquireSemaphore.push_back(Semaphore::from(device, { .name = std::move(name) }));
	}
	
	for (size_t i = 0; i < numImages; ++i)
	{
		if (swapchainHasName)
		{
			name = fmt::format("<semaphore>:{}_present_{}", vkswapchain.m_info.name, i);
		}
		
		vkswapchain.m_presentSemaphore.push_back(Semaphore::from(device, { .name = std::move(name) }));
	}

	vkswapchain.m_images = Image::from(vkswapchain);

	if constexpr (ENABLE_GPU_RESOURCE_DEBUG_NAMES)
	{
		vkdevice.setup_debug_name(vkswapchain);
	}

	return handle_type{ vkdevice, id };
}

auto Swapchain::Deleter::operator()(Device& device, uint64 id) const -> void
{
	ASSERTION(id != std::numeric_limits<uint64>::max() && "Attempting to destroy a Swapchain with an invalid id");

	/*
	 * At this point, the ref count on the resource is only 1 which means ONLY the resource has a reference to itself and can be safely deleted.
	 */
	auto&& vkdevice = to_device(device);
	auto&& vkswapchain = *vkdevice.gpuResourcePool.caches.swapchain[id];

	vkswapchain.m_gpuElapsedFrames.destroy();

	for (auto&& acquireSemaphore : vkswapchain.m_acquireSemaphore)
	{
		acquireSemaphore.destroy();
	}

	for (auto&& presentSemaphore : vkswapchain.m_presentSemaphore)
	{
		presentSemaphore.destroy();
	}

	for (auto&& swapchainImages : vkswapchain.m_images)
	{
		swapchainImages.destroy();
	}

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();

	vkdevice.gpuResourcePool.zombies.emplace_back(
		cpuTimelineValue,
		[&vkswapchain, id](vk::DeviceImpl& device) -> void
		{
			auto surface = vkswapchain.surface;

			vkDestroySwapchainKHR(device.device, vkswapchain.handle, nullptr);
		
			auto it = device.gpuResourcePool.caches.swapchain[id];

			device.gpuResourcePool.caches.swapchain.erase(id);
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