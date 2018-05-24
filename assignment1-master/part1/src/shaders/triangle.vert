// Vertex shader
#version 150
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec3 a_position;

void main()
{
    gl_Position = vec4(-a_position.xyz, 1.0); // part 1 (3)
	//gl_Position = a_position;
	//gl_Position = vec4(a_position.xyz, 1.0); 
    //gl_Position = vec4(a_position.xyz * 2, 1.0); 
	//gl_Position = vec4(a_position.xyz / 2, 1.0);

}
