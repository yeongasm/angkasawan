#version 460

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec2 inTexCoord;

layout (location = 4) in mat4 inTransform;

layout (location = 0) out vec2 texCoord;
layout (location = 1) out vec3 fragPos;
layout (location = 2) out vec3 normal;

layout (set = 0, binding = 0) uniform SModelTransform
{
    mat4 inViewProj;
    mat4 inView;
} ubo;

void main()
{
    // We need to export the result of the TBN matrix here as well ....
    vec4 worldPos = inTransform * vec4(inPosition, 1.0);
    mat3 normalMatrix = transpose(inverse(mat3(inTransform)));

    texCoord = inTexCoord;
    fragPos = worldPos.xyz;
    normal = normalMatrix * inNormal;

    gl_Position = ubo.inViewProj * worldPos;
}