#version 460 core

#include <forge.glsl>

layout (buffer_reference, std140) readonly buffer ProjectionViewTransform
{
	mat4 projection;
	mat4 view;
};

layout (buffer_reference, std140) readonly buffer WorldTransform
{
	mat4 transform;
};

layout (push_constant) uniform PushConstant
{
	uint proj_view_i;
	uint model_transform_i;
	uint texture_i;
	uint sampler_i;
} _constants;

#if defined(VERTEX_SHADER)

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_uv;

layout (location = 0) out vec2 out_uv;

void main()
{
	ProjectionViewTransform projView = deref(ProjectionViewTransform, _constants.proj_view_i);
	WorldTransform world = deref(WorldTransform, _constants.model_transform_i);
	out_uv = in_uv;
	gl_Position = projView.projection * projView.view * world.transform * vec4(in_pos, 1.0);
}

#elif defined(FRAGMENT_SHADER)

layout (set = 0, binding = SAMPLED_IMAGE_BINDING) uniform texture2D _tex_store[];
layout (set = 0, binding = SAMPLER_BINDING) uniform sampler _sampler_store[];

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 frag_color;

void main()
{
	frag_color = vec4(texture(sampler2D(_tex_store[_constants.texture_i], _sampler_store[_constants.sampler_i]), in_uv).xyz, 1.0);
}

#endif