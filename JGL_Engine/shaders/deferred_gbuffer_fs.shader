#version 330 core

layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormalRoughness;
layout(location = 2) out vec4 gAlbedoMetallic;

in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 color = vec3(1.0);
uniform sampler2D baseMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D normalMap;
uniform sampler2D aoMap;

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

void main()
{
    // Store linear albedo so the lighting pass matches the forward PBR shader.
    vec3 albedo = pow(texture(baseMap, TexCoords).rgb, vec3(2.2)) * color;
    float metallic = texture(metallicMap, TexCoords).r;
    float roughness = texture(roughnessMap, TexCoords).r;
    float ao = texture(aoMap, TexCoords).r;
    vec3 normal = getNormalFromMap();

    gPosition = vec4(WorldPos, ao);
    gNormalRoughness = vec4(normalize(normal), roughness);
    gAlbedoMetallic = vec4(albedo, metallic);
}
