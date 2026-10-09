#type vertex
#version 450 core
layout(location = 0) in vec3 aPosition;
layout(location = 2) in vec2 aTexCoord;
layout(location = 0) out vec2 vUV;
layout(std140, binding = 7) uniform ShadowPass3D
{
    mat4 uLightViewProjection;
    mat4 uModel;
    vec4 uAlphaParams;
};
void main()
{
    vUV = aTexCoord;
    gl_Position = uLightViewProjection * uModel * vec4(aPosition, 1.0);
}
#type fragment
#version 450 core
layout(location = 0) in vec2 vUV;
layout(binding = 0) uniform sampler2D uBaseMap;
layout(std140, binding = 7) uniform ShadowPass3D
{
    mat4 uLightViewProjection;
    mat4 uModel;
    vec4 uAlphaParams;
};
void main()
{
    if (uAlphaParams.z > 0.5)
    {
        float alpha = uAlphaParams.x * (uAlphaParams.w > 0.5 ? texture(uBaseMap, vUV).a : 1.0);
        if (alpha < uAlphaParams.y) discard;
    }
}
