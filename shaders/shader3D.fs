#version 300 es
precision highp float;

in vec2 TexCoords;

uniform sampler2D material;

layout (location = 0) out vec4 color;

void main() {
    vec4 baseTex = texture(material, TexCoords);
    color = pow(baseTex, vec4(0.45));
}