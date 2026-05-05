#version 330 core

layout(location = 0) in vec3 v_position;

uniform mat4 projection, view;
uniform vec3 position;

void main() {
  gl_Position = (projection * view) * vec4(position + v_position, 1.0);
}
