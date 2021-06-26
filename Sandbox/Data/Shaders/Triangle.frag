#version 460

#define MAX_TEXTURE_COUNT 128

layout (location = 0) out vec4 FragColor;
layout (location = 0) in vec2 texCoord;

layout (set = 0, binding = 1) uniform sampler2D baseColor[MAX_TEXTURE_COUNT];

layout (push_constant) uniform Constants
{
    int baseColorIdx;
} pushConstants;

void main()
{
    //FragColor = vec4(texCoord.x, texCoord.y, 1.0, 1.0);
    FragColor = vec4(texture(baseColor[pushConstants.baseColorIdx], texCoord).xyz, 1.0);
    //AttachmentColor = FragColor;
}