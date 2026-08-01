#version 450

// Per-vertex (binding 0)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

// Per-instance (binding 1) — mat4 occupies locations 2..5, one column each
layout(location = 2) in mat4 inModel;
layout(location = 6) in vec4 inTintColor;

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
} pc;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragTint;

void main() {
    gl_Position = pc.viewProj * inModel * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
    fragTint = inTintColor;
}