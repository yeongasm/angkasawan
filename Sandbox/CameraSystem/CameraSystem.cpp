#include "CameraSystem.h"
#include "../SandboxApp/SandboxRenderConfig.h"
#include "Renderer.h"

namespace sandbox
{

	Handle<ISystem> g_CameraSystemHandle;

	CameraSystem::CameraSystem(
		EngineImpl& InEngine, 
		IRenderSystem& InRenderer, 
		RendererSetup& InRenderSetup,
		Handle<ISystem> Hnd
	) :
		SystemInterface(),
		CameraBase(),
		Engine(InEngine),
		Renderer(InRenderer),
		Setup(InRenderSetup),
		CaptMousePos(0.0f),
		LastMousePos(0.0f),
		Offsets(0.0f),
		OriginPos(0.0f),
		State(),
		ProjType(Projection_Mode_None),
		Hnd(Hnd),
		KeyCallback{},
		MouseCallback{},
		CachedMouseDelta{}
	{}

	CameraSystem::~CameraSystem() {}

	void CameraSystem::OnInit()
	{
		g_CameraSystemHandle = Hnd;
		Rotate(vec3(45.f, 0.f, 0.0f), Engine.Clock.FTimestep());
		SetCameraProjection(Projection_Mode_Perspective);
		SetNear(0.1f);
		SetFar(1000.0f);
		SetFieldOfView(45.0f);
		SetMoveSpeed(30.0f);
		SetSensitivity(2.0f);
		SetPosition({ 0.0f, -5.0f, -30.0f });
		Rotate(vec3(0.0f, -10.0f, 0.0f), 1.0f / 60.0f);
		SetWidth(800.0f);
		SetHeight(600.0f);
		SetState(Camera_State_IsDirty);
	}

	void CameraSystem::OnUpdate()
	{
		// Need to do this in the future.
		//if (!math::InBounds(io.MousePos, { 0.0f, 0.0f }, { wndWidth, wndHeight })) { return; }

		CallbackFuncArgs args = { Engine, *this, Engine.Clock.FTimestep() };

		if (Engine.IsWindowFocused())
		{
			MouseCallback(args);
			KeyCallback(args);
		}

		if (CheckState(Camera_State_IsDirty))
		{
			UpdateProjection(true);
			UpdateView();
		}

		CameraUbo ubo;
		ubo.ViewProj = Projection * View;
		ubo.View = View;
		
		Renderer.DescriptorSetUpdateDataAtBinding(
			Setup.GetDescriptorSetHandle(),
			0,
			&ubo,
			sizeof(CameraUbo)
		);

		ResetState(Camera_State_IsDirty);
	}

	void CameraSystem::OnTerminate()
	{}

	void CameraSystem::SetCameraProjection(ProjectionMode Mode)
	{
		ProjType = Mode;
	}

	void CameraSystem::CacheMouseDelta(vec2 Pos)
	{
		static uint32 index = 0;
		CachedMouseDelta[index] = Pos;
		index = ++index % 8;
	}

	vec2 CameraSystem::GetMouseDeltaAverage()
	{
		vec2 total{ 0.0f };
		for (uint32 i = 0; i < 8; i++)
		{
			total += CachedMouseDelta[i];
		}
		return total / 8;
	}

	void CameraSystem::ClearMouseDragDeltaCache()
	{
		constexpr size_t count = sizeof(math::vec2) * 8;
		astl::IMemory::Memzero(&CachedMouseDelta, count);
	}

	void CameraSystem::SetWidth(float32 Value)
	{
		Width = Value;
	}

	void CameraSystem::SetHeight(float32 Value)
	{
		Height = Value;
	}

	void CameraSystem::SetNear(float32 Value)
	{
		Near = Value;
	}

	void CameraSystem::SetFar(float32 Value)
	{
		Far = Value;
	}

	void CameraSystem::SetFieldOfView(float32 Value)
	{
		Fov = Value;
	}

	void CameraSystem::SetMoveSpeed(float32 Value)
	{
		MoveSpeed = Value;
	}

	void CameraSystem::SetSensitivity(float32 Value)
	{
		Sensitivity = Value;
	}

	void CameraSystem::SetPosition(vec3 Value)
	{
		Position = Value;
	}

	void CameraSystem::SetOriginPosition(vec2 Value)
	{
		OriginPos = Value;
	}

	void CameraSystem::SetCapturedMousePos(vec2 Value)
	{
		CaptMousePos = Value;
	}

	void CameraSystem::SetLastMousePos(vec2 Value)
	{
		LastMousePos = Value;
	}

	void CameraSystem::SetMouseOffsetDelta(vec2 Value)
	{
		Offsets = Value;
	}

	void CameraSystem::SetState(ECameraStates State)
	{
		this->State.Set(State);
	}

	void CameraSystem::ResetState(ECameraStates State)
	{
		this->State.Remove(State);
	}

	bool CameraSystem::CheckState(ECameraStates State)
	{
		return this->State.Has(State);
	}

	float32 CameraSystem::GetWidth() const
	{
		return Width;
	}

	float32 CameraSystem::GetHeight() const
	{
		return Height;
	}

	float32 CameraSystem::GetNear() const
	{
		return Near;
	}

	float32 CameraSystem::GetFar() const
	{
		return Far;
	}

	float32 CameraSystem::GetMoveSpeed() const
	{
		return MoveSpeed;
	}

	float32 CameraSystem::GetSensitivity() const
	{
		return Sensitivity;
	}

	vec3 CameraSystem::GetPosition() const
	{
		return Position;
	}

	vec2 CameraSystem::GetOriginPosition() const
	{
		return OriginPos;
	}

	vec2 CameraSystem::GetCapturedMousePos() const
	{
		return CaptMousePos;
	}

	vec2 CameraSystem::GetLastMousePos() const
	{
		return LastMousePos;
	}

	vec2 CameraSystem::GetMouseOffsetDelta() const
	{
		return Offsets;
	}

	Handle<ISystem> CameraSystem::GetSystemHandle()
	{
		return g_CameraSystemHandle;
	}

}
