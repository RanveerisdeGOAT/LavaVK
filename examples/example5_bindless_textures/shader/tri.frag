#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform sampler2D bindlessTextures[];

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec4 inTintColor;
layout(location = 2) flat in uint inTextureId;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(bindlessTextures[nonuniformEXT(inTextureId)], inTexCoord);
    outColor = texColor * inTintColor;
}
