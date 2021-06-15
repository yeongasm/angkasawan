#include "SandboxApp.h"
#include "Renderer.h"
#include "Importer/Importer.h"
#include "SubSystem/Time/ScopedTimer.h"

namespace sandbox
{

	Handle<Model> zeldaModel;

	SandboxApp::SandboxApp() :
		AssetManager(ao::FetchEngineCtx()),
		Setup(),
		pRenderer(),
		pCamera()
	{}

	SandboxApp::~SandboxApp() {}

	void SandboxApp::Initialize()
	{
		EngineImpl& engine = ao::FetchEngineCtx();
		IRenderSystem& renderer = ao::FetchRenderSystem();
		pRenderer = &renderer;
		pEngine = &engine;

		if (!Setup.Initialize(&engine, &renderer, &AssetManager))
		{
			engine.State = AppState::Exit;
			return;
		}

		Handle<ISystem> camHnd;
		CameraSystem* pTemp = nullptr;
		pTemp = reinterpret_cast<CameraSystem*>(
			engine.AllocateAndRegisterSystem(
				sizeof(CameraSystem), 
				System_Game_Type, 
				&camHnd
			)
		);
		new (pTemp) CameraSystem(engine, renderer, Setup, camHnd);
		pCamera = pTemp;

		pCamera->OnInit();
		pCamera->MouseCallback = [](CameraSystem::CallbackArgs& Args) {
			static bool firstMouse = true;

			enum CameraMouseModes : uint32
			{
				Camera_Mouse_Mode_NoMode = 0,
				Camera_Mouse_Mode_MoveFBAndRotateLR = 1,
				Camera_Mouse_Mode_RotateOnHold = 2
			};

			uint32 mode = Camera_Mouse_Mode_NoMode;
			IOSystem& io = Args.Engine.GetIO();

			const float32 timestep = Args.Timestep;

			float32 horizontalRange = Args.Camera.GetWidth() - io.MousePos.x;
			float32 verticalRange = Args.Camera.GetHeight() - io.MousePos.y;
			vec2 viewportOffset(0.f);
			vec2 captMousePos = Args.Camera.GetCapturedMousePos();

			Args.Camera.Zoom(io.MouseWheel);

			if (io.IsMouseHeld(Io_MouseButton_Right))
			{
				mode = Camera_Mouse_Mode_RotateOnHold;
			}

			if (io.IsMouseHeld(Io_MouseButton_Left))
			{
				mode = Camera_Mouse_Mode_MoveFBAndRotateLR;
			}

			if (!mode)
			{
				if (!firstMouse)
				{
					Args.Engine.ShowCursor();
					Args.Engine.SetMousePosition(captMousePos.x, captMousePos.y);
					Args.Camera.SetCapturedMousePos(vec2(0.f));
					firstMouse = true;
				}

				return;
			}

			if (mode && firstMouse)
			{
				const vec2 originPos = Args.Camera.GetOriginPosition();
				vec2 cameraCentre = vec2((originPos.x + Args.Camera.GetWidth()) * 0.5f, (originPos.y + Args.Camera.GetHeight()) * 0.5f);

				Args.Camera.SetCapturedMousePos(io.MousePos);

				Args.Engine.SetMousePosition(cameraCentre.x, cameraCentre.y);
				Args.Engine.ShowCursor(false);

				Args.Camera.SetLastMousePos(io.MousePos);
				firstMouse = false;
			}

			Args.Camera.SetMouseOffsetDelta(io.MousePos - Args.Camera.GetLastMousePos());;
			Args.Camera.SetLastMousePos(io.MousePos);

			const vec2 delta = Args.Camera.GetMouseOffsetDelta();

			switch (mode)
			{
			case Camera_Mouse_Mode_MoveFBAndRotateLR:
				if (delta.y > 0.0f) { Args.Camera.Translate(Args.Camera.GetForwardVector(), timestep); }
				if (delta.y < 0.0f) { Args.Camera.Translate(-Args.Camera.GetForwardVector(), timestep); }
				Args.Camera.Rotate(vec3(delta.x, 0.0f, 0.0f), timestep);
				break;
			case Camera_Mouse_Mode_RotateOnHold:
				Args.Camera.Rotate(vec3(delta.x, delta.y, 0.0f), timestep);
				break;
			default:
				break;
			}
		};

		pCamera->KeyCallback = [](CameraSystem::CallbackArgs& Args) {

			IOSystem& io = Args.Engine.GetIO();
			const float32 timestep = Args.Timestep;
			const vec3 up = Args.Camera.GetUpVector();
			const vec3 forward = Args.Camera.GetForwardVector();
			const vec3 right = Args.Camera.GetRightVector();

			if (io.IsKeyHeld(Io_Key_Q))
			{
				Args.Camera.Translate(up, timestep);
			}

			if (io.IsKeyHeld(Io_Key_W))
			{
				Args.Camera.Translate(-forward, timestep);
			}

			if (io.IsKeyHeld(Io_Key_E))
			{
				Args.Camera.Translate(-up, timestep);
			}

			if (io.IsKeyHeld(Io_Key_A))
			{
				Args.Camera.Translate(right, timestep);
			}

			if (io.IsKeyHeld(Io_Key_S))
			{
				Args.Camera.Translate(forward, timestep);
			}

			if (io.IsKeyHeld(Io_Key_D))
			{
				Args.Camera.Translate(-right, timestep);
			}
		};

		if (!Setup.Build())
		{
			VKT_ASSERT(false && "Failed to build rendering pipeline for application");
			engine.State = AppState::Exit;
			return;
		}

		IStagingManager& staging = renderer.GetStagingManager();
		ModelImporter importer;
		zeldaModel = importer.ImportModelFromPath("Data/Models/zelda_-_breath_of_the_wild/scene.gltf", &AssetManager);
		//zeldaModel = importer.ImportModelFromPath("Data/Models/sponza/Sponza.gltf", &AssetManager);
		Ref<Model> pZelda = AssetManager.GetModelWithHandle(zeldaModel);
		
		uint32 previousVertexOffset = 0;
		uint32 previousIndexOffset = 0;

		for (Mesh& mesh : pZelda->Meshes)
		{
			VertexInformation vertInfo = {};
			vertInfo.VertexOffset = previousVertexOffset;
			vertInfo.NumVertices = static_cast<uint32>(mesh.Vertices.Length());

			IndexInformation indexInfo = {};
			indexInfo.IndexOffset = previousIndexOffset;
			indexInfo.NumIndices = static_cast<uint32>(mesh.Indices.Length());

			previousVertexOffset += vertInfo.NumVertices;
			previousIndexOffset += indexInfo.NumIndices;

			pZelda->MaxVertices = previousVertexOffset;
			pZelda->MaxIndices = previousIndexOffset;
			pZelda->NumDrawables++;

			pZelda->VertexInformations.Push(Move(vertInfo));
			pZelda->IndexInformation.Push(Move(indexInfo));

			staging.StageVertexData(mesh.Vertices.First(), mesh.Vertices.Length() * sizeof(Vertex));
			staging.StageIndexData(mesh.Indices.First(), mesh.Indices.Length() * sizeof(uint32));

			mesh.Vertices.Release();
			mesh.Indices.Release();
		}
		staging.Upload();
	}

	/**
	* NOTE(Ygsm):
	* Systems will be responsible to update their own descriptor sets.
	* E.g, Camera component would be responsible to update the projection and view matrices for uniforms.
	*/
	void SandboxApp::Run(float32 Timestep)
	{
		static bool firstFrame = true;
		if (pEngine->HasWindowSizeChanged() && !firstFrame)
		{
			HandleWindowResize();
		}
		//if (pEngine->HasWindowSizeChanged())
		//{
		//	const WindowInfo::Extent2D extent = pEngine->GetWindowInformation().Extent;
		//	pCamera->SetWidth(static_cast<float32>(extent.Width));
		//	pCamera->SetHeight(static_cast<float32>(extent.Height));
		//	pRenderer->GetFrameGraph().SetOutputExtent(extent.Width, extent.Height);
		//}

		pRenderer->BindPipeline(
			Setup.GetColorPass().GetPipelineHandle(),
			Setup.GetColorPass().GetRenderPassHandle()
		);
		pRenderer->BindDescriptorSet(
			Setup.GetDescriptorSetHandle(),
			Setup.GetColorPass().GetRenderPassHandle()
		);

		Ref<Model> pZelda = AssetManager.GetModelWithHandle(zeldaModel);
		VKT_ASSERT(pZelda);

		math::mat4 transform(1.0f);
		math::Scale(transform, math::vec3(0.075f));
		//math::Rotate(transform, pEngine->Clock.FElapsedTime(), math::vec3(0.f, 1.f, 0.f));

		DrawInfo info = {};
		info.Id = static_cast<uint32>(zeldaModel);
		info.DrawableCount = pZelda->NumDrawables;
		info.Instanced = true;
		info.pVertexInformation = pZelda->VertexInformations.First();
		info.pIndexInformation = pZelda->IndexInformation.First();
		info.Transform = transform;
		info.Renderpass = Setup.GetColorPass().GetRenderPassHandle();

		pRenderer->Draw(info);
		firstFrame = false;
	}

	void SandboxApp::Terminate()
	{
		pCamera->OnTerminate();
		Setup.Terminate();
	}

	void SandboxApp::HandleWindowResize()
	{
		const Extent2D wndExtent = pEngine->GetWindowInformation().Extent;
		IFrameGraph& frameGraph = pRenderer->GetFrameGraph();
		pCamera->SetWidth(static_cast<float32>(wndExtent.Width));
		pCamera->SetHeight(static_cast<float32>(wndExtent.Height));
		frameGraph.SetOutputExtent(wndExtent.Width, wndExtent.Height);
		frameGraph.SetRenderPassExtent(
			Setup.GetColorPass().GetRenderPassHandle(),
			wndExtent
		);
		frameGraph.OnWindowResize();
	}
}