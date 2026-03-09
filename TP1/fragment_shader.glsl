#version 430 core

uniform sampler2D texture_sampler;
uniform int texture_i;

in vec3 f_position;
in vec3 f_position_world_space;
in vec3 f_normal;
in vec2 f_uv;

// Ouput data
out vec3 color;

void main() {
  if (texture_i >= 0) {
    color = texture(texture_sampler, f_uv).rgb;
  } else {
    // color = vec3(1.);
    color = (f_normal + vec3(1.)) / vec3(2.);
  }
}
