// Vertex shader
#version 150
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec4 a_position;

out vec3 v_texcoord;

uniform mat4 u_mvp;

void main()
{
    v_texcoord = 0.5 * a_position.xyz + 0.5;
    gl_Position = u_mvp * a_position;
}
