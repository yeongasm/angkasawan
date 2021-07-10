#version 450
layout (location = 0) out vec4 FragColor;
layout (set = 0, binding = 2) uniform sampler2D textureAtlas;

layout (location = 0) in vec2 texCoord;
layout (location = 1) in vec3 glyphColor;

layout (push_constant) uniform Constants
{
  mat4 Projection;
} pushConstants;

void main()
{
  FragColor = vec4(glyphColor, 1.0) * vec4(1.0, 1.0, 1.0, texture(textureAtlas, texCoord).w);
}