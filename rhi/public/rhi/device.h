#pragma once
#ifndef RENDERER_RHI_DEVICE_H
#define RENDERER_RHI_DEVICE_H

#include "swapchain.h"
#include "pipeline.h"
#include "buffer.h"
#include "command_pool.h"
#include "sampler.h"

namespace rhi
{

class Device final
{
public:
	Device()	= default;
	~Device()	= default;

	RHI_API auto device_info() const -> DeviceInfo const&;
	RHI_API auto device_config() const -> DeviceConfig const&;
	RHI_API auto create_swapchain(SwapchainInfo&& info, Swapchain* oldSwapchain = nullptr) -> Swapchain;
	RHI_API auto destroy_swapchain(Swapchain& swapchain, bool destroySurface = false) -> void;
	RHI_API auto create_shader(CompiledShaderInfo&& info) -> Shader;
	RHI_API auto destroy_shader(Shader& shader) -> void;
	RHI_API auto allocate_buffer(BufferInfo&& info) -> Buffer;
	RHI_API auto release_buffer(Buffer& buffer) -> void;
	RHI_API auto create_image(ImageInfo&& info) -> Image;
	RHI_API auto destroy_image(Image& image) -> void;
	RHI_API auto create_sampler(SamplerInfo&& info) -> Sampler;
	RHI_API auto destroy_sampler(Sampler& sampler) -> void;
	RHI_API auto create_pipeline(RasterPipelineInfo&& info, PipelineShaderInfo const& pipelineShaders) -> RasterPipeline;
	RHI_API auto destroy_pipeline(RasterPipeline& pipeline) -> void;
	RHI_API auto clear_destroyed_resources() -> void;
	RHI_API auto wait_idle() -> void;
	RHI_API auto create_command_pool(CommandPoolInfo&& info) -> CommandPool;
	RHI_API auto destroy_command_pool(CommandPool& commandPool) -> void;
	RHI_API auto create_semaphore(SemaphoreInfo&& info) -> Semaphore;
	RHI_API auto destroy_semaphore(Semaphore& semaphore) -> void;
	RHI_API auto create_fence(FenceInfo&& info) -> Fence;
	RHI_API auto destroy_fence(Fence& fence) -> void;
	RHI_API auto submit(SubmitInfo const& info) -> bool;
	RHI_API auto present(PresentInfo const& info) -> bool;
private:
	APIContext* m_context;
	DeviceInfo m_info;
	friend class _Instance_;

	auto initialize(DeviceInitInfo const&) -> bool;
	auto terminate() -> void;
};

}

#endif // !RENDERER_RHI_DEVICE_H

