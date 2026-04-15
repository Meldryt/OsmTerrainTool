#version 440

layout(location = 0) in vec2 position;
layout(location = 1) in int height;

layout(location = 0) out float v_heightPercent;
layout(location = 1) out float v_alpha;

layout(std140, binding = 0) uniform bufStatic {
    mat4 mvp;
};

layout(std140, binding = 1) uniform bufDynamic {
    int minHeight;
    int maxHeight;
};

void main()
{
    v_heightPercent = float(height-minHeight)/(maxHeight-minHeight);

    if(height <= -9999)
    {
        v_alpha = 0.0f;
    }
    else
    {
        v_alpha = 1.0f;
    }

    gl_Position = mvp * vec4(position,0.0,1.0);
}
