// Fragment shader
#version 150

in vec3 v_normal;

out vec4 frag_color;

void main()
{
    vec3 N = normalize(v_normal);
    frag_color = vec4(0.5 * N + 0.5, 1.0);
}
