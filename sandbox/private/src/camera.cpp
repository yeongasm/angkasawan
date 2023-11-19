#include <algorithm>
#include "camera.h"

namespace sandbox
{
/**
* Require the cursor to move for at least 2 pixel before it gets registered as a mouse movement.
* Might reduce based on usability experience.
*/
Camera::DeltaEpsilon const Camera::DELTA_EPSILON = {
	.min = { 2.f, 2.f },
	.max = { 100.f, 100.f }
};

glm::vec3 const Camera::WORLD_RIGHT		= { 1.f, 0.f, 0.f };
glm::vec3 const Camera::WORLD_UP		= { 0.f, 1.f, 0.f };
glm::vec3 const Camera::WORLD_FORWARD	= { 0.f, 0.f, 1.f };

auto Camera::update_projection() -> void
{
	projection = glm::mat4{};

	float32 const fovRad = glm::radians(fov);
	float32 const aspectRatio = frameWidth / frameHeight;
	float32 const tanHalfFOV = 1.f / glm::tan(fovRad * 0.5f);

	projection[0][0] = tanHalfFOV / aspectRatio;
	/**
	* The y-basis is inverted because +y in Vulkan points downwards instead of upwards.
	*/
	projection[1][1] = -tanHalfFOV;
	projection[2][2] = far / (far - near);
	projection[2][3] = 1.f;
	/**
	* This is not the near clipping plane but rather a value to offset z-coordinates
	* to fit within the bounds of the near near clipping plane and far clipping plane.
	*/
	projection[3][2] = -(far * near) / (far - near);
}

auto Camera::update_view() -> void
{
	static glm::mat4 const IDENTITY = glm::mat4{ 1.f };

	glm::mat4 rotation = glm::mat4_cast(orientation);

	/**
	* We negate the position of the camera because conceptually, the view frustum does not move
	* and needs to be fixed at the canonical view volume's origin. To achieve the same effect
	* of moving around the world while still having the view frustum fixed at the canonical view
	* volume's origin, we instead inverse the position and translate objects in the world into
	* the view's frustum.
	*/
	glm::mat4 translation = glm::translate(IDENTITY, -position);
	view = rotation * translation;
}

auto Camera::get_up_vector() const -> glm::vec3
{
	return glm::normalize(glm::inverse(orientation) * WORLD_UP);
}

auto Camera::get_right_vector() const -> glm::vec3
{
	return glm::normalize(glm::inverse(orientation) * WORLD_RIGHT);
}

auto Camera::get_forward_vector() const -> glm::vec3
{
	return glm::normalize(glm::inverse(orientation) * WORLD_FORWARD);
}

auto Camera::zoom(float32 amount) -> void
{
	fov -= (zoomMultiplier * amount);
	fov = std::clamp(fov, 1.f, 90.f);
}

auto Camera::rotate(glm::vec3 angles, float32 timestep) -> void
{
	static glm::quat const unitQuat{ 1.f, 0.f, 0.f, 0.f };

	float32 const yaw	= angles.x * sensitivity * timestep;
	float32 const pitch	= angles.y * sensitivity * timestep;
	float32 const roll	= angles.z * sensitivity * timestep;

	rotationAngles.x += yaw;
	rotationAngles.y += pitch;
	rotationAngles.z += roll;

	rotationAngles.x = glm::mod(rotationAngles.x, 360.f);
	rotationAngles.y = glm::mod(rotationAngles.y, 360.f);
	rotationAngles.z = glm::mod(rotationAngles.z, 360.f);

	glm::quat pitchQuat	= glm::angleAxis(glm::radians(rotationAngles.y), WORLD_RIGHT);
	glm::quat yawQuat	= glm::angleAxis(glm::radians(rotationAngles.x), WORLD_UP);
	glm::quat rollQuat	= glm::angleAxis(glm::radians(rotationAngles.z), WORLD_FORWARD);

	if (glm::dot(orientation, pitchQuat) < 0.0f)
	{
		pitchQuat *= -1.0f;
	}

	if (glm::dot(orientation, yawQuat) < 0.0f)
	{
		yawQuat *= -1.0f;
	}

	orientation = rollQuat * unitQuat * pitchQuat * unitQuat * yawQuat;
	orientation = glm::normalize(orientation);
}

auto Camera::translate(glm::vec3 direction, float32 timestep) -> void
{
	position += direction * moveSpeed * timestep;
}

auto Camera::get_camera_view() const -> glm::mat4 const&
{
	return view;
}

auto Camera::get_camera_projection() const -> glm::mat4 const&
{
	return projection;
}
}