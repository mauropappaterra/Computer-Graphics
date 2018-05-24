// Assignment 1, Part 1

#include "utils.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cstdlib>

// The attribute locations we will use in the vertex shader
enum AttributeLocation {
	POSITION = 0
};

// Struct for resources
struct Context {
	int width;
	int height;
	GLFWwindow *window;
	GLuint program;
	GLuint triangleVAO;
	GLuint defaultVAO;
};

// Returns the value of an environment variable
std::string getEnvVar(const std::string &name)
{
	char const* value = std::getenv(name.c_str());
	if (value == nullptr) {
		return std::string();
	}
	else {
		return std::string(value);
	}
}

// Returns the absolute path to the shader directory
std::string shaderDir(void)
{
	std::string rootDir = getEnvVar("ASSIGNMENT1_ROOT");
	if (rootDir.empty()) {
		std::cout << "Error: ASSIGNMENT1_ROOT is not set." << std::endl;
		std::exit(EXIT_FAILURE);
	}
	return rootDir + "/part1/src/shaders/";
}

GLuint createTriangleVAO()
{
	const GLfloat vertices[] = {
		0.0f, 0.5f, 0.0f,
		-0.5f,-0.5f, 0.0f,
		0.5f,-0.5f, 0.0f,
	};

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(POSITION);
	glVertexAttribPointer(POSITION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	return vao;
}

void init(Context &ctx)
{
	ctx.program = loadShaderProgram(shaderDir() + "triangle.vert",
		shaderDir() + "triangle.frag");

	ctx.triangleVAO = createTriangleVAO();
}

void drawTriangle(GLuint program, GLuint vao)
{
	glUseProgram(program);

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glUseProgram(0);
}

void display(Context &ctx)
{
	glClearColor(0.9, 0.5, 0.6, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawTriangle(ctx.program, ctx.triangleVAO);
}

void reloadShaders(Context *ctx)
{
	glDeleteProgram(ctx->program);
	ctx->program = loadShaderProgram(shaderDir() + "triangle.vert",
		shaderDir() + "triangle.frag");
}

void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
	Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));

	if (key == GLFW_KEY_R && action == GLFW_PRESS) {
		reloadShaders(ctx);
	}
}

void resizeCallback(GLFWwindow* window, int width, int height)
{
	Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
	ctx->width = width;
	ctx->height = height;
	glViewport(0, 0, width, height);
}

int main(void)
{
	Context ctx;

	// Create a GLFW window
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	ctx.width = 800;
	ctx.height = 800;
	ctx.window = glfwCreateWindow(ctx.width, ctx.height, "Triangle", nullptr, nullptr);
	glfwMakeContextCurrent(ctx.window);
	glfwSetWindowUserPointer(ctx.window, &ctx);
	glfwSetFramebufferSizeCallback(ctx.window, resizeCallback);
	glfwSetKeyCallback(ctx.window, keyCallback);

	// Load OpenGL functions
	glewExperimental = true;
	GLenum status = glewInit();
	if (status != GLEW_OK) {
		std::cerr << "Error: " << glewGetErrorString(status) << std::endl;
		std::exit(EXIT_FAILURE);
	}
	std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

	// Initialize rendering
	glGenVertexArrays(1, &ctx.defaultVAO);
	glBindVertexArray(ctx.defaultVAO);
	init(ctx);

	// Start rendering loop
	while (!glfwWindowShouldClose(ctx.window)) {
		glfwPollEvents();
		display(ctx);
		glfwSwapBuffers(ctx.window);
	}

	// Shutdown
	glfwDestroyWindow(ctx.window);
	glfwTerminate();
	std::exit(EXIT_SUCCESS);
}
