// Vertex shader
#version 150
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec3 a_normal;

out vec3 v_normal;
out vec4 v_color;

uniform vec4 u_ambient; 
uniform vec4 u_diffuse; 
uniform vec4 u_specular;
uniform mat4 u_mvp;
uniform mat4 u_mv;
uniform vec4 u_lightpos;
uniform float u_shininess;
uniform int u_ambient_toggle;
uniform int u_diffuse_toggle;
uniform int u_specular_toggle;
uniform int u_gamma_toggle;
uniform int u_normal_toggle;
uniform int u_cubemap_toggle;
uniform samplerCube u_cubemap;

void main()
{
    v_normal = a_normal;
    gl_Position = u_mvp * a_position;

	vec3 N = normalize(mat3(u_mv) * a_normal.xyz);
	vec3 L = normalize(u_lightpos.xyz - (u_mv * a_position).xyz);
	vec3 V = -normalize((u_mv * a_position).xyz);
	vec3 H = normalize(L+V);
	vec3 R = reflect(-V, N);
	
	vec4 ambient, diffuse, specular;
	if(u_ambient_toggle){
		ambient = u_ambient;
	}
	if(u_diffuse_toggle){
		diffuse = u_diffuse * max(dot(L, N), 0.0);
	}
	if(u_specular_toggle){
		specular = ((u_shininess+8.0f)/8.0f) * u_specular * pow(max(dot(N, H), 0.0), u_shininess);
	}
	
	vec4 cube_color;
	if(u_cubemap_toggle){
		cube_color = vec4(texture(u_cubemap, R).rgb, 0);
	}

	if(u_normal_toggle){
		v_color = vec4(v_normal, 0);
	} else {
		v_color = vec4((ambient + diffuse + specular + cube_color).xyz, 1.0);
	}
	
}