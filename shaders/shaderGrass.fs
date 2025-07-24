#version 300 es
precision highp float;

in float dis;
in float height;

out vec4 color;

void main()
{
    color = vec4(dis * 0.5, 0.2 + 0.2 * height + 0.4 * dis, dis, 1.0);
    color = pow(color, vec4(0.45));
}