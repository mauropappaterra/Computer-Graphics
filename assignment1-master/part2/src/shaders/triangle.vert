// Vertex shader
#version 150
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec3 a_color;

out vec3 v_color; // part 2 (1)

void main()
{
    gl_Position = vec4(-a_position.xyz, 1.0); // part 1 (3)
	//gl_Position = a_position;
	//gl_Position = vec4(a_position.xyz, 1.0); 
    //gl_Position = vec4(a_position.xyz * 2, 1.0); 
	//gl_Position = vec4(a_position.xyz / 2, 1.0);

	v_color = vec3(0.3, 1.0, 0.9); // part 2 (1)

	v_color = a_color; // part 2 (2)

}
