#pragma once
#ifndef LEARNVK_SANDBOX_SANDBOX_APP_SANDBOX_RENDER_CONFIG_H
#define LEARNVK_SANDBOX_SANDBOX_APP_SANDBOX_RENDER_CONFIG_H

#include "Library/Templates/Types.h"
#include "Engine/Interface.h"
#include "Renderer.h"
#include "Asset/Assets.h"
#include "Library/Math/Math.h"

namespace sandbox
{
	using Extent2D = WindowInfo::Extent2D;
	using Position = WindowInfo::Position;

	class RendererSetup;

	class IBasePass
	{
	public:

		Position Pos;
		Extent2D Extent;
		Ref<EngineImpl> pEngine;
		Ref<IRenderSystem> pRenderer;
		Ref<IAssetManager> pAssetManager;
		Handle<SPipeline> PipelineHnd;
		Handle<SRenderPass> RenderPassHnd;
	};

	class CColorPass : protected IBasePass
	{
	private:

		Handle<Shader> VertexShader;
		Handle<Shader> FragmentShader;
		Handle<SImage> OutputAtt;

	public:
		
		using IBasePass::Pos;
		using IBasePass::Extent;

		bool Initialize(Ref<EngineImpl> pInEngine, Ref<IRenderSystem> pInRenderSystem, Ref<IAssetManager> pInAssetManager);
		void Terminate();

		const Handle<SRenderPass> GetRenderPassHandle() const;
		const Handle<SPipeline> GetPipelineHandle() const;
		const Handle<Shader> GetShaderHandle(EShaderType Type) const;
	};

	class RendererSetup
	{
	private:

		Handle<SDescriptorPool> PoolHandle;
		Handle<SDescriptorSetLayout> SetLayoutHnd;
		Handle<SDescriptorSet> SetHnd;
		Handle<SMemoryBuffer> CameraUboHnd;
		Ref<EngineImpl> pEngine;
		Ref<IRenderSystem> pRenderer;
		Ref<IAssetManager> pAssetManager;
		CColorPass ColorPass;

	public:

		struct CameraUbo
		{
			math::mat4 ViewProj;
			math::mat4 View;
		};

		bool Initialize(Ref<EngineImpl> pInEngine, Ref<IRenderSystem> pInRenderSystem, Ref<IAssetManager> pInAssetManager);
		const Handle<SDescriptorPool> GetDescriptorPoolHandle() const;
		const Handle<SDescriptorSetLayout> GetDescriptorSetLayoutHandle() const;
		const Handle<SDescriptorSet> GetDescriptorSetHandle() const;
		const Handle<SMemoryBuffer> GetCameraUboHandle() const;
		bool Build();
		void Terminate();

		const CColorPass& GetColorPass() const;
	};

}

//struct SandboxRenderConfig
//{
//	SandboxRenderConfig();
//	~SandboxRenderConfig();
//
//	DELETE_COPY_AND_MOVE(SandboxRenderConfig)
//
//	void CreateRenderingPipeline();
//	void GenerateRenderGroups();
//	void FinalizeRenderingPipeline();
//};

#endif // !LEARNVK_SANDBOX_SANDBOX_APP_SANDBOX_RENDER_CONFIG_H