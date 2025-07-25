#version 300 es
precision highp float;

in float dis;
in float height;

out vec4 color;

void main()
{
    color = vec4(vec3(0.2*dis), 1.0);
    color.b *= 1.5;
    color.g += 0.1 + 0.15 * height;
    color = pow(color, vec4(0.45));
}