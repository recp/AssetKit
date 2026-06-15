#version 330 core

#define MAX_LIGHTS 16
#define LIGHT_AMBIENT 1
#define LIGHT_DIRECTIONAL 2
#define LIGHT_POINT 3
#define LIGHT_SPOT 4

struct SampleLight {
  int type;
  vec3 position;
  vec3 direction;
  vec3 color;
  float intensity;
  float range;
  float inner_cos;
  float outer_cos;
};

in vec3 v_pos;
in vec3 v_normal;
in vec2 v_uv;

uniform float u_angle;
uniform vec4 u_base_color;
uniform bool u_has_base_texture;
uniform sampler2D u_base_texture;
uniform int u_light_count;
uniform SampleLight u_lights[MAX_LIGHTS];

out vec4 frag_color;

mat3 rotate_y_mat(float angle) {
  float c = cos(angle);
  float s = sin(angle);
  return mat3(c, 0.0, -s,
              0.0, 1.0, 0.0,
              s, 0.0, c);
}

void main() {
  vec4 base = u_base_color;
  if (u_has_base_texture)
    base *= texture(u_base_texture, v_uv);

  vec3 n = normalize(v_normal);
  vec3 lit = vec3(0.02);
  mat3 rot = rotate_y_mat(u_angle);

  for (int i = 0; i < u_light_count; i++) {
    SampleLight light = u_lights[i];
    vec3 light_color = light.color * max(light.intensity, 0.0);

    if (light.type == LIGHT_AMBIENT) {
      lit += light_color;
    } else if (light.type == LIGHT_DIRECTIONAL) {
      vec3 l = normalize(-(rot * light.direction));
      lit += light_color * max(dot(n, l), 0.0);
    } else {
      vec3 lp = rot * light.position;
      vec3 to_light = lp - v_pos;
      float dist = length(to_light);
      vec3 l = dist > 0.0 ? to_light / dist : vec3(0.0, 0.0, 1.0);
      float attenuation = 1.0;
      if (light.range > 0.0)
        attenuation = clamp(1.0 - dist / light.range, 0.0, 1.0);

      if (light.type == LIGHT_SPOT) {
        vec3 spot_dir = normalize(rot * light.direction);
        float cone = dot(-l, spot_dir);
        float width = max(light.inner_cos - light.outer_cos, 0.001);
        attenuation *= clamp((cone - light.outer_cos) / width, 0.0, 1.0);
      }

      lit += light_color * max(dot(n, l), 0.0) * attenuation;
    }
  }

  frag_color = vec4(clamp(base.rgb * lit, 0.0, 1.0), base.a);
}
