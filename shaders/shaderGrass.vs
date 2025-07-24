#version 300 es
precision highp float;
precision highp int;

layout (std140) uniform PROJ {mat4 projection;};
layout (std140) uniform VIEW {mat4 view;};
layout (std140) uniform cam  {vec3 campos;};

uniform sampler2D noisemap;
uniform ivec2 forward;
uniform float time;
uniform vec2 playerpos;

out float dis;
out float height;

const int power = 10;
const int rows = 1 << power;
const int halfrow = rows / 2;
const vec3 vertices[5] = vec3[5]
(
    vec3(-0.1, 0, 0),
    vec3(0.1, 0, 0),
    vec3(-0.075, 0.25, 0),
    vec3(0.075, 0.25, 0),
    vec3(0, 0.5, 0)
);

void main()
{
    float group = 3.0 - float(gl_InstanceID >> 14);
    float density = 1.0/(2.0 * 3.0);

    int id = gl_InstanceID;
    ivec2 base = ivec2(id & (rows - 1), id >> power) - halfrow;
    base *= sign(forward) | ivec2(1);
    base += forward;

    vec2 grid = vec2(base) * density;
    dis = dot(grid, grid) * 0.0001;

    grid += floor(campos.xz);
    vec2 rnd = texture(noisemap, grid * 0.01).xy;
    grid += rnd;

    vec3 vpos = vertices[gl_VertexID];
    vpos.y *= rnd.x + 1.0;
    height = vpos.y * 2.0;

    vec2 direction = grid - playerpos;
    float dist = length(direction);
    float effect = clamp(dist, 0.0, 1.0);

    grid += height * (1.0 - effect) * direction * 2.0;
    grid.x += height * floor(effect) * sin(time + grid.x + grid.y) * 0.2;

    vec4 worldpos = view * vec4(grid.x, 0.0, grid.y, 1.0) + vec4(vpos, 0);
    gl_Position = projection * worldpos;
}