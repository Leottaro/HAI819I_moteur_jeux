#version 330 core

uniform sampler2DArray albedo_atlas;
// uniform sampler2D normal_atlas;
// uniform sampler2D specular_atlas;

in vec2 f_uv;
flat in uint f_block_tex;

void main() {
  if (texture(albedo_atlas, vec3(f_uv, f_block_tex)).a == 0.)
    discard;
}