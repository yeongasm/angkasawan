#version 450

layout (location = 0) in vec3 InPosition;  
layout (location = 1) in vec3 InNormal;    
layout (location = 2) in vec3 InTangent;
layout (location = 3) in vec3 InBitangent;
layout (location = 4) in vec2 InTexCoord;

layout (location = 5)   in mat4 InTransform;
layout (location = 9)   in vec2 VertTexCoord[4];
layout (location = 13)  in vec3 InColor;

layout (location = 0) out vec2 texCoord;
layout (location = 1) out vec3 glyphColor;

layout (push_constant) uniform Constants
{
  mat4 Projection;
} pushConstants;

void main()
{
  texCoord = VertTexCoord[gl_VertexIndex];
  glyphColor = InColor;
  gl_Position = pushConstants.Projection * InTransform * vec4(InPosition.xy, 0.0, 1.0);
}