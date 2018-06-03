// Fragment shader
#version 150

in vec3 v_normal;
in vec4 v_color;

out vec4 frag_color;
varying vec3 N;
varying vec3 L;
varying vec3 V;

void main()
{
    vec3 N = normalize(v_normal);
    frag_color.xyz = pow(v_color.xyz, vec3(1 / 2.2)); // = vec4(0.5 * N + 0.5, 1.0);
}
