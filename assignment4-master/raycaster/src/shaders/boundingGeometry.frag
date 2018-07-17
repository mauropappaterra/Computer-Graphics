// Fragment shader
#version 150

in vec3 v_texcoord;

out vec4 frag_color;

void main()
{
    frag_color = vec4(v_texcoord, 1.0);
}
