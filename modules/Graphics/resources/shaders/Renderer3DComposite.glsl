#type vertex
#version 450 core

layout(location = 0) out vec2 vUV;

void main()
{
    int vertexID = gl_VertexIndex;

    vec2 p = vec2(
        (vertexID << 1) & 2,
        vertexID & 2
    );

    vUV = p;

    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}

#type fragment
#version 450 core
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 oColour;
layout(binding = 0) uniform sampler2D uHDR;
layout(std140, binding = 8) uniform Composite3D
{
    vec4 uTone; // exposure, gamma, fxaa, reserved
    vec4 uTexel; // inv width, inv height, reserved
};
vec3 ReadHDR(vec2 uv) { return texture(uHDR, uv).rgb; }
void main()
{
    vec3 hdr = ReadHDR(vUV);
    if (uTone.z > 0.5)
    {
        vec2 pixel = uTexel.xy;
        float l00 = dot(ReadHDR(vUV + vec2(-1, -1) * pixel), vec3(0.299, 0.587, 0.114));
        float l10 = dot(ReadHDR(vUV + vec2( 1, -1) * pixel), vec3(0.299, 0.587, 0.114));
        float l01 = dot(ReadHDR(vUV + vec2(-1,  1) * pixel), vec3(0.299, 0.587, 0.114));
        float l11 = dot(ReadHDR(vUV + vec2( 1,  1) * pixel), vec3(0.299, 0.587, 0.114));
        float lc = dot(hdr, vec3(0.299, 0.587, 0.114));
        float minL = min(lc, min(min(l00, l10), min(l01, l11)));
        float maxL = max(lc, max(max(l00, l10), max(l01, l11)));
        vec2 direction = vec2(-(l00 + l10 - l01 - l11), l00 + l01 - l10 - l11);
        float reduce = max((l00 + l10 + l01 + l11) * 0.03125, 0.0078125);
        float rcp = 1.0 / (min(abs(direction.x), abs(direction.y)) + reduce);
        direction = clamp(direction * rcp, vec2(-8.0), vec2(8.0)) * pixel;
        vec3 a = (ReadHDR(vUV + direction / 6.0) + ReadHDR(vUV - direction / 6.0)) * 0.5;
        vec3 b = a * 0.5 + (ReadHDR(vUV + direction * 0.5) + ReadHDR(vUV - direction * 0.5)) * 0.25;
        float lb = dot(b, vec3(0.299, 0.587, 0.114));
        hdr = lb < minL || lb > maxL ? a : b;
    }
    vec3 mapped = vec3(1.0) - exp(-hdr * max(uTone.x, 0.001));
    oColour = vec4(pow(mapped, vec3(1.0 / max(uTone.y, 0.001))), 1.0);
}
