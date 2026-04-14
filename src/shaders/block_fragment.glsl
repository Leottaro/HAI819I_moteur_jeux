#version 430 core

uniform sampler2D block_atlas;

// in vec3 f_position;
// in vec3 f_position_world_space;
// in vec3 f_normal;
in vec2 f_uv;

// Ouput data
out vec3 color;

void main() {
  color = texture(block_atlas, f_uv).rgb;
}
