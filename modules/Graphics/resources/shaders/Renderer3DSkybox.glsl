#type vertex
#version 450 core
layout(location = 0) out vec2 vUV;
void main()
{
    vec2 p = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    vUV = p;
    gl_Position = vec4(p * 2.0 - 1.0, 1.0, 1.0);
}
#type fragment
#version 450 core
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 oColour;
layout(binding = 0) uniform samplerCube uSkybox;
layout(std140, binding = 9) uniform SkyboxCamera3D
{
    mat4 uInverseViewProjection;
};
void main()
{
    vec4 direction = uInverseViewProjection * vec4(vUV * 2.0 - 1.0, 1.0, 1.0);
    vec3 ray = normalize(direction.xyz / direction.w);
    oColour = vec4(textureLod(uSkybox, ray, 0.0).rgb, 1.0);
}
