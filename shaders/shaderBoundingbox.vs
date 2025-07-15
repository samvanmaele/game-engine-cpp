#version 300 es
precision highp float;

layout (location = 0) in int vert;
uniform vec3 boundingBox[2];

layout (std140) uniform PROJ
{
    mat4 projection;
};
layout (std140) uniform VIEW
{
    mat4 view;
};

void main()
{
    vec3 pos = vec3(boundingBox[vert%2].x, boundingBox[(vert/2)%2].y, boundingBox[vert/4].z);
    gl_Position = projection * view * vec4(pos, 1);
}