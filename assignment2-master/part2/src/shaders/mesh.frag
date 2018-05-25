// Fragment shader
#version 150

in vec3 v_color;

out vec4 frag_color;

void main()
{
	// Lambertian reflectance model:
    frag_color = vec4(v_color, 1.0);
}
