#version 430 core

layout(location = 0) uniform sampler2D grass;
layout(location = 1) uniform sampler2D rock;
layout(location = 2) uniform sampler2D snowrocks;
layout(location = 3) uniform sampler2D heightmap_mountain;
layout(location = 4) uniform sampler2D heightmap_rocky;
layout(location = 5) uniform sampler2D heightmap_test;

in vec3 f_position;
in vec3 f_position_world_space;
in vec3 f_normal;
in vec2 f_uv;

// Ouput data
out vec3 color;

void main(){
    if (f_position_world_space.y < 0.3) {
        color = texture(grass, f_uv).rgb;
    } else if (f_position_world_space.y < 0.6) {
        color = texture(rock, f_uv).rgb;
    } else {
        color = texture(snowrocks, f_uv).rgb;
    }
}
