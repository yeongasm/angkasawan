#pragma once
#ifndef LEARNVK_SANBOX_SANBOX_APP_H
#define LEARNVK_SANBOX_SANBOX_APP_H

#include "SubSystem/Game/GameApp.h"
#include "CameraSystem/CameraSystem.h"
#include "SandboxRenderConfig.h"
#include "Asset/Assets.h"
#include "Material/MaterialController.h"
#include "Text/TextController.h"

namespace sandbox
{

	class SandboxApp : public GameAppBase
	{
	public:
		SandboxApp();
		~SandboxApp();

		void Initialize();
		void Run(float32 Timestep);
		void Terminate();

	private:
		IAssetManager AssetManager;
    MaterialController MatController;
    TextController TypeWriter;
		RendererSetup Setup;
		astl::Ref<IRenderSystem> pRenderer;
		astl::Ref<EngineImpl> pEngine;
		astl::Ref<CameraSystem> pCamera;

		void HandleWindowResize();
    bool CreateMaterialDefinition(Handle<MaterialDef>& DefHnd);
	};

}

#endif // !LEARNVK_SANBOX_SANBOX_APP_H
