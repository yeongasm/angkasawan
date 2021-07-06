#version 460
layout (location = 0) out vec4 FragColor;
layout (set = 0, binding = 2) uniform sampler2D textureAtlas;

layout (location = 0) in vec2 texCoord;

layout (push_constant) uniform Constants
{
  mat4 Projection;
  vec2 TexCoord[4];
  vec3 Color;
} pushConstants;

void main()
{
  FragColor = vec4(pushConstants.Color, 1.0) * vec4(1.0, 1.0, 1.0, texture(textureAtlas, texCoord).w);
  //FragColor = vec4(pushConstants.Color, 1.0);
}