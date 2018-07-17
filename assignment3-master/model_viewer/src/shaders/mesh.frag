// Fragment shader
#version 150

in vec3 v_normal;
in vec4 v_color;

out vec4 frag_color;
varying vec3 N;
varying vec3 L;
varying vec3 V;

uniform int u_gamma_toggle;

void main()
{
	if(u_gamma_toggle){
		frag_color.xyz = pow(v_color.xyz, vec3(1 / 2.2));
	} else {
		frag_color = v_color;
	}
}