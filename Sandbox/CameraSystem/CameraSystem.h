#pragma once
#ifndef LEARNVK_SANDBOX_CAMERA_SYSTEM_PERSPECTIVE_CAMERA_H
#define LEARNVK_SANDBOX_CAMERA_SYSTEM_PERSPECTIVE_CAMERA_H

#include "Library/Containers/Bitset.h"
#include "Engine/Interface.h"
#include "CameraBase.h"
#include "Renderer.h"
#include "SandboxApp/SandboxRenderConfig.h"

namespace sandbox
{

	enum ECameraStates : uint32
	{
		Camera_State_IsDirty = 0,
		Camera_State_FirstMove = 1
	};
	using ECameraStatesFlagBits = uint32;

	enum ProjectionMode : uint32
	{
		Projection_Mode_None = 0,
		Projection_Mode_Perspective = 1//,
		//Projection_Mode_Orthographic	= 2
	};

	enum CameraMouseModes : uint32
	{
		Camera_Mouse_Mode_NoMode = 0,
		Camera_Mouse_Mode_MoveFBAndRotateLR = 1,
		Camera_Mouse_Mode_RotateOnHold = 2
	};

	class CameraSystem : public SystemInterface, public CameraBase
	{
	private:

		struct CallbackFuncArgs
		{
			EngineImpl& Engine;
			CameraSystem& Camera;
			float32	Timestep;
		};

		using CallbackFunc = void(*)(CallbackFuncArgs&);
		using CameraUbo = RendererSetup::CameraUbo;

		EngineImpl& Engine;
		IRenderSystem& Renderer;
		RendererSetup& Setup;
		vec2 CaptMousePos;
		vec2 LastMousePos;
		vec2 Offsets;
		vec2 OriginPos;
		BitSet<ECameraStatesFlagBits> State;
		uint32 ProjType;
		Handle<ISystem> Hnd;
		math::vec2 CachedMouseDelta[32];

	public:

		CallbackFunc KeyCallback;
		CallbackFunc MouseCallback;

		using CallbackArgs = CallbackFuncArgs;

		CameraSystem(
			EngineImpl& InEngine,
			IRenderSystem& InRenderer,
			RendererSetup& InRenderSetup,
			Handle<ISystem> Hnd);
		~CameraSystem();

		DELETE_COPY_AND_MOVE(CameraSystem)

		void OnInit() override;
		void OnUpdate() override;
		void OnTerminate() override;
		//void OnEvent		(const OS::Event& e) override;

		void SetCameraProjection(ProjectionMode Mode);
		void CacheMouseDelta(vec2 Pos);
		vec2 GetMouseDeltaAverage();
		void ClearMouseDragDeltaCache();
		void SetWidth(float32 Value);
		void SetHeight(float32 Value);
		void SetNear(float32 Value);
		void SetFar(float32 Value);
		void SetFieldOfView(float32 Value);
		void SetMoveSpeed(float32 Value);
		void SetSensitivity(float32 Value);
		void SetPosition(vec3 Value);
		void SetOriginPosition(vec2 Value);
		void SetCapturedMousePos(vec2 Value);
		void SetLastMousePos(vec2 Value);
		void SetMouseOffsetDelta(vec2 Value);
		void SetState(ECameraStates State);
		void ResetState(ECameraStates State);
		bool CheckState(ECameraStates State);

		float32 GetWidth() const;
		float32 GetHeight() const;
		float32 GetNear() const;
		float32 GetFar() const;
		float32 GetMoveSpeed() const;
		float32 GetSensitivity() const;

		vec3	GetPosition() const;
		vec2	GetOriginPosition() const;
		vec2	GetCapturedMousePos() const;
		vec2	GetLastMousePos() const;
		vec2	GetMouseOffsetDelta() const;

		static Handle<ISystem> GetSystemHandle();

	};

}

#endif // !LEARNVK_SANDBOX_CAMERA_SYSTEM_PERSPECTIVE_CAMERA_H