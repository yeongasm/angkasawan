#pragma once
#ifndef LEARNVK_SANDBOX_CAMERA_SYSTEM_CAMERA_BASE
#define LEARNVK_SANDBOX_CAMERA_SYSTEM_CAMERA_BASE

#include "Library/Math/Math.h"
#include "Library/Templates/Types.h"
#include "Library/Templates/Templates.h"

/**
* 
* Coordinate system info:
* Left handed system.
* 
* Right		- Position X-axis
* Up		- Positive Y-axis
* Forward	- Positive Z-axis
*/

using vec2 = math::vec2;
using vec3 = math::vec3;
using vec4 = math::vec4;
using mat3 = math::mat3;
using mat4 = math::mat4;
using quat = math::quat;

struct CameraGlobals
{
	static const vec3 WorldRight;
	static const vec3 WorldUp;
	static const vec3 WorldForward;
};


/**
* Camera uses the Left-hand coordinate system
*/
class CameraBase
{
protected:

	mat4	Projection;
	mat4	View;
	quat	Orientation;
	vec3	Position;
	vec3	Angles;
	//vec3 UpVector;
	//vec3 RightVector;
	//vec3 ForwardVector;
	float32 Fov;
	float32 Width;
	float32 Height;
	float32 Near;
	float32 Far;
	float32 MoveSpeed;
	float32 Sensitivity;

	void UpdateProjection	(bool Perspective = true);
	void UpdateView			();

public:

	CameraBase();
	~CameraBase();

	CameraBase(const CameraBase& Rhs);
	CameraBase(CameraBase&& Rhs);

	CameraBase& operator=(const CameraBase& Rhs);
	CameraBase& operator=(CameraBase&& Rhs);

	vec3 GetUpVector				() const;
	vec3 GetRightVector				() const;
	vec3 GetForwardVector			() const;

	void Zoom						(float32 Offset);
	void Rotate						(vec3 Angle, float32 Timestep);
	void Translate					(vec3 Direction, float32 Timestep);

	const mat4& GetCameraView		()	const;
	const mat4& GetCameraProjection	()	const;

};

#endif // !LEARNVK_SANDBOX_CAMERA_SYSTEM_CAMERA_BASE