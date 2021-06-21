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
			uint32 mode = Camera_Mouse_Mode_NoMode;
			IOSystem& io = Args.Engine.GetIO();
			const vec2 originPos = Args.Camera.GetOriginPosition();
			vec2 mousePos = io.MousePos;
			vec2 cameraCentre = vec2((originPos.x + Args.Camera.GetWidth()) * 0.5f, (originPos.y + Args.Camera.GetHeight()) * 0.5f);

			const float32 timestep = Args.Timestep;

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
				if (!Args.Camera.CheckState(Camera_State_FirstMove))
				{
					const vec2 captMousePos = Args.Camera.GetCapturedMousePos();
					Args.Engine.ShowCursor();
					Args.Engine.SetMousePosition(captMousePos.x, captMousePos.y);
					Args.Camera.SetCapturedMousePos(vec2(0.f));
					Args.Camera.SetState(Camera_State_FirstMove);
					Args.Camera.ClearMouseDragDeltaCache();
				}
				return;
			}

			if (Args.Camera.CheckState(Camera_State_FirstMove))
			{
				Args.Camera.SetCapturedMousePos(mousePos);
				Args.Camera.ResetState(Camera_State_FirstMove);
				mousePos = cameraCentre;
			}

			Args.Engine.ShowCursor(false);
			Args.Engine.SetMousePosition(cameraCentre.x, cameraCentre.y);

			vec2 delta = mousePos - cameraCentre;
			Args.Camera.CacheMouseDelta(delta);
			delta = Args.Camera.GetMouseDeltaAverage();

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

			Args.Camera.SetState(Camera_State_IsDirty);
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

		DrawInfo info = {};
		info.Id = static_cast<uint32>(zeldaModel);
		info.DrawableCount = pZelda->NumDrawables;
		info.Instanced = true;
		info.pVertexInformation = pZelda->VertexInformations.First();
		info.pIndexInformation = pZelda->IndexInformation.First();
		info.Renderpass = Setup.GetColorPass().GetRenderPassHandle();

		math::mat4 transform(1.0f);
		math::Scale(transform, math::vec3(0.075f));

		info.Transform = transform;
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
		pCamera->SetState(Camera_State_IsDirty);
		frameGraph.SetOutputExtent(wndExtent.Width, wndExtent.Height);
		frameGraph.SetRenderPassExtent(
			Setup.GetColorPass().GetRenderPassHandle(),
			wndExtent
		);
		frameGraph.OnWindowResize();
	}
}
