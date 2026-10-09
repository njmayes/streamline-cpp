#type vertex
#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aTangent;

layout(std140, binding = 5) uniform Camera3D
{
    mat4 uViewProjection;
    mat4 uLightViewProjection;
    vec4 uCameraPosition;
    vec4 uAmbient;
    vec4 uSunDirectionIntensity;
    vec4 uSunColourShadow;
    ivec4 uLightCounts;
    vec4 uPointData[32];
    vec4 uSpotData[32];
};
layout(std140, binding = 6) uniform Surface3D
{
    mat4 uModel;
    mat4 uNormalMatrix;
    vec4 uBaseColour;
    vec4 uEmissiveMetallic;
    vec4 uSurfaceParams;
    vec4 uTextureFlags;
};

layout(location = 0) out vec3 vWorldPosition;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec2 vUV;
layout(location = 3) out vec4 vTangent;
layout(location = 4) out vec4 vLightClip;

void main()
{
    vec4 world = uModel * vec4(aPosition, 1.0);
    vWorldPosition = world.xyz;
    vNormal = normalize(mat3(uNormalMatrix) * aNormal);
    vTangent = vec4(normalize(mat3(uModel) * aTangent.xyz), aTangent.w);
    vUV = aTexCoord;
    vLightClip = uLightViewProjection * world;
    gl_Position = uViewProjection * world;
}

#type fragment
#version 450 core

layout(location = 0) in vec3 vWorldPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;
layout(location = 3) in vec4 vTangent;
layout(location = 4) in vec4 vLightClip;
layout(location = 0) out vec4 oColour;

layout(binding = 0) uniform sampler2D uBaseMap;
layout(binding = 1) uniform sampler2D uNormalMap;
layout(binding = 2) uniform sampler2D uMetalRoughMap;
layout(binding = 3) uniform sampler2D uEmissiveMap;
layout(binding = 4) uniform sampler2DShadow uShadowMap;

layout(std140, binding = 5) uniform Camera3D
{
    mat4 uViewProjection;
    mat4 uLightViewProjection;
    vec4 uCameraPosition;
    vec4 uAmbient;
    vec4 uSunDirectionIntensity;
    vec4 uSunColourShadow;
    ivec4 uLightCounts;
    vec4 uPointData[32];
    vec4 uSpotData[32];
};
layout(std140, binding = 6) uniform Surface3D
{
    mat4 uModel;
    mat4 uNormalMatrix;
    vec4 uBaseColour;
    vec4 uEmissiveMetallic;
    vec4 uSurfaceParams;
    vec4 uTextureFlags;
};

const float PI = 3.14159265359;
float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 0.00001);
}
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float k = (roughness + 1.0);
    k = k * k / 8.0;
    return NdotV / max(NdotV * (1.0 - k) + k, 0.00001);
}
vec3 Fresnel(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - clamp(cosTheta, 0.0, 1.0), 5.0);
}
vec3 EvaluateBRDF(vec3 N, vec3 V, vec3 L, vec3 radiance, vec3 albedo, float metallic, float roughness)
{
    vec3 H = normalize(V + L);
    float NdotV = max(dot(N, V), 0.001);
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    if (NdotL <= 0.0) return vec3(0.0);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = Fresnel(max(dot(H, V), 0.0), F0);
    float D = DistributionGGX(NdotH, roughness);
    float G = GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
    vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.001);
    vec3 diffuse = (1.0 - F) * (1.0 - metallic) * albedo / PI;
    return (diffuse + specular) * radiance * NdotL;
}
float ShadowVisibility(vec3 N, vec3 sunLightDirection)
{
    if (uSunColourShadow.w < 0.5) return 1.0;
    vec3 projected = vLightClip.xyz / max(vLightClip.w, 0.00001);
    projected = projected * 0.5 + 0.5;
    if (projected.z <= 0.0 || projected.z >= 1.0) return 1.0;
    float bias = max(0.0004 * (1.0 - dot(N, sunLightDirection)), 0.00008);
    vec2 texel = 1.0 / vec2(textureSize(uShadowMap, 0));
    float visibility = 0.0;
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            visibility += texture(uShadowMap, vec3(projected.xy + vec2(x, y) * texel, projected.z - bias));
    return visibility / 9.0;
}
void main()
{
    vec4 albedoSample = uTextureFlags.x > 0.5 ? texture(uBaseMap, vUV) : vec4(1.0);
    vec4 base = uBaseColour * vec4(pow(albedoSample.rgb, vec3(2.2)), albedoSample.a);
    if (uSurfaceParams.z > 0.5 && base.a < uSurfaceParams.y) discard;
    vec3 N = normalize(vNormal);
    if (!gl_FrontFacing) N = -N;
    if (uTextureFlags.y > 0.5)
    {
        vec3 T = normalize(vTangent.xyz - N * dot(vTangent.xyz, N));
        vec3 B = normalize(cross(N, T)) * vTangent.w;
        vec3 sampled = texture(uNormalMap, vUV).xyz * 2.0 - 1.0;
        sampled.xy *= uSurfaceParams.w;
        N = normalize(mat3(T, B, N) * sampled);
    }
    float metallic = clamp(uEmissiveMetallic.w, 0.0, 1.0);
    float roughness = clamp(uSurfaceParams.x, 0.045, 1.0);
    if (uTextureFlags.z > 0.5)
    {
        vec4 sampleMR = texture(uMetalRoughMap, vUV);
        metallic *= sampleMR.b;
        roughness *= sampleMR.g;
        roughness = max(roughness, 0.045);
    }
    vec3 V = normalize(uCameraPosition.xyz - vWorldPosition);
    vec3 colour = uAmbient.rgb * base.rgb * (1.0 - metallic);
    vec3 L = normalize(-uSunDirectionIntensity.xyz);
    colour += EvaluateBRDF(N, V, L, uSunColourShadow.rgb * uSunDirectionIntensity.w * ShadowVisibility(N, L), base.rgb, metallic, roughness);
    for (int i = 0; i < uLightCounts.x; ++i)
    {
        vec4 point = uPointData[i * 2];
        vec4 light = uPointData[i * 2 + 1];
        vec3 offset = point.xyz - vWorldPosition;
        float distance = length(offset);
        if (distance >= point.w || distance < 0.0001) continue;
        float falloff = pow(clamp(1.0 - pow(distance / point.w, 4.0), 0.0, 1.0), 2.0);
        colour += EvaluateBRDF(N, V, offset / distance, light.rgb * light.w * falloff / max(distance * distance, 0.01), base.rgb, metallic, roughness);
    }
    for (int i = 0; i < uLightCounts.y; ++i)
    {
        vec4 spot = uSpotData[i * 4];
        vec4 direction = uSpotData[i * 4 + 1];
        vec4 light = uSpotData[i * 4 + 2];
        vec4 cone = uSpotData[i * 4 + 3];
        vec3 offset = spot.xyz - vWorldPosition;
        float distance = length(offset);
        if (distance >= spot.w || distance < 0.0001) continue;
        vec3 toLight = offset / distance;
        float theta = dot(-toLight, normalize(direction.xyz));
        float angular = smoothstep(cone.y, cone.x, theta);
        float falloff = pow(clamp(1.0 - pow(distance / spot.w, 4.0), 0.0, 1.0), 2.0);
        colour += EvaluateBRDF(N, V, toLight, light.rgb * light.w * angular * falloff / max(distance * distance, 0.01), base.rgb, metallic, roughness);
    }
    vec3 emission = uEmissiveMetallic.rgb * (uTextureFlags.w > 0.5 ? pow(texture(uEmissiveMap, vUV).rgb, vec3(2.2)) : vec3(1.0));
    oColour = vec4(colour + emission, base.a);
}
