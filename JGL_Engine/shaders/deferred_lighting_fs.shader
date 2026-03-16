#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormalRoughness;
uniform sampler2D gAlbedoMetallic;

uniform vec3 camPos;
uniform int debugView;

const float PI = 3.14159265359;
const int MAX_DEFERRED_LIGHTS = 8;

struct PointLight
{
    vec3 position;
    vec3 color;
    int enabled;
};

uniform PointLight lights[MAX_DEFERRED_LIGHTS];
uniform int lightCount;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.0000001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / max(denom, 0.0000001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

void main()
{
    vec3 WorldPos = texture(gPosition, TexCoords).rgb;
    vec4 normalRoughness = texture(gNormalRoughness, TexCoords);
    vec4 albedoMetallic = texture(gAlbedoMetallic, TexCoords);

    vec3 N = normalize(normalRoughness.rgb);
    if (length(N) < 0.001)
    {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    float roughness = clamp(normalRoughness.a, 0.04, 1.0);
    // G-Buffer albedo is already stored in linear space.
    vec3 albedo = albedoMetallic.rgb;
    float metallic = clamp(albedoMetallic.a, 0.0, 1.0);

    if (debugView == 1)
    {
        vec3 debugColor = abs(WorldPos) / (abs(WorldPos) + vec3(1.0));
        FragColor = vec4(debugColor, 1.0);
        return;
    }
    if (debugView == 2)
    {
        FragColor = vec4(N * 0.5 + 0.5, 1.0);
        return;
    }
    if (debugView == 3)
    {
        FragColor = vec4(pow(clamp(albedoMetallic.rgb, vec3(0.0), vec3(1.0)), vec3(1.0 / 2.2)), 1.0);
        return;
    }
    if (debugView == 4)
    {
        FragColor = vec4(vec3(roughness), 1.0);
        return;
    }
    if (debugView == 5)
    {
        FragColor = vec4(vec3(metallic), 1.0);
        return;
    }

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    vec3 V = normalize(camPos - WorldPos);
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < lightCount && i < MAX_DEFERRED_LIGHTS; ++i)
    {
        if (lights[i].enabled == 0)
            continue;

        vec3 L = normalize(lights[i].position - WorldPos);
        vec3 H = normalize(V + L);

        float distance = length(lights[i].position - WorldPos);
        float attenuation = 1.0 / max(distance * distance, 0.001);
        vec3 radiance = lights[i].color * attenuation;

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

        vec3 nominator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        vec3 specular = nominator / max(denominator, 0.001);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * albedo;
    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
