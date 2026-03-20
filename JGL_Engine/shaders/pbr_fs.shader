#version 330 core

out vec4 FragColor;
in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoords;

// material parameters
uniform vec3 color;
uniform sampler2D baseMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D normalMap;
uniform sampler2D aoMap;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;
uniform sampler2D shadowMap;

uniform vec3 camPos;
uniform float opacity = 1.0;
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
uniform int lightCount = 0;
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Easy trick to get tangent-normals to world-space to keep PBR code simplified.
// Don't worry if you don't get what's going on; you generally want to do normal 
// mapping the usual way for performance anways; I do plan make a note of this 
// technique somewhere later in the normal mapping tutorial.
vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1 = dFdx(WorldPos);
    vec3 Q2 = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N = normalize(Normal);
    vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH * NdotH;

  float nom = a2;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = PI * denom * denom;

  return nom / max(denom, 0.0000001); // prevent divide by zero for roughness=0.0 and NdotH=1.0
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
  float r = (roughness + 1.0);
  float k = (r * r) / 8.0;

  float nom = NdotV;
  float denom = NdotV * (1.0 - k) + k;

  return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
  float NdotV = max(dot(N, V), 0.0);
  float NdotL = max(dot(N, L), 0.0);
  float ggx2 = GeometrySchlickGGX(NdotV, roughness);
  float ggx1 = GeometrySchlickGGX(NdotL, roughness);

  return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
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

vec3 accumulateLight(SceneLight light, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, vec3 F0)
{
  vec3 L = vec3(0.0, 1.0, 0.0);
  vec3 radiance = light.color;

  if (light.type == LIGHT_TYPE_DIRECTIONAL)
  {
    L = normalize(-light.direction);
  }
  else
  {
    vec3 lightVector = light.position - WorldPos;
    float distance = max(length(lightVector), 0.001);
    L = lightVector / distance;
    radiance *= 1.0 / max(distance * distance, 0.001);
  }

  vec3 H = normalize(V + L);

  float NDF = DistributionGGX(N, H, roughness);
  float G = GeometrySmith(N, V, L, roughness);
  vec3 F = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

  vec3 nominator = NDF * G * F;
  float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
  vec3 specular = nominator / max(denominator, 0.001);

  vec3 kS = F;
  vec3 kD = vec3(1.0) - kS;
  kD *= 1.0 - metallic;

  float NdotL = max(dot(N, L), 0.0);
  return (kD * albedo / PI + specular) * radiance * NdotL;
}
// ----------------------------------------------------------------------------
void main()
{
  vec3 N = getNormalFromMap();

  vec3 V = normalize(camPos - WorldPos);
  vec4 albedoSample = texture(baseMap, TexCoords);
  // Keep the forward path in the same linear workflow as deferred shading.
  vec3 albedo = pow(albedoSample.rgb, vec3(2.2)) * color;
  float metallic = texture(metallicMap, TexCoords).r;
  float roughness = clamp(texture(roughnessMap, TexCoords).r, 0.04, 1.0);
  float ao = texture(aoMap, TexCoords).r;
  float surfaceAlpha = albedoSample.a * opacity;

  // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
  // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, metallic);

  // reflectance equation
  vec3 Lo = vec3(0.0);

  for (int i = 0; i < lightCount && i < MAX_DEFERRED_LIGHTS; ++i)
  {
    if (lights[i].enabled == 0)
      continue;

    float shadow = 0.0;
    if (shadowEnabled != 0 &&
        i == shadowLightIndex &&
        lights[i].type == LIGHT_TYPE_DIRECTIONAL)
    {
      shadow = calculateDirectionalShadow(WorldPos, N, lights[i].direction);
    }

    Lo += (1.0 - shadow) * accumulateLight(lights[i], N, V, albedo, metallic, roughness, F0);
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

  vec3 finalColor = ambient + Lo;

  // HDR tonemapping
  finalColor = finalColor / (finalColor + vec3(1.0));

  // gamma correct
  finalColor = pow(finalColor, vec3(1.0 / 2.2));
  FragColor = vec4(finalColor, surfaceAlpha);
}
