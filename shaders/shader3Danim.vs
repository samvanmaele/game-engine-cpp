#version 300 es
precision highp float;

layout(location = 0) in vec3 vpos;
layout(location = 1) in vec3 vnorm;
layout(location = 2) in vec2 vtex;
layout(location = 3) in ivec4 vboneIds;
layout(location = 4) in vec4 vweights;

layout (std140) uniform PROJ
{
    mat4 projection;
};
layout (std140) uniform VIEW
{
    mat4 view;
};

uniform mat4 model;
uniform mat4 animation[50];

out vec2 TexCoords;
out vec3 fragPos;
out vec3 fragNorm;

vec4 applyBone(vec4 p)
{
    vec4 result = vec4(0.0);
    for(int i = 0; i < 4; ++i)
    {
        if(vboneIds[i] >= 100)
        {
            result = p;
            break;
        }
        result += vweights[i] * (animation[vboneIds[i]] * p);
    }
    return result;
}

void main()
{

    vec4 position = applyBone(vec4(vpos, 1.0));
    vec4 normal = normalize(applyBone(vec4(vnorm, 0.0)));

    vec4 vertPos = model * position;

    TexCoords = vtex;
    fragPos = vertPos.xyz;
    fragNorm = (view * normal).xyz;

    gl_Position = projection * view * vertPos;
}