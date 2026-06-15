#version 330 core

layout(location=0) in vec3 a_pos;
layout(location=1) in vec3 a_normal;
layout(location=2) in vec2 a_uv;

uniform float u_angle;

out vec3 v_pos;
out vec3 v_normal;
out vec2 v_uv;

mat3 rotate_y_mat(float angle) {
  float c = cos(angle);
  float s = sin(angle);
  return mat3(c, 0.0, -s,
              0.0, 1.0, 0.0,
              s, 0.0, c);
}

void main() {
  mat3 r = rotate_y_mat(u_angle);
  vec3 p = r * a_pos;

  v_pos = p;
  v_normal = normalize(r * a_normal);
  v_uv = a_uv;
  gl_Position = vec4(p.xy * 0.86, p.z * 0.15, 1.0);
}
