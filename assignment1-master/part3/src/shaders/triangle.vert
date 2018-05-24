// Vertex shader
#version 150
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec4 a_position;
//layout(location = 1) in vec3 a_color;

uniform float u_time;
out vec3 v_color;

void main() {
    v_color = vec3(0.5, sin(u_time), sin(u_time));
    gl_Position = a_position;
}
