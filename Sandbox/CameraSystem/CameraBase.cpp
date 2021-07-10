#include <new>
#include <cstdio>
#include "CameraBase.h"

using vec3 = math::vec3;

const vec3 CameraGlobals::WorldRight(1.0f, 0.0f, 0.0f);
const vec3 CameraGlobals::WorldUp(0.0f, 1.0f, 0.0f);
const vec3 CameraGlobals::WorldForward(0.0f, 0.0f, -1.0f);

CameraBase::CameraBase() :
	Projection(0.0f),
	View(0.0f),
	Orientation(),
	Position(0.0f),
	Angles(0.0f),
	Fov(0.0f),
	Width(0.0f),
	Height(0.0f),
	Near(0.0f),
	Far(0.0f),
	MoveSpeed(0.0f),
	Sensitivity(0.0f)
{}

void CameraBase::UpdateProjection(bool Perspective)
{
	Projection = math::PerspectiveRH(math::Radians(Fov), Width / Height, Near, Far);
}

void CameraBase::UpdateView()
{
	mat4 rotation	 = math::Mat4Cast(Orientation);
	mat4 translation = math::Translated(mat4(1.0f), Position);
	View = rotation * translation;
}

vec3 CameraBase::GetUpVector() const
{
	return math::Normalized(math::Inversed(Orientation) *  CameraGlobals::WorldUp);
}

vec3 CameraBase::GetRightVector() const
{
	return math::Normalized(math::Inversed(Orientation) * CameraGlobals::WorldRight);
}

vec3 CameraBase::GetForwardVector() const
{
	return math::Normalized(math::Inversed(Orientation) * CameraGlobals::WorldForward);
}

CameraBase::~CameraBase() {}

CameraBase::CameraBase(const CameraBase& Rhs) 
{ 
	*this = Rhs; 
}

CameraBase::CameraBase(CameraBase&& Rhs)
{
	*this = astl::Move(Rhs);
}

CameraBase& CameraBase::operator=(const CameraBase& Rhs)
{
	if (this != &Rhs)
	{
		Projection	= Rhs.Projection;
		View		= Rhs.View;
		Orientation = Rhs.Orientation;
		Position	= Rhs.Position;
		Angles		= Rhs.Angles;
		Fov			= Rhs.Fov;
		Width		= Rhs.Width;
		Height		= Rhs.Height;
		Near		= Rhs.Near;
		Far			= Rhs.Far;
		MoveSpeed	= Rhs.MoveSpeed;
		Sensitivity = Rhs.Sensitivity;
	}

	return *this;
}

CameraBase& CameraBase::operator=(CameraBase&& Rhs)
{
	if (this != &Rhs)
	{
		Projection	= Rhs.Projection;
		View		= Rhs.View;
		Orientation = Rhs.Orientation;
		Position	= Rhs.Position;
		Angles		= Rhs.Angles;
		Fov			= Rhs.Fov;
		Width		= Rhs.Width;
		Height		= Rhs.Height;
		Near		= Rhs.Near;
		Far			= Rhs.Far;
		MoveSpeed	= Rhs.MoveSpeed;
		Sensitivity = Rhs.Sensitivity;

		new (&Rhs) CameraBase();
	}

	return *this;
}

void CameraBase::Zoom(float32 Offset)
{
	Fov -= Offset;

	if (Fov < 1.0f)
	{
		Fov = 1.0f;
	}

	if (Fov > 90.0f)
	{
		Fov = 90.0f;
	}
}

void CameraBase::Rotate(vec3 Angle, float32 Timestep)
{
	quat unitQuat;

	float32 yaw		= Angle.x * Sensitivity * Timestep;
	float32 pitch	= Angle.y * Sensitivity * Timestep;
	float32 roll	= Angle.z * Sensitivity * Timestep;

	Angles.x += yaw;
	Angles.y += pitch;
	Angles.z += roll;

	Angles.x = math::FMod(Angles.x, 360.0f);
	Angles.y = math::FMod(Angles.y, 360.0f);
	Angles.z = math::FMod(Angles.z, 360.0f);

	quat pitchQuat	= math::AngleAxis(math::Radians(Angles.y), vec3(1.0f, 0.0f, 0.0f));
	quat yawQuat	= math::AngleAxis(math::Radians(Angles.x), vec3(0.0f, 1.0f, 0.0f));
	quat rollQuat	= math::AngleAxis(math::Radians(Angles.z), vec3(0.0f, 0.0f, 1.0f));

	if (math::Dot(Orientation, pitchQuat) < 0.0f)	
	{ 
		pitchQuat *= -1.0f; 
	}

	if (math::Dot(Orientation, yawQuat) < 0.0f)		
	{ 
		yawQuat *= -1.0f; 
	}

	Orientation = rollQuat * unitQuat * pitchQuat * unitQuat * yawQuat;
	Orientation = math::Normalized(Orientation);
}

void CameraBase::Translate(vec3 Direction, float32 Timestep)
{
	Position += Direction * (MoveSpeed * Timestep);
}

const mat4& CameraBase::GetCameraView() const
{
	return View;
}

const mat4& CameraBase::GetCameraProjection() const
{
	return Projection;
}
