// Assignment 2, Part 1
//
// Modify this file and the rgb_cube.vert and rgb_cube.frag shaders to
// implement the spinning RGB cube.
//

#include "utils.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <cstdlib>

// The attribute locations we will use in the vertex shader
enum AttributeLocation {
    POSITION = 0,
    COLOR = 1
};

// Struct for resources and state
struct Context {
    int width;
    int height;
    GLFWwindow *window;
    GLuint program;
    GLuint cubeVAO;
    GLuint cubeVBO;
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
    std::string rootDir = getEnvVar("ASSIGNMENT2_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: ASSIGNMENT2_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/part1/src/shaders/";
}

void errorCallback(int /*error*/, const char* description)
{
    std::cerr << description << std::endl;
}

// MODIFY THIS FUNCTION
void createCube(Context &ctx)
{
    // MODIFY THIS PART: Define the six faces (front, back, left,
    // right, top, and bottom) of the cube. Each face should be
    // constructed from two triangles, and each triangle should be
    // constructed from three vertices. That is, you should define 36
    // vertices that together make up 12 triangles. One triangle is
    // given; you have to define the rest!
	const GLfloat vertices[] = {
		// front face
		-0.5f, -0.5f,  0.5f, // first triangle starts here
		0.5f, -0.5f,  0.5f,
		0.5f,  0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f, // second triangle starts here
		-0.5f, 0.5f,  0.5f,
		0.5f,  0.5f,  0.5f,
		// back face
		-0.5f, -0.5f,  -0.5f, // first triangle starts here
		0.5f, -0.5f,  -0.5f,
		0.5f,  0.5f,  -0.5f,
		-0.5f, -0.5f,  -0.5f, // second triangle starts here
		-0.5f, 0.5f,  -0.5f,
		0.5f,  0.5f,  -0.5f,
		// left face
		-0.5f, -0.5f,  0.5f, // first triangle starts here
		-0.5f, 0.5f,  0.5f,
		-0.5f,  0.5f,  -0.5f,
		-0.5f, -0.5f,  0.5f, // second triangle starts here
		-0.5f, -0.5f,  -0.5f,
		-0.5f,  0.5f,  -0.5f,
		// right face
		0.5f, -0.5f,  0.5f, // first triangle starts here
		0.5f, 0.5f,  0.5f,
		0.5f,  0.5f,  -0.5f,
		0.5f, -0.5f,  0.5f, // second triangle starts here
		0.5f, -0.5f,  -0.5f,
		0.5f,  0.5f,  -0.5f,
		// top face
		-0.5f, 0.5f,  0.5f, // first triangle starts here
		0.5f, 0.5f,  0.5f,
		0.5f,  0.5f,  -0.5f,
		-0.5f, 0.5f,  0.5f, // second triangle starts here
		0.5f, 0.5f,  -0.5f,
		-0.5f,  0.5f,  -0.5f,
		// bottom face
		-0.5f, -0.5f,  0.5f, // first triangle starts here
		0.5f, -0.5f,  0.5f,
		0.5f,  -0.5f,  -0.5f,
		-0.5f, -0.5f,  0.5f, // second triangle starts here
		0.5f, -0.5f,  -0.5f,
		-0.5f,  -0.5f,  -0.5f
	};

    // Generates and populates a vertex buffer object (VBO) for the
    // vertices (DO NOT CHANGE THIS)
    glGenBuffers(1, &ctx.cubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, ctx.cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Creates a vertex array object (VAO) for drawing the cube
    // (DO NOT CHANGE THIS)
    glGenVertexArrays(1, &ctx.cubeVAO);
    glBindVertexArray(ctx.cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, ctx.cubeVBO);
    glEnableVertexAttribArray(POSITION);
    glVertexAttribPointer(POSITION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(ctx.defaultVAO); // unbinds the VAO
}

void init(Context &ctx)
{
    ctx.program = loadShaderProgram(shaderDir() + "rgb_cube.vert",
                                    shaderDir() + "rgb_cube.frag");

    createCube(ctx);
}

// MODIFY THIS FUNCTION
void drawCube(Context &ctx)
{
    glUseProgram(ctx.program);

    double elapsed_time = glfwGetTime();

	// angles that increase with elapsed time
	float angle_a = (float)(glm::sin(elapsed_time) + 1) * 3.14;
	float angle_b = (float)(glm::cos(elapsed_time) + 1) * 3.14;

	glm::vec3 axis = glm::vec3(angle_a, angle_b, 0);

	// Define the model, view, and projection matrices here

	// 1) model = position, rotation, scale. Identity Matrix -> glm::mat4 model = glm::mat4(1.0f);

	//glm::mat4 model = glm::translate(glm::mat4(1.0f), 1.0, glm::vec3(0.0, 1.5, 1.0));

	// Rotate
	glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle_a, axis);
	//glm::mat4 model = glm::rotate(glm::mat4(1.0f), angle_b, axis);

	// Scale
	//glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
	//glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
	//glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(1.2f));
	//glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));


	// 2) view of the camera. Identity Matrix -> glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 view = glm::lookAt(
		glm::vec3(1, 0, 3), // EYE: Position of camera in world (x,y,z)
		glm::vec3(0, 0, 0), // AT: Position the camera is looking at (origin in this case)
		glm::vec3(0, 1, 0)  // UP: Head's up  (Use -> (0, -1, 0) for head's down)
	);

	// 3) projection, 'orthogonal' or 'perspective' projection. Identity Matrix ->	glm::mat4 projection = glm::mat4(1.0f);
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);

	// Concatenate the model, view, and projection matrices to a
    // ModelViewProjection (MVP) matrix and pass it as a uniform
    // variable to the shader program.

	glm::mat4 mvp = projection * view * model; // this is done by multiplying them together in this order (reversed!)

    //
    // Hint: you pass GLM matrices to shader programs like this:
    //glUniformMatrix4fv(glGetUniformLocation(program, "u_mvp"), 1, GL_FALSE, &mvp[0][0]);

	glUniformMatrix4fv(glGetUniformLocation(ctx.program, "u_mvp"), 1, GL_FALSE, &mvp[0][0]);


    glBindVertexArray(ctx.cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36); // <-- change this third parameter to the number of total vertices (36)
	glBindVertexArray(ctx.defaultVAO);

    glUseProgram(0);
}

void display(Context &ctx)
{
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST); // ensures that polygons overlap correctly
    drawCube(ctx);
}

void reloadShaders(Context *ctx)
{
    glDeleteProgram(ctx->program);
    ctx->program = loadShaderProgram(shaderDir() + "rgb_cube.vert",
                                     shaderDir() + "rgb_cube.frag");
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
    glfwSetErrorCallback(errorCallback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    ctx.width = 500;
    ctx.height = 500;
    ctx.window = glfwCreateWindow(ctx.width, ctx.height, "RGB cube", nullptr, nullptr);
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
