#version 330 core

uniform sampler2D screen_texture;

uniform vec4 added_color;
uniform vec2 screen_size;

out vec4 out_color;

void main() {
  vec2 uv = gl_FragCoord.xy / screen_size;
  vec4 original_color = texture(screen_texture, uv);
  out_color = original_color + added_color;
}