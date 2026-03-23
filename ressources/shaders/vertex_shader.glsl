#version 330 core

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;

uniform mat4 projection, view, model;

out vec3 f_position;
out vec3 f_position_world_space;
out vec3 f_normal;
out vec2 f_uv;

void main() {
  f_position_world_space = v_position;
  mat4 model_view = view * model;
  vec4 p = model_view * vec4(v_position, 1.0);
  gl_Position = projection * p;

  vec4 n = model * vec4(v_normal, 0.0);

  f_position = p.xyz / p.w;
  f_normal = normalize(n.xyz);
  f_uv = v_uv;
}