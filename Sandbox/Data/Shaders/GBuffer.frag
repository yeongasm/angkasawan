#version 460

#define MAX_TEXTURE_COUNT 128

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 outPositionColor;
layout (location = 2) out vec4 outNormalsColor;
layout (location = 3) out vec4 outAlbedoColor;

layout (location = 0) in vec2 texCoord;
layout (location = 1) in vec3 fragPos;
layout (location = 2) in vec3 normal;

layout (set = 0, binding = 1) uniform sampler2D baseColor[MAX_TEXTURE_COUNT];

layout (push_constant) uniform Constants
{
    int baseColorIdx;
} pushConstants;

void main()
{
    outPositionColor = vec4(fragPos, 1.0);
    outNormalsColor = vec4(normalize(normal), 1.0);
    outAlbedoColor = vec4(texture(baseColor[pushConstants.baseColorIdx], texCoord).xyz, 1.0);

    FragColor = outAlbedoColor;
}