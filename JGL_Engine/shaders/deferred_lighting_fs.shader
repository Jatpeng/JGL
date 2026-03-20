#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormalRoughness;
uniform sampler2D gAlbedoMetallic;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;
uniform sampler2D shadowMap;

uniform vec3 camPos;
uniform int debugView;
uniform int iblEnabled = 0;
uniform float prefilterMaxLod = 4.0;
uniform int shadowEnabled = 0;
uniform int shadowLightIndex = -1;
uniform mat4 lightSpaceMatrix;
uniform float shadowBiasMin = 0.0008;
uniform float shadowBiasMax = 0.02;
uniform int shadowFilterRadius = 1;

const float PI = 3.14159265359;
const int MAX_DEFERRED_LIGHTS = 8;
const int LIGHT_TYPE_POINT = 0;
const int LIGHT_TYPE_DIRECTIONAL = 1;

struct SceneLight
{
    vec3 position;
    int enabled;
    vec3 color;
    int type;
    vec3 direction;
    float padding0;
};

uniform SceneLight lights[MAX_DEFERRED_LIGHTS];
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

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

float calculateDirectionalShadow(vec3 worldPos, vec3 normal, vec3 lightDirection)
{
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(worldPos, 1.0);
    if (fragPosLightSpace.w <= 0.0)
        return 0.0;

    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0)
    {
        return 0.0;
    }

    float ndotl = max(dot(normalize(normal), normalize(-lightDirection)), 0.0);
    float bias = max(shadowBiasMax * (1.0 - ndotl), shadowBiasMin);
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    int radius = clamp(shadowFilterRadius, 0, 4);
    float shadow = 0.0;
    float sampleCount = 0.0;

    for (int x = -4; x <= 4; ++x)
    {
        if (abs(x) > radius)
            continue;

        for (int y = -4; y <= 4; ++y)
        {
            if (abs(y) > radius)
                continue;

            float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += projCoords.z - bias > closestDepth ? 1.0 : 0.0;
            sampleCount += 1.0;
        }
    }

    return sampleCount > 0.0 ? shadow / sampleCount : 0.0;
}

void main()
{
    vec4 positionAo = texture(gPosition, TexCoords);
    vec3 WorldPos = positionAo.rgb;
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
    float ao = positionAo.a;

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

        vec3 L = vec3(0.0, 1.0, 0.0);
        vec3 radiance = lights[i].color;
        if (lights[i].type == LIGHT_TYPE_DIRECTIONAL)
        {
            L = normalize(-lights[i].direction);
        }
        else
        {
            vec3 lightVector = lights[i].position - WorldPos;
            float distance = max(length(lightVector), 0.001);
            L = lightVector / distance;
            radiance *= 1.0 / max(distance * distance, 0.001);
        }
        vec3 H = normalize(V + L);

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
        float shadow = 0.0;
        if (shadowEnabled != 0 &&
            i == shadowLightIndex &&
            lights[i].type == LIGHT_TYPE_DIRECTIONAL)
        {
            shadow = calculateDirectionalShadow(WorldPos, N, lights[i].direction);
        }

        Lo += (1.0 - shadow) * (kD * albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * albedo * ao;
    if (iblEnabled != 0)
    {
        vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
        vec3 irradiance = texture(irradianceMap, N).rgb;
        vec3 diffuse = irradiance * albedo;

        vec3 R = reflect(-V, N);
        vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * prefilterMaxLod).rgb;
        vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
        vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

        ambient = (kD * diffuse + specular) * ao;
    }

    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
