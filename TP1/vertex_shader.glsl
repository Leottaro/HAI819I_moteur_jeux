#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertices_position_modelspace;
layout(location = 1) in vec2 vertices_uv;

uniform mat4 MVP;
out vec2 uv;

void main(){
    gl_Position = MVP * vec4(vertices_position_modelspace,1);
    uv = vertices_uv;
}

