#version 450

// Input interpolated from the vertex shader
layout(location = 0) in vec3 fragColor;

// Output color written to the framebuffer
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}