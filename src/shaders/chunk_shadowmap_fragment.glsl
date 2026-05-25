#version 330 core

uniform sampler2D albedo_atlas;
// uniform sampler2D normal_atlas;
// uniform sampler2D specular_atlas;

in vec2 f_uv;

void main() {
  if (texture(albedo_atlas, f_uv).a == 0.)
    discard;
}