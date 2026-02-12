#version 430 core

in vec3 f_position;
in vec3 f_position_world_space;
in vec3 f_normal;
in vec2 f_uv;

// Ouput data
out vec3 color;

void main(){
    color = (f_normal + vec3(1.)) / vec3(2.);
}
