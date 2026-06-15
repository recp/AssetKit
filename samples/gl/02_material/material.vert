#version 330 core

layout(location=0) in vec3 a_pos;
layout(location=1) in vec2 a_uv;

uniform float u_angle;

out vec2 v_uv;

void main() {
  float c = cos(u_angle);
  float s = sin(u_angle);
  vec3 p = vec3(c * a_pos.x + s * a_pos.z,
                a_pos.y,
                -s * a_pos.x + c * a_pos.z);

  gl_Position = vec4(p.xy * 0.86, p.z * 0.15, 1.0);
  v_uv = a_uv;
}
