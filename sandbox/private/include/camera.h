#pragma once
#ifndef SANDBOX_CAMERA_H
#define SANDBOX_CAMERA_H

#include <array>

#pragma warning(push)
#pragma warning(disable:4201)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#pragma warning(pop)

#include "rhi/buffer.h"

namespace sandbox
{
struct Camera
{
	struct DeltaEpsilon
	{
		glm::vec2 min;
		glm::vec2 max;
	};
	static DeltaEpsilon const DELTA_EPSILON;

	static glm::vec3 const WORLD_RIGHT;
	static glm::vec3 const WORLD_UP;
	static glm::vec3 const WORLD_FORWARD;

	glm::mat4 projection;
	glm::mat4 view;
	glm::quat orientation = { 1.0f, glm::vec3{ 0.f } };
	glm::vec3 position;
	glm::vec3 rotationAngles;
	float32 fov;
	float32 frameWidth;
	float32 frameHeight;
	float32 near;
	float32 far;
	float32 moveSpeed;
	float32 sensitivity;
	float32 zoomMultiplier;

	auto update_projection() -> void;
	auto update_view() -> void;

	auto get_right_vector() const -> glm::vec3;
	auto get_up_vector() const -> glm::vec3;
	auto get_forward_vector() const -> glm::vec3;

	auto zoom(float32 amount) -> void;
	auto rotate(glm::vec3 angles, float32 timestep) -> void;
	auto translate(glm::vec3 direction, float32 timestep) -> void;

	auto get_camera_view() const -> glm::mat4 const&;
	auto get_camera_projection() const -> glm::mat4 const&;
};

enum class CameraMouseMode
{
	None,
	Pan_And_Dolly,
	Pan_And_Tilt
};

struct CameraState
{
	glm::vec2 capturedMousePos;
	glm::vec2 lastMousePos;
	glm::vec2 offsets;
	glm::vec2 originPos;
	uint32 dragDeltaIndex;
	CameraMouseMode mode;
	bool dirty				= false;
	bool firstMove			= true;
};
}

#endif // !SANDBOX_CAMERA_H
