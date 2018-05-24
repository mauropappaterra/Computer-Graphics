// Fragment shader
#version 150

in vec3 v_color; // part 2 (1)
out vec4 frag_color;

void main()
{
    //frag_color = vec4(0.5, 0.3, 1.0, 1.0);
	frag_color = vec4(v_color, 1.0); // part 2 (1)
}

