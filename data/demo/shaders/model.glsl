#version 460 core

#include <forge.glsl>

layout (buffer_reference, scalar) readonly buffer buffer_CameraProjView
{
	mat4 proj;
	mat4 view;
};

layout (buffer_reference, scalar) readonly buffer buffer_WorldTransform
{
	mat4 data;
};

layout (push_constant, scalar) uniform PushConstant
{
	uint i_camProjView;
	uint i_transform;
	uint i_baseColorMap;
	// buffer_CameraPojView cameraViewProj;
	// buffer_WorldTransform transform;
} _constants;

#if defined(VERTEX_SHADER)

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_uv;

layout (location = 0) out vec2 out_uv;

void main()
{
	//ProjectionViewTransform projView = deref(ProjectionViewTransform, _constants.proj_view_i);
	//WorldTransform world = deref(WorldTransform, _constants.model_transform_i);
	buffer_CameraProjView camProjView = deref(buffer_CameraProjView, _constants.i_camProjView);
	buffer_WorldTransform transform = deref(buffer_WorldTransform, _constants.i_transform);

	mat4 projection = camProjView.proj;
	mat4 view = camProjView.view;
	mat4 model = transform.data;
	// mat4 projection = _constants.cameraViewProj.proj;
	// mat4 view = _constants.cameraViewProj.view;
	//mat4 model = _constants.transform.data;
	out_uv = in_uv;
	//gl_Position = projection * view * model * vec4(positions[gl_VertexIndex], 0.0, 1.0);
	gl_Position = projection * view * model * vec4(in_pos, 1.0);
}

#elif defined(FRAGMENT_SHADER)

layout (set = 0, binding = SAMPLED_IMAGE_BINDING) uniform texture2D _tex_store[];
layout (set = 0, binding = SAMPLER_BINDING) uniform sampler _sampler_store[];

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 frag_color;

void main()
{
	frag_color = vec4(texture(sampler2D(_tex_store[_constants.i_baseColorMap], _sampler_store[0]), in_uv).xyz, 1.0);
	//frag_color = vec4(1.0, 1.0, 1.0, 1.0);
}

#endif