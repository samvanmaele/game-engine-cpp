#version 300 es
precision highp float;

in float dis;
in float height;

out vec4 color;

void main()
{
    color = vec4(0.0, height * 0.2 + 0.2, 0.0, 1.0);
    color.r = dis * 0.2;
    color.b = dis;
    //color.a = 1.0 - 5.0 * dis;
    color = pow(color, vec4(0.45));
}