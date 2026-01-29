#version 430 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertices_position_modelspace;
layout(location = 1) in vec2 vertices_uv;

uniform mat4 MVP;
out vec2 fragment_uv;
out vec3 fragment_position_modelspace;

void main(){
    gl_Position = MVP * vec4(vertices_position_modelspace,1);
    fragment_uv = vertices_uv;
    fragment_position_modelspace = vertices_position_modelspace;
}

