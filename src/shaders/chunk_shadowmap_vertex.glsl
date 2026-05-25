#version 330 core

layout(location = 0) in ivec3 v_position;
// layout(location = 1) in ivec3 v_normal;
// layout(location = 2) in ivec3 v_tangent;
// layout(location = 3) in ivec3 v_bitangent;
layout(location = 4) in vec2 v_uv;
uniform ivec3 chunk_pos;

uniform mat4 VP;

out vec2 f_uv;

void main() {
  gl_Position = VP * vec4(v_position + chunk_pos, 1);
  f_uv = v_uv;
}
