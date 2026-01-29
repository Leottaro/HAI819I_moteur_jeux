#version 430 core

layout(location = 0) uniform sampler2D grass;
layout(location = 1) uniform sampler2D rock;
layout(location = 2) uniform sampler2D snowrocks;
layout(location = 3) uniform sampler2D heightmap_mountain;
layout(location = 4) uniform sampler2D heightmap_rocky;
layout(location = 5) uniform sampler2D heightmap_test;

in vec2 fragment_uv;
in vec3 fragment_position_modelspace;

// Ouput data
out vec3 color;

void main(){
    if (fragment_position_modelspace.y < 0.3) {
        color = texture(grass, fragment_uv).rgb;
    } else if (fragment_position_modelspace.y < 0.6) {
        color = texture(rock, fragment_uv).rgb;
    } else {
        color = texture(snowrocks, fragment_uv).rgb;
    }
}
