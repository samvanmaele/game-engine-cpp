#version 300 es
precision highp float;

layout(location = 0) in vec3 vpos;
layout(location = 1) in vec2 vtex;

layout (std140) uniform PROJ {
    mat4 projection;
};
layout (std140) uniform VIEW {
    mat4 view;
};

uniform mat4 model;

out vec2 TexCoords;

void main() {
    TexCoords = vtex;
    gl_Position = projection * view * model * vec4(vpos, 1.0);
}