#version 430 core

void main() {
  vec2 vertices[6] = vec2[](
      vec2(-1.0, -1.0), // Bottom-left
      vec2(1.0, -1.0),  // Bottom-right
      vec2(-1.0, 1.0),  // Top-left
      vec2(1.0, -1.0),  // Bottom-right
      vec2(1.0, 1.0),   // Top-right
      vec2(-1.0, 1.0)   // Top-left
  );

  gl_Position = vec4(vertices[gl_VertexID], 0.0, 1.0);
}
