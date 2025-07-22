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
layout (std140) uniform cam
{
    vec3 campos;
};

uniform sampler2D map;
uniform vec2 forward;
uniform float time;
uniform float density;

out float dis;
out float height;

const float rows = 128.0;
const vec3 vertices[5] = vec3[5]
(
    vec3(-0.1, 0, 0),
    vec3(0.1, 0, 0),
    vec3(-0.075, 0.25, 0),
    vec3(0.075, 0.25, 0),
    vec3(0, 0.5, 0)
);

float hash12(vec2 p)
{
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}
void main()
{
    float id = float(gl_InstanceID);
    float x = mod(id, rows);
    float y = floor(id / rows);

    vec2 grid = (vec2(x, y) - rows * 0.5 + floor(forward * (rows * 0.5))) * density;
    dis = length(grid) * 0.005;

    grid += floor(vec2(campos.x, campos.z));

    float rnd = hash12(grid);
    vec2 jitter = vec2(cos(rnd * 6.2831), sin(rnd * 6.2831)) * 0.3;
    grid += jitter;

    vec3 vpos = vertices[gl_VertexID];
    height = vpos.y * 2.0;

    grid.x += height * (cos(time + grid.x + grid.y)) * 0.2;

    vec4 worldpos = view * vec4(grid.x, 0, grid.y, 1) + vec4(vpos, 0);
    gl_Position = projection * worldpos;
}