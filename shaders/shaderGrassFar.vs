#version 300 es
precision highp float;
precision highp int;

layout (std140) uniform PROJ {mat4 projection;};
layout (std140) uniform VIEW {mat4 view;};
layout (std140) uniform cam  {vec3 campos;};

uniform sampler2D noisemap;
uniform ivec2 forward;

out float dis;
out float height;

const int power = 8;
const int rows = 1 << power;
const int halfrow = 1 << (power - 1);
const vec3 vertices[3] = vec3[3]
(
    vec3(-0.1, 0, 0),
    vec3(0.1, 0, 0),
    vec3(0, 0.5, 0)
);

void main()
{
    int id = gl_InstanceID;
    int x = id & (rows - 1);
    int y = id >> power;

    bool swap = abs(forward.y) > abs(forward.x);
    ivec2 base = swap ? ivec2(x, y) : ivec2(y, x);

    base -= halfrow;
    base *= sign(forward) | ivec2(1);
    base += forward;

    vec2 grid = vec2(base);
    dis = dot(grid, grid) * 0.00005;

    grid += floor(campos.xz);
    grid += texture(noisemap, grid * 0.01).xy * 10.0;

    grid.x += 0.4 * float(gl_VertexID / 3);
    vec3 vpos = vertices[gl_VertexID % 3];
    height = vpos.y * 2.0;

    vec4 worldpos = view * vec4(grid.x, 0.0, grid.y, 1.0) + vec4(vpos, 0);
    gl_Position = projection * worldpos;
}