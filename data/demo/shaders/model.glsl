#version 460 core

#include <forge.glsl>

struct Vertex
{
	vec3 pos;
	vec2 uv;
};

layout (buffer_reference, scalar) readonly buffer buffer_CameraProjView
{
	mat4 proj;
	mat4 view;
};

layout (buffer_reference, scalar) readonly buffer buffer_WorldTransform
{
	mat4 data;
};

layout (buffer_reference, scalar) readonly buffer buffer_VertexBuffer
{
	Vertex vertices[];
};

layout (push_constant, scalar) uniform PushConstant
{
	buffer_VertexBuffer vb;
	buffer_WorldTransform transform;
	buffer_CameraProjView cameraViewProj;
	uint baseColorMapIndex;
} _pc;

#if defined(VERTEX_SHADER)

layout (location = 0) out vec2 out_uv;

void main()
{
	out_uv = _pc.vb.vertices[gl_VertexIndex].uv;
	gl_Position = _pc.cameraViewProj.proj * _pc.cameraViewProj.view * _pc.transform.data * vec4(_pc.vb.vertices[gl_VertexIndex].pos, 1.0);
}

#elif defined(FRAGMENT_SHADER)

layout (set = 0, binding = COMBINED_IMAGE_SAMPLER_BINDING) uniform sampler2D _combined_img_sampler[];

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 fragColor;

void main()
{
	fragColor = vec4(texture(_combined_img_sampler[_pc.baseColorMapIndex], in_uv).xyz, 1.0);
}

#endif