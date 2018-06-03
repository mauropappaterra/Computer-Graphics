// Vertex shader
#version 150
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec3 a_normal;

out vec3 v_normal;
out vec4 v_color;

uniform mat4 u_mvp; //Model View Projection
uniform mat4 u_mv;	//Model View
uniform vec4 u_lightpos; //Position of Light

void main()
{
    v_normal = a_normal;
    gl_Position = u_mvp * a_position;

	vec3 N = normalize(mat3(u_mv) * a_normal.xyz);
	vec3 L = normalize(u_lightpos.xyz - (u_mv * a_position).xyz);
	vec3 V = -normalize((u_mv * a_position).xyz);
	vec3 H = normalize(L+V);
	vec3 R = reflect(-V, N);

}
