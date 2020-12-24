#pragma once
#ifndef LEARNVK_RENDERER_API_CONTEXT_H
#define LEARNVK_RENDERER_API_CONTEXT_H

#include "Driver.h"
#include "Assets/Shader.h"
#include "Assets/Model.h"
#include "FrameGraph/FrameGraph.h"
#include "RenderAbstracts/CommandBuffer.h"

// Asset building.
// Command buffer creation.
// Rendering.
class RenderContext : protected GraphicsDriver
{
public:

	RenderContext();
	~RenderContext();

	DELETE_COPY_AND_MOVE(RenderContext)

	bool InitializeContext				();
	void TerminateContext				();

	void BindRenderPass					(RenderPass& Pass);
	void UnbindRenderPass				(RenderPass& Pass);
	void SubmitForRender				(const Drawable& DrawObj, RenderPass& Pass);
	void FinalizeRenderPass				(RenderPass& Pass);

	bool NewFrameImages					(FrameImages& Images);
	void DestroyFrameImages				(FrameImages& Images);

	bool NewGraphicsPipeline			(RenderPass& Pass);
	
	bool NewRenderPassFramebuffer		(RenderPass& Pass);
	void DestroyRenderPassFramebuffer	(RenderPass& Pass);

	bool NewRenderPassRenderpass		(RenderPass& Pass);
	void DestroyRenderPassRenderpass	(RenderPass& Pass);

	bool CreateCmdPoolAndBuffers		();

	bool NewVertexBuffer				(Handle<HBuffer>& Vbo, void* Data, size_t Size);
	bool NewIndexBuffer					(Handle<HBuffer>& Ebo, void* Data, size_t Size);

	using GraphicsDriver::DestroyBuffer;
	using GraphicsDriver::DestroyImage;
	using GraphicsDriver::DestroyPipeline;

	using GraphicsDriver::Clear;
	using GraphicsDriver::SwapBuffers;
	using GraphicsDriver::OnWindowResize;

private:

};

#endif // !LEARNVK_RENDERER_API_CONTEXT_H