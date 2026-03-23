#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform samplerCube cubemapTexture;
uniform float lod;

const float PI = 3.14159265359;

vec3 sample_direction(vec2 uv)
{
    float phi = uv.x * 2.0 * PI - PI;
    float theta = (0.5 - uv.y) * PI;
    float cosTheta = cos(theta);
    return normalize(vec3(
        cosTheta * cos(phi),
        sin(theta),
        cosTheta * sin(phi)));
}

void main()
{
    vec3 direction = sample_direction(TexCoords);
    vec3 color = textureLod(cubemapTexture, direction, lod).rgb;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
