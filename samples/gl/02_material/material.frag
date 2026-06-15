#version 330 core

in vec2 v_uv;

uniform vec4 u_base_color;
uniform bool u_has_base_texture;
uniform sampler2D u_base_texture;

out vec4 frag_color;

void main() {
  vec4 base = u_base_color;
  if (u_has_base_texture)
    base *= texture(u_base_texture, v_uv);
  frag_color = vec4(clamp(base.rgb, 0.0, 1.0), base.a);
}
