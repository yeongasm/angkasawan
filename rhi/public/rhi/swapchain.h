#pragma once
#ifndef RHI_SWAPCHAIN_H
#define RHI_SWAPCHAIN_H

#include "resource.h"
#include "image.h"
#include "sync.h"

namespace rhi
{
class Swapchain : public Resource
{
public:
	RHI_API Swapchain() = default;
	RHI_API ~Swapchain();

	RHI_API Swapchain(Swapchain&& rhs) noexcept;
	RHI_API auto operator=(Swapchain&& rhs) noexcept -> Swapchain&;

	// TODO(afiq):
	// Add more swapchain related functionalities.
	RHI_API auto info() const -> SwapchainInfo const&;
	RHI_API auto state() const -> SwapchainState;
	RHI_API auto num_images() const -> uint32;
	RHI_API auto current_image() const -> Image const&;
	RHI_API auto acquire_next_image() -> Image const&;
	RHI_API auto current_acquire_semaphore() const -> Semaphore const&;
	RHI_API auto current_present_semaphore() const -> Semaphore const&;
	RHI_API auto cpu_frame_count() const -> uint64;
	RHI_API auto gpu_frame_count() const -> uint64;
	RHI_API auto get_gpu_fence() const -> Fence const&;
	RHI_API auto image_format() const -> ImageFormat;
private:
	friend struct APIContext;

	static Image const NULL_SWAPCHAIN_IMAGE;

	SwapchainInfo m_info;
	SwapchainState m_state = SwapchainState::Error;
	lib::array<Image> m_images;
	Fence m_gpu_elapsed_frames;
	lib::array<Semaphore> m_acquire_semaphore;
	lib::array<Semaphore> m_present_semaphore;
	uint64 m_cpu_elapsed_frames;	// This variable is monotonically incremeneted and tracks the swapchain's total elapsed frame count.
	uint32 m_frame_index;			// m_frame_index tells us the current frame the swapchain is in. Value is always between 0 < m_frame_index < frames_in_flight.
	uint32 m_next_image_index;		// This variable is updated by the graphics API that tells us which image index to use next.

	Swapchain(
		SwapchainInfo&& info,
		APIContext* context,
		void* data,
		resource_type type_id
	);
};
}

#endif // !RHI_SWAPCHAIN_H
