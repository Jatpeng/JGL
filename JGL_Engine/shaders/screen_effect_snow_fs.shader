#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D hdrBuffer;

uniform float time;
uniform vec2 screenResolution;

uniform vec3 flakeColor;
uniform float density;
uniform float speed;
uniform float size;
uniform float drift;
uniform float softness;
uniform float brightness;

float hash12(vec2 p)
{
  vec3 p3 = fract(vec3(p.xyx) * 0.1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.x + p3.y) * p3.z);
}

float snow_cell(vec2 uv, float scale, float speed_scale)
{
  vec2 p = uv * scale;
  p.y += time * speed * speed_scale;

  vec2 cell = floor(p);
  vec2 f = fract(p) - 0.5;

  float seed = hash12(cell);
  float sway = sin(time * (0.8 + seed) + cell.y * 0.37 + seed * 6.2831853) * drift;
  f.x += sway;
  f.y += seed - 0.5;

  float radius = (0.05 + seed * 0.07) * max(size, 0.05);
  float edge = max(radius * softness, 0.001);
  float flake = smoothstep(radius, radius - edge, length(f));
  return flake * (0.45 + 0.55 * seed);
}

void main()
{
  vec4 scene = texture(hdrBuffer, TexCoords);
  float aspect = screenResolution.x / max(screenResolution.y, 1.0);
  vec2 uv = vec2(TexCoords.x * aspect, TexCoords.y);

  float layer0 = snow_cell(uv, 18.0, 0.8);
  float layer1 = snow_cell(uv + vec2(0.27, 0.11), 28.0, 0.55);
  float layer2 = snow_cell(uv + vec2(0.53, 0.37), 40.0, 0.35);
  float snow = clamp((layer0 + layer1 * 0.7 + layer2 * 0.45) * density, 0.0, 1.0);

  vec3 color = mix(scene.rgb, scene.rgb * 0.98 + vec3(0.02, 0.03, 0.04), density * 0.15);
  color += flakeColor * snow * brightness;
  FragColor = vec4(color, scene.a);
}
