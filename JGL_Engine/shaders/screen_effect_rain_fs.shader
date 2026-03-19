#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D hdrBuffer;
uniform sampler2D noiseMap;

uniform float time;
uniform vec2 screenResolution;

uniform vec3 tint;
uniform float density;
uniform float speed;
uniform float tilt;
uniform float brightness;
uniform float trailLength;
uniform float distortion;

float noise_value(vec2 uv)
{
  return texture(noiseMap, uv).r;
}

float rain_layer(vec2 uv, float scale, float speed_scale)
{
  vec2 p = uv * vec2(130.0 * scale, 36.0 * scale);
  p.x += p.y * tilt;
  p.y += time * speed * speed_scale;
  p.x += time * speed * distortion * 0.15;

  vec2 cell = floor(p);
  vec2 f = fract(p);

  float seed = noise_value((cell + 0.5) / vec2(256.0, 256.0));
  float center = mix(0.10, 0.90, seed);
  float streak = smoothstep(0.055, 0.0, abs(f.x - center));
  float trail = smoothstep(1.0, max(0.02, 1.0 - trailLength), f.y);
  return streak * trail * (0.4 + 0.6 * seed);
}

void main()
{
  vec4 scene = texture(hdrBuffer, TexCoords);
  float aspect = screenResolution.x / max(screenResolution.y, 1.0);
  vec2 uv = vec2(TexCoords.x * aspect, TexCoords.y);

  float rain0 = rain_layer(uv, 1.0, 1.0);
  float rain1 = rain_layer(uv + vec2(0.31, 0.17), 1.8, 1.35);
  float rain = clamp((rain0 + rain1 * 0.75) * density, 0.0, 1.0);

  vec3 color = scene.rgb * mix(1.0, 0.95, density * 0.12);
  color += tint * rain * brightness;
  FragColor = vec4(color, scene.a);
}
