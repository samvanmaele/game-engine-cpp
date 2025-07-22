#version 300 es
precision highp float;

layout (std140) uniform PROJ
{
    mat4 projection;
};
layout (std140) uniform VIEW
{
    mat4 view;
};
const int vertices[36] = int[36]
(
    0, 2, 1, 3, 1, 2,
    4, 5, 6, 7, 6, 5,
    0, 1, 4, 5, 4, 1,
    2, 6, 3, 7, 3, 6,
    0, 4, 2, 6, 2, 4,
    1, 3, 5, 7, 5, 3
);

out vec3 TexCoords;

void main()
{
    int vert = vertices[gl_VertexID];
    vec3 vpos = vec3(vert%2, (vert/2)%2, vert/4) - 0.5;
    TexCoords = vpos;
    vec4 pos = projection * mat4(mat3(view)) * vec4(vpos, 1.0);
    gl_Position = pos.xyww;
}