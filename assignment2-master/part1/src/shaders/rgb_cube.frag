// Fragment shader
#version 150

out vec4 frag_color;
in vec3 v_color;

void main()
{
    frag_color = vec4(vec3(v_color), 1.0);
}