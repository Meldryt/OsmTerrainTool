#version 440

layout(location = 0) in float v_heightPercent;
layout(location = 1) in float v_alpha;

layout(location = 0) out vec4 fragColor;

void main()
{
    vec3 color;
    vec3 color_clear = vec3(0,0,0);
    vec3 color_a = vec3(0,0.4,1);
    vec3 color_b = vec3(0,1,0);
    vec3 color_c = vec3(1,1,0);
    vec3 color_d = vec3(1,0,0);
    vec3 color_e = vec3(1,1,1);

    if(v_heightPercent < 0.0f)
    {
        color = color_clear;
    }
    else if(v_heightPercent < 0.25f)
    {
        color = mix(color_a,color_b, v_heightPercent / 0.25f);
    }
    else if(v_heightPercent < 0.5f)
    {
        color = mix(color_b,color_c, (v_heightPercent-0.25f) / 0.25f);
    }
    else if(v_heightPercent < 0.75f)
    {
        color = mix(color_c,color_d, (v_heightPercent-0.5f) / 0.25f);
    }
    else
    {
        color = mix(color_d,color_e, (v_heightPercent-0.75f) / 0.25f);
    }

    fragColor = vec4(color,v_alpha);
}
