#version 330 core

layout(location = 0) in ivec3 v_position;
layout(location = 1) in ivec3 v_normal;
layout(location = 2) in vec2 v_uv;

uniform ivec3 chunk_pos;
uniform mat4 projection, view;

// out vec3 f_position;
out vec3 f_worldpos;
out vec3 f_normal;
out vec2 f_uv;

void main() {
  ivec3 world_pos = chunk_pos + v_position;
  vec4 p = view * vec4(world_pos, 1.0);
  gl_Position = projection * p;

  f_worldpos = world_pos;
  // f_position = p.xyz / p.w;
  f_normal = v_normal;
  f_uv = v_uv;
}