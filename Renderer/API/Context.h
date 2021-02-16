#pragma once
#ifndef LEARNVK_RENDERER_API_CONTEXT_H
#define LEARNVK_RENDERER_API_CONTEXT_H

#include "Driver.h"
#include "Assets/Shader.h"
#include "Assets/Model.h"
#include "FrameGraph/FrameGraph.h"
#include "RenderAbstracts/DrawCommand.h"
#include "RenderAbstracts/DescriptorSets.h"

/**
* RenderContext class.
* 
* Acts as a wrapper for raw Graphic API calls.
*/
class RenderContext : protected GraphicsDriver
{
public:

	RenderContext();
	~RenderContext();

	DELETE_COPY_AND_MOVE(RenderContext)

	bool InitializeContext				();
	void TerminateContext				();

	void BlitToDefault					(const IRFrameGraph& Graph);

	void BindRenderPass					(RenderPass& Pass);
	void UnbindRenderPass				(RenderPass& Pass);
	void FinalizeRenderPass				(RenderPass& Pass);
	void BindVertexAndIndexBuffer		(const DrawCommand& Command, RenderPass& Pass);
	void SubmitForRender				(const DrawCommand& Command, RenderPass& Pass);

	bool NewFrameImages					(FrameImages& Images);
	void DestroyFrameImages				(FrameImages& Images);

	bool NewGraphicsPipeline			(RenderPass& Pass);
	void DestroyGraphicsPipeline		(RenderPass& Pass, bool Release = false);
	
	bool NewRenderPassFramebuffer		(RenderPass& Pass);
	void DestroyRenderPassFramebuffer	(RenderPass& Pass, bool Release = false);

	bool NewRenderPassRenderpass		(RenderPass& Pass);
	void DestroyRenderPassRenderpass	(RenderPass& Pass);

	bool NewDescriptorPool				(DescriptorPool& Pool);
	bool NewDestriptorSetLayout			(DescriptorLayout& Layout);
	bool NewDescriptorSet				(DescriptorSet& Set);

	bool CreateCmdPoolAndBuffers		();

	bool NewVertexBuffer				(Handle<HBuffer>& Vbo, void* Data, size_t Count);
	bool NewIndexBuffer					(Handle<HBuffer>& Ebo, void* Data, size_t Count);
	bool NewUniformBuffer				(Handle<HBuffer>& Ubo, void* Data, size_t Size);

	void BindDescriptorSetInstance		(DescriptorSetInstance& Descriptor, RenderPass& Pass);
	void BindDataToDescriptorSet		(void* Data, size_t Size, DescriptorSet& Set);

	void MapDescriptorSetToBuffer		(DescriptorSet& Set);

	uint32 PadSizeToAlignedSize			(uint32 Size);

	uint32 GetCurrentFrameIndex			() const;

	//using GraphicsDriver::FreeDescriptorSetBuffer;
	using GraphicsDriver::DestroyBuffer;
	using GraphicsDriver::DestroyImage;
	//using GraphicsDriver::DestroyPipeline;
	using GraphicsDriver::DestroyDescriptorPool;
	using GraphicsDriver::DestroyDescriptorSetLayout;

	using GraphicsDriver::Clear;
	using GraphicsDriver::SwapBuffers;
	using GraphicsDriver::OnWindowResize;
};

#endif // !LEARNVK_RENDERER_API_CONTEXT_H