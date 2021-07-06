#version 460

layout (location = 0) in vec3 InPosition;  
layout (location = 1) in vec3 InNormal;    
layout (location = 2) in vec3 InTangent;
layout (location = 3) in vec3 InBitangent;
layout (location = 4) in vec2 InTexCoord;

layout (location = 5) in mat4 InTransform;

layout (location = 0) out vec2 texCoord;

layout (push_constant) uniform Constants
{
  mat4 Projection;
  vec2 TexCoord[4];
  vec3 Color;
} pushConstants;

void main()
{
    texCoord = pushConstants.TexCoord[gl_VertexIndex];
    gl_Position = pushConstants.Projection * InTransform * vec4(InPosition, 1.0);
}