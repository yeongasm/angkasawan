#version 460

layout (location = 0) in vec3 InPosition;  
layout (location = 1) in vec3 InNormal;    
layout (location = 2) in vec3 InTangent;
layout (location = 3) in vec3 InBitangent;
layout (location = 4) in vec2 InTexCoord;

layout (location = 5) in mat4 InTransform;

layout (location = 0) out vec2 texCoord;

layout (set = 0, binding = 0) uniform ModelTransform
{
    mat4 ViewProj;
    mat4 View;
} ubo;

void main()
{
    texCoord = InTexCoord;
    gl_Position = ubo.ViewProj * InTransform * vec4(InPosition, 1.0);
}