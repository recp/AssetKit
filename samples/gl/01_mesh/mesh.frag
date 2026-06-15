#version 330 core

in vec3 v_color;

out vec4 frag_color;

void main() {
  frag_color = vec4(clamp(v_color, 0.0, 1.0), 1.0);
}
