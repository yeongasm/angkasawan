#version 460
layout (location = 0) out vec4 FragColor;
layout (set = 0, binding = 2) uniform sampler2D textureAtlas;

layout (location = 0) flat in int vertexIndex;

layout (push_constant) uniform Constants
{
  mat4 Projection;
  vec2 TexCoord[4];
  vec3 Color;
} pushConstants;

void main()
{
  vec2 sampleAt = pushConstants.TexCoord[vertexIndex];
  vec3 outSampler = texture(textureAtlas, sampleAt).xyz;
  FragColor = vec4(pushConstants.Color, 1.0) * vec4(outSampler, 1.0);
}