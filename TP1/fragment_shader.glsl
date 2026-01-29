#version 330 core

uniform sampler2D sampler;
in vec2 uv;

// Ouput data
out vec3 color;

void main(){
//    color = vec3(0.3, 0.3, 0.3);
    color = texture(sampler, uv).rgb;
}
