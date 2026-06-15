#version 330 core

layout(location=0) in vec3 a_pos;

uniform float u_angle;

out vec3 v_color;

void main() {
  float c = cos(u_angle);
  float s = sin(u_angle);
  vec3 p = vec3(c * a_pos.x + s * a_pos.z,
                a_pos.y,
                -s * a_pos.x + c * a_pos.z);

  gl_Position = vec4(p.xy * 0.86, p.z * 0.15, 1.0);
  v_color = vec3(0.55, 0.66, 0.78) + p.y * vec3(0.12, 0.08, 0.02);
}
