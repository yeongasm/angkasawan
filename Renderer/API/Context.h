#pragma once
#ifndef LEARNVK_RENDERER_API_CONTEXT_H
#define LEARNVK_RENDERER_API_CONTEXT_H

#include "Driver.h"
#include "Assets/Shader.h"
#include "FrameGraph/FrameGraph.h"
#include "RenderAbstracts/DrawCommand.h"

// Asset building.
// Command buffer creation.
// Rendering.
class RenderContext : protected GraphicsDriver
{
public:

	RenderContext();
	~RenderContext();

	DELETE_COPY_AND_MOVE(RenderContext)

	bool InitializeContext();
	void TerminateContext();

	void Render(DrawCommand& Command);

	bool NewGraphicsPipeline			(RenderPass& Pass);
	bool RegisterRenderPassCmdBuffer	(RenderPass& Pass);
	bool CreateCmdPoolAndBuffers		();

	using GraphicsDriver::Clear;
	using GraphicsDriver::DestroyShader;
	using GraphicsDriver::DestroyPipeline;
	using GraphicsDriver::FreeCommandBuffer;
	using GraphicsDriver::SwapBuffers;
	using GraphicsDriver::OnWindowResize;

private:
	//bool ValidateGraphicsPipelineCreateInfo(const RenderPass& Pass);
};

#endif // !LEARNVK_RENDERER_API_CONTEXT_H