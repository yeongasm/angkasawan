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

	bool NewGraphicsPipeline			(RenderPass& Pass);
	bool NewRenderPassFramebuffer		(RenderPass& Pass);
	bool CreateCmdPoolAndBuffers		();

	bool NewVertexBuffer				(Mesh& InMesh);

	Handle<HFramePass> GetDefaultFramebuffer() const;

	using GraphicsDriver::DestroyVertexBuffer;
	using GraphicsDriver::DestroyImage;
	using GraphicsDriver::DestroyShader;
	using GraphicsDriver::DestroyPipeline;
	using GraphicsDriver::DestroyRenderPass;
	using GraphicsDriver::DestroyFramebuffer;

	using GraphicsDriver::Clear;
	using GraphicsDriver::SwapBuffers;
	using GraphicsDriver::OnWindowResize;

private:
	//bool ValidateGraphicsPipelineCreateInfo(const RenderPass& Pass);
};

#endif // !LEARNVK_RENDERER_API_CONTEXT_H