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
  return texture(noiseMap, fract(uv)).r;
}

vec2 noise_vec2(vec2 uv)
{
  return texture(noiseMap, fract(uv)).rg * 2.0 - 1.0;
}

struct RainLayerResult
{
  float mask;
  float highlight;
  vec2 offset;
};

RainLayerResult rain_layer(vec2 uv, float scale, float speed_scale, vec2 seed_offset)
{
  RainLayerResult result;
  result.mask = 0.0;
  result.highlight = 0.0;
  result.offset = vec2(0.0);

  vec2 p = uv * vec2(108.0 * scale, 30.0 * scale);
  p.x += p.y * tilt;
  p.y += time * speed * speed_scale;
  p.x += time * speed * distortion * 0.06;

  vec2 cell = floor(p);
  vec2 f = fract(p);

  vec2 seed_uv = (cell + seed_offset) / vec2(256.0, 256.0);
  float seed = noise_value(seed_uv);
  vec2 flow = noise_vec2(seed_uv + vec2(0.31, 0.67));
  float center = mix(0.10, 0.90, seed);
  float width = mix(0.018, 0.05, seed) / max(scale, 0.35);
  float trail_end = mix(0.48, 0.95, clamp(trailLength, 0.0, 1.0));
  float trail_start = max(0.02, trail_end - mix(0.22, 0.7, clamp(trailLength, 0.0, 1.0)));

  float x = f.x - center + flow.x * 0.08;
  float streak = smoothstep(width * 1.75, width * 0.35, abs(x));
  float trail = smoothstep(trail_start - 0.04, trail_start + 0.08, f.y) *
                (1.0 - smoothstep(trail_end - 0.06, trail_end, f.y));

  vec2 head_delta = vec2(x / max(width * 2.5, 0.0001), (f.y - trail_end) / 0.12);
  float head = 1.0 - smoothstep(0.35, 1.0, length(head_delta));

  float ripples = smoothstep(0.72, 0.96, noise_value(seed_uv * 3.7 + vec2(0.17, time * 0.03)));
  result.mask = (streak * trail + head * 0.9) * (0.45 + 0.55 * seed) + ripples * trail * 0.08;
  result.highlight = streak * smoothstep(trail_end - 0.18, trail_end, f.y) * (0.35 + 0.65 * seed) + head * 0.55;
  result.offset = vec2(x, -(f.y - trail_end) * 0.45 + flow.y * 0.15) *
                  result.mask *
                  (0.008 + 0.012 * seed) /
                  max(scale, 0.5);
  return result;
}

void main()
{
  vec4 scene = texture(hdrBuffer, TexCoords);
  vec2 pixel = 1.0 / max(screenResolution, vec2(1.0));
  float aspect = screenResolution.x / max(screenResolution.y, 1.0);
  vec2 uv = vec2(TexCoords.x * aspect, TexCoords.y);

  RainLayerResult rain0 = rain_layer(uv, 1.0, 1.0, vec2(0.5, 0.5));
  RainLayerResult rain1 = rain_layer(uv + vec2(0.31, 0.17), 1.65, 1.28, vec2(19.5, 7.5));
  RainLayerResult rain2 = rain_layer(uv + vec2(-0.23, 0.41), 2.4, 1.65, vec2(37.5, 29.5));

  float rain = clamp((rain0.mask + rain1.mask * 0.75 + rain2.mask * 0.45) * density, 0.0, 1.0);
  float highlight = clamp((rain0.highlight + rain1.highlight * 0.7 + rain2.highlight * 0.4) * density, 0.0, 1.0);
  vec2 refraction = (rain0.offset + rain1.offset * 0.75 + rain2.offset * 0.45) * distortion * 0.85;

  vec2 refract_uv = clamp(TexCoords + refraction, pixel * 0.5, vec2(1.0) - pixel * 0.5);
  vec3 refracted_scene = texture(hdrBuffer, refract_uv).rgb;
  vec3 softened_scene =
    refracted_scene +
    texture(hdrBuffer, clamp(refract_uv + vec2(pixel.x, 0.0), pixel * 0.5, vec2(1.0) - pixel * 0.5)).rgb +
    texture(hdrBuffer, clamp(refract_uv - vec2(pixel.x, 0.0), pixel * 0.5, vec2(1.0) - pixel * 0.5)).rgb +
    texture(hdrBuffer, clamp(refract_uv + vec2(0.0, pixel.y), pixel * 0.5, vec2(1.0) - pixel * 0.5)).rgb +
    texture(hdrBuffer, clamp(refract_uv - vec2(0.0, pixel.y), pixel * 0.5, vec2(1.0) - pixel * 0.5)).rgb;
  softened_scene *= 0.2;

  float mist = density * (0.04 + 0.06 * noise_value(uv * vec2(4.0, 2.6) + vec2(0.0, time * 0.02)));
  vec3 color = mix(scene.rgb, softened_scene, rain * 0.42);
  color = mix(color, color * 0.94 + tint * 0.08, mist);
  color *= mix(1.0, 0.93, density * 0.16);
  color += tint * (rain * 0.12 + highlight * brightness);
  FragColor = vec4(color, scene.a);
}
