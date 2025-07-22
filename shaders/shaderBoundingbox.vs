#version 300 es
precision highp float;

uniform vec3 boundingBox[2];

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

void main()
{
    int vert = vertices[gl_VertexID];
    vec3 pos = vec3(boundingBox[vert%2].x, boundingBox[(vert/2)%2].y, boundingBox[vert/4].z);
    gl_Position = projection * view * vec4(pos, 1);
}