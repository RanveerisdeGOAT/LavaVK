#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 2) in vec4 inModelCol0;
layout(location = 3) in vec4 inModelCol1;
layout(location = 4) in vec4 inModelCol2;
layout(location = 5) in vec4 inModelCol3;
layout(location = 6) in vec4 inTintColor;
layout(location = 7) in vec4 inTextureId; // .x holds the bindless texture index, packed as a float

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
} pc;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec4 outTintColor;
layout(location = 2) flat out uint outTextureId;

void main() {
    mat4 model = mat4(inModelCol0, inModelCol1, inModelCol2, inModelCol3);

    gl_Position = pc.viewProj * model * vec4(inPosition, 1.0);

    outTexCoord = inTexCoord;
    outTintColor = inTintColor;
    outTextureId = uint(inTextureId.x + 0.5);
}
