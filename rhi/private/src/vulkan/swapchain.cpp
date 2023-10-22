#include "swapchain.h"
#include "vulkan/vk_device.h"

namespace rhi
{

Image const Swapchain::NULL_SWAPCHAIN_IMAGE = {};

Swapchain::~Swapchain()
{
	for (Image& swapchainImage : m_images)
	{
		vulkan::Image& image = swapchainImage.as<vulkan::Image>();
		lib::release_memory(&image);
	}
	m_images.release();
}

Swapchain::Swapchain(
	SwapchainInfo&& info,
	APIContext* context,
	void* data,
	resource_type type_id
) :
	Resource{ context, data, type_id },
	m_info{ std::move(info) },
	m_state{ SwapchainState::Ok },
	m_images{},
	m_gpu_elapsed_frames{},
	m_acquire_semaphore{},
	m_present_semaphore{},
	m_cpu_elapsed_frames{},
	m_frame_index{},
	m_next_image_index{}
{
	vulkan::Swapchain& swapchainData = as<vulkan::Swapchain>();
	m_images.reserve(static_cast<size_t>(m_info.imageCount));
	
	m_gpu_elapsed_frames = m_context->create_timeline_semaphore({ .name = "gpu_frame_timeline", .initialValue = 0 });

	uint32 maxFramesInFlight = m_context->config.maxFramesInFlight;
	m_acquire_semaphore.reserve(static_cast<size_t>(maxFramesInFlight));
	m_present_semaphore.reserve(static_cast<size_t>(maxFramesInFlight));

	for (uint32 i = 0; i < maxFramesInFlight; ++i)
	{
		auto acquireSemaphore = m_context->create_binary_semaphore({ .name = lib::format("acquire_{}", i) });
		auto presentSemaphore = m_context->create_binary_semaphore({ .name = lib::format("present_{}", i) });
		m_acquire_semaphore.push_back(std::move(acquireSemaphore));
		m_present_semaphore.push_back(std::move(presentSemaphore));
	}

	for (uint32 i = 0; i < m_info.imageCount; ++i)
	{
		ImageInfo swapchainImageInfo{
			.name = lib::format("<swapchain_image_{}>:{}", i, m_info.name.c_str()),
			.type = ImageType::Image_2D,
			.format = static_cast<Format>(swapchainData.surfaceColorFormat.format),
			.samples = SampleCount::Sample_Count_1,
			.tiling = ImageTiling::Optimal,
			.imageUsage = m_info.imageUsage,
			.memoryUsage = MemoryUsage::Dedicated,
			.dimension = { .width = m_info.dimension.width, .height = m_info.dimension.height, .depth = 0u },
			.clearValue = { .color = { .f32 = { 0.f, 0.f, 0.f, 1.f } } },
			.mipLevel = 0
		};
		// TODO(afiq):
		// Include image creation as part of the device.
		vulkan::Image* pImage = reinterpret_cast<vulkan::Image*>(lib::allocate_memory({ .size = sizeof(vulkan::Image) }));
		new (pImage) vulkan::Image{ swapchainData.images[i], swapchainData.imageViews[i], {}, true };
		Image swapchainImage{
			std::move(swapchainImageInfo),
			m_context,
			pImage,
			resource_type_id_v<vulkan::Image>
		};
		m_images.push_back(std::move(swapchainImage));
	}
	m_context->setup_debug_name(*this);
}

Swapchain::Swapchain(Swapchain&& rhs) noexcept
{
	*this = std::move(rhs);
}

auto Swapchain::operator=(Swapchain&& rhs) noexcept -> Swapchain&
{
	if (this != &rhs)
	{
		m_info = std::move(rhs.m_info);
		m_state = rhs.m_state;
		m_images = std::move(rhs.m_images);
		m_gpu_elapsed_frames = std::move(rhs.m_gpu_elapsed_frames);
		m_acquire_semaphore = std::move(rhs.m_acquire_semaphore);
		m_present_semaphore = std::move(rhs.m_present_semaphore);
		m_cpu_elapsed_frames = std::move(rhs.m_cpu_elapsed_frames);
		m_frame_index = std::move(rhs.m_frame_index);
		m_next_image_index = std::move(rhs.m_next_image_index);
		Resource::operator=(std::move(rhs));
		new (&rhs) Swapchain{};
	}
	return *this;
}

auto Swapchain::info() const -> SwapchainInfo const&
{
	return m_info;
}

auto Swapchain::state() const -> SwapchainState
{
	return m_state;
}

auto Swapchain::num_images() const -> uint32
{
	return static_cast<uint32>(m_images.size());
}

auto Swapchain::current_image() const -> Image const&
{
	return m_images[m_next_image_index];
}

auto Swapchain::acquire_next_image() -> Image const&
{
	if (!valid())
	{
		return NULL_SWAPCHAIN_IMAGE;
	}
	[[maybe_unused]] int64 const maxFramesInFlight = static_cast<int64>(m_context->config.maxFramesInFlight);
	// We wait until the gpu elapsed frame count is behind our swapchain's elapsed frame count.
	[[maybe_unused]] uint64 currentValue = m_gpu_elapsed_frames.value();
	m_gpu_elapsed_frames.wait_for_value(static_cast<uint64>(std::max(0ll, static_cast<int64>(m_cpu_elapsed_frames))));

	// Update swapchain's frame index for the next call to this function.
	m_frame_index = (m_frame_index + 1) % maxFramesInFlight;

	vulkan::Swapchain& swapchainResource = as<vulkan::Swapchain>();
	vulkan::Semaphore& semaphore = m_acquire_semaphore[m_frame_index].as<vulkan::Semaphore>();

	VkResult result = vkAcquireNextImageKHR(
		m_context->device, 
		swapchainResource.handle, 
		UINT64_MAX, 
		semaphore.handle, 
		nullptr, 
		&m_next_image_index
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
	m_cpu_elapsed_frames += 1;

	return m_images[static_cast<size_t>(m_next_image_index)];
}

auto Swapchain::current_acquire_semaphore() const -> Semaphore const&
{
	return m_acquire_semaphore[m_frame_index];
}

auto Swapchain::current_present_semaphore() const -> Semaphore const&
{
	return m_present_semaphore[m_frame_index];
}

auto Swapchain::cpu_frame_count() const -> uint64
{
	return m_cpu_elapsed_frames;
}

auto Swapchain::gpu_frame_count() const -> uint64
{
	return m_gpu_elapsed_frames.value();
}

auto Swapchain::get_gpu_fence() const -> Fence const&
{
	return m_gpu_elapsed_frames;
}

auto Swapchain::image_format() const -> Format
{
	Format format = Format::Undefined;
	if (m_images.size())
	{
		format = m_images[0].info().format;
	}
	return format;
}

}