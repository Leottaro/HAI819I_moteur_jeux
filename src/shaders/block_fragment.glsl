#version 430 core

uniform sampler2D block_atlas;

// in vec3 f_position;
// in vec3 f_position_world_space;
// in vec3 f_normal;
in vec2 f_uv;

// Ouput data
out vec4 out_color;

void main() {
  out_color = texture(block_atlas, f_uv).rgba;

  // Discard fully transparent fragments to prevent depth buffer artifacts
  if (out_color.a == 0.0) {
    discard;
  }
}
