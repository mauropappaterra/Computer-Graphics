// Assignment 4, Ray-casting Project
//
// Modify this file according to the lab instructions.
//

#include "utils.h"
#include "utils2.h"
#include "cgVolume.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <cstdlib>
#include <algorithm>

// The attribute locations we will use in the vertex shader
enum AttributeLocation {
    POSITION = 0,
    NORMAL = 1,
    TEXCOORD = 2,
};

// Struct for representing an indexed triangle mesh
struct Mesh {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;
};

// Struct for representing a vertex array object (VAO) created from a
// mesh. Used for rendering.
struct MeshVAO {
    GLuint vao;
    GLuint vertexVBO;
    GLuint normalVBO;
    GLuint indexVBO;
    int numVertices;
    int numIndices;
};

// Struct for representing a volume used for ray-casting.
struct RayCastVolume {
    cg::VolumeBase volume;
    GLuint volumeTexture;
    GLuint frontFaceFBO;
    GLuint backFaceFBO;
    GLuint frontFaceTexture;
    GLuint backFaceTexture;

    RayCastVolume() :
        volumeTexture(0),
        frontFaceFBO(0),
        backFaceFBO(0),
        frontFaceTexture(0),
        backFaceTexture(0)
    {}
};

// Struct for resources and state
struct Context {
    int width;
    int height;
    float aspect;
    GLFWwindow *window;
    Trackball trackball;
    Mesh cubeMesh;
    MeshVAO cubeVAO;
    MeshVAO quadVAO;
    GLuint defaultVAO;
    RayCastVolume rayCastVolume;
    GLuint boundingGeometryProgram;
    GLuint rayCasterProgram;
    float elapsed_time;
     // Resources used by imgui
     const char* dataset[4] = {"foot.vtk", "abdomen.vtk", "bonsai.vtk", "tooth.vtk"};
     int dataset_current = 0;
     int dataset_changed = -1; // used to (re)load volume dataset in gui
     int mode = 0; //Arbitrarily set to 0: Alpha Blending, 1: MIP.
     float step_size = 0.005f;
     // Transfer function colors
     glm::vec3 tf1;
     glm::vec3 tf2;
     glm::vec3 tf3;
     glm::vec3 tf4;
     // Intensity thresholds
     float tf1_intensity;
     float tf2_intensity;
     float tf3_intensity;
     float tf4_intensity;
     // tf1_alpha is grayscale opacity and tf2_alpha is color opacity.
     float tf1_alpha;
     float tf2_alpha;
     // background color
     glm::vec3 background = glm::vec3(0.2f, 0.2f, 0.2f);
     // sampling rate is a constant to scale stepsize in fragment shader
     float sample_rate = 200.0f;
     bool correction_enable = true;
     int correction = 1;
     float correction_threshold = 0.02;

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
    std::string rootDir = getEnvVar("ASSIGNMENT4_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: ASSIGNMENT4_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/raycaster/src/shaders/";
}

// Returns the absolute path to the 3D model directory
std::string modelDir(void)
{
    std::string rootDir = getEnvVar("ASSIGNMENT4_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: ASSIGNMENT4_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/raycaster/3d_models/";
}

// Returns the absolute path to the cubemap texture directory
std::string cubemapDir(void)
{
    std::string rootDir = getEnvVar("ASSIGNMENT4_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: ASSIGNMENT4_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/raycaster/cubemaps/";
}

// Returns the absolute path to the volume data directory
std::string volumeDataDir(void)
{
    std::string rootDir = getEnvVar("ASSIGNMENT4_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: ASSIGNMENT4_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/raycaster/data/";
}

void loadMesh(const std::string &filename, Mesh *mesh)
{
    OBJMesh obj_mesh;
    objMeshLoad(obj_mesh, filename);
    mesh->vertices = obj_mesh.vertices;
    mesh->normals = obj_mesh.normals;
    mesh->indices = obj_mesh.indices;
}

void loadRayCastVolume(Context &ctx, const std::string &filename, RayCastVolume *rayCastVolume)
{
    cg::VolumeBase volume;
    cg::volumeLoadVTK(&volume, (volumeDataDir() + ctx.dataset[ctx.dataset_current]));
    rayCastVolume->volume = volume;

    glDeleteTextures(1, &rayCastVolume->volumeTexture);
    glGenTextures(1, &rayCastVolume->volumeTexture);
    glBindTexture(GL_TEXTURE_3D, rayCastVolume->volumeTexture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, volume.dimensions.x,
                 volume.dimensions.y, volume.dimensions.z,
                 0, GL_RED, GL_UNSIGNED_BYTE, &volume.data[0]);
    glBindTexture(GL_TEXTURE_3D, 0);

    glDeleteTextures(1, &rayCastVolume->backFaceTexture);
    glGenTextures(1, &rayCastVolume->backFaceTexture);
    glBindTexture(GL_TEXTURE_2D, rayCastVolume->backFaceTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, ctx.width, ctx.height,
                 0, GL_RGBA, GL_UNSIGNED_SHORT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    glDeleteTextures(1, &rayCastVolume->frontFaceTexture);
    glGenTextures(1, &rayCastVolume->frontFaceTexture);
    glBindTexture(GL_TEXTURE_2D, rayCastVolume->frontFaceTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, ctx.width, ctx.height,
                 0, GL_RGBA, GL_UNSIGNED_SHORT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    // outline of LUT texture (scrapped idea)
    /*
    glDeleteTextures(1, &rayCastVolume->lut);
    glGenTextures(1, &rayCastVolume->lut);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glEnable(GL_TEXTURE_1D);
    unsigned char colors[6];
    colors[0] = 255; colors[1] = 0; colors[2] = 0; //red
    colors[3] = 0; colors[4] = 0; colors[5] = 255; //blue
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 6, 0, GL_RGB, GL_UNSIGNED_BYTE, colors);  
    */ 

    glDeleteFramebuffers(1, &rayCastVolume->frontFaceFBO);
    glGenFramebuffers(1, &rayCastVolume->frontFaceFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, rayCastVolume->frontFaceFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, rayCastVolume->frontFaceTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: Framebuffer is not complete\n";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDeleteFramebuffers(1, &rayCastVolume->backFaceFBO);
    glGenFramebuffers(1, &rayCastVolume->backFaceFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, rayCastVolume->backFaceFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, rayCastVolume->backFaceTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: Framebuffer is not complete\n";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void createMeshVAO(Context &ctx, const Mesh &mesh, MeshVAO *meshVAO)
{
    // Generates and populates a VBO for the vertices
    glGenBuffers(1, &(meshVAO->vertexVBO));
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->vertexVBO);
    auto verticesNBytes = mesh.vertices.size() * sizeof(mesh.vertices[0]);
    glBufferData(GL_ARRAY_BUFFER, verticesNBytes, mesh.vertices.data(), GL_STATIC_DRAW);

    // Generates and populates a VBO for the vertex normals
    glGenBuffers(1, &(meshVAO->normalVBO));
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->normalVBO);
    auto normalsNBytes = mesh.normals.size() * sizeof(mesh.normals[0]);
    glBufferData(GL_ARRAY_BUFFER, normalsNBytes, mesh.normals.data(), GL_STATIC_DRAW);

    // Generates and populates a VBO for the element indices
    glGenBuffers(1, &(meshVAO->indexVBO));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshVAO->indexVBO);
    auto indicesNBytes = mesh.indices.size() * sizeof(mesh.indices[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesNBytes, mesh.indices.data(), GL_STATIC_DRAW);

    // Creates a vertex array object (VAO) for drawing the mesh
    glGenVertexArrays(1, &(meshVAO->vao));
    glBindVertexArray(meshVAO->vao);
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->vertexVBO);
    glEnableVertexAttribArray(POSITION);
    glVertexAttribPointer(POSITION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->normalVBO);
    glEnableVertexAttribArray(NORMAL);
    glVertexAttribPointer(NORMAL, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshVAO->indexVBO);
    glBindVertexArray(ctx.defaultVAO);

    // Additional information required by draw calls
    meshVAO->numVertices = mesh.vertices.size();
    meshVAO->numIndices = mesh.indices.size();
}

void createQuadVAO(Context &ctx, MeshVAO *meshVAO)
{
    const glm::vec3 vertices[] = {
        glm::vec3(-1.0f, -1.0f,  0.0f),
        glm::vec3(1.0f, -1.0f,  0.0f),
        glm::vec3(1.0f,  1.0f,  0.0f),
        glm::vec3(-1.0f, -1.0f,  0.0f),
        glm::vec3(1.0f,  1.0f,  0.0f),
        glm::vec3(-1.0f,  1.0f,  0.0f),
    };

    // Generates and populates a VBO for the vertices
    glGenBuffers(1, &(meshVAO->vertexVBO));
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->vertexVBO);
    auto verticesNBytes = 6 * sizeof(vertices[0]);
    glBufferData(GL_ARRAY_BUFFER, verticesNBytes, &vertices[0], GL_STATIC_DRAW);

    // Creates a vertex array object (VAO) for drawing the mesh
    glGenVertexArrays(1, &(meshVAO->vao));
    glBindVertexArray(meshVAO->vao);
    glBindBuffer(GL_ARRAY_BUFFER, meshVAO->vertexVBO);
    glEnableVertexAttribArray(POSITION);
    glVertexAttribPointer(POSITION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(ctx.defaultVAO);

    // Additional information required by draw calls
    meshVAO->numVertices = 6;
    meshVAO->numIndices = 0;
}

void initializeTrackball(Context &ctx)
{
    double radius = double(std::min(ctx.width, ctx.height)) / 2.0;
    ctx.trackball.radius = radius;
    glm::vec2 center = glm::vec2(ctx.width, ctx.height) / 2.0f;
    ctx.trackball.center = center;
}

void init(Context &ctx)
{
    // Load shaders
    ctx.boundingGeometryProgram = loadShaderProgram(shaderDir() + "boundingGeometry.vert",
                                                    shaderDir() + "boundingGeometry.frag");
    ctx.rayCasterProgram = loadShaderProgram(shaderDir() + "rayCaster.vert",
                                             shaderDir() + "rayCaster.frag");

    // Load bounding geometry (2-unit cube)
    loadMesh((modelDir() + "cube.obj"), &ctx.cubeMesh);
    createMeshVAO(ctx, ctx.cubeMesh, &ctx.cubeVAO);

    // Create fullscreen quad for ray-casting
    createQuadVAO(ctx, &ctx.quadVAO);

    // Load volume data
    loadRayCastVolume(ctx, (volumeDataDir() + ctx.dataset[ctx.dataset_current]), &ctx.rayCastVolume);
    ctx.rayCastVolume.volume.spacing *= 0.008f;  // FIXME
    initializeTrackball(ctx);
}

// MODIFY THIS FUNCTION
void drawBoundingGeometry(Context &ctx, GLuint program, const MeshVAO &cubeVAO,
                          const RayCastVolume &rayCastVolume)
{
    glm::mat4 model = cg::volumeComputeModelMatrix(rayCastVolume.volume);
    model = trackballGetRotationMatrix(ctx.trackball) * model;
    glm::mat4 view = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, -2.0f));
    glm::mat4 projection = glm::perspective(45.0f * (3.141592f / 180.0f), ctx.aspect, 0.1f, 100.0f);
    glm::mat4 mvp = projection * view * model;

    glUseProgram(program);
    glUniformMatrix4fv(glGetUniformLocation(program, "u_mvp"), 1, GL_FALSE, &mvp[0][0]);

    glBindVertexArray(cubeVAO.vao);
    glDrawElements(GL_TRIANGLES, cubeVAO.numIndices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(ctx.defaultVAO);

    glUseProgram(0);
}

// MODIFY THIS FUNCTION
void drawRayCasting(Context &ctx, GLuint program, const MeshVAO &quadVAO,
                    const RayCastVolume &rayCastVolume)
{
    glUseProgram(program);
    // Set uniforms and bind textures here...
    // Moved uniforms to ctx for use in imgui 
     glUniform1f(glGetUniformLocation(program, "u_step_size"), ctx.step_size);
     glUniform1i(glGetUniformLocation(program, "u_mode"), ctx.mode);
     glUniform3fv(glGetUniformLocation(program, "u_tf1_color"), 1, &ctx.tf1[0]);
     glUniform3fv(glGetUniformLocation(program, "u_tf2_color"), 1, &ctx.tf2[0]);
     glUniform3fv(glGetUniformLocation(program, "u_tf3_color"), 1, &ctx.tf3[0]);
     glUniform3fv(glGetUniformLocation(program, "u_tf4_color"), 1, &ctx.tf4[0]);
     glUniform1f(glGetUniformLocation(program, "u_tf1_intensity"), ctx.tf1_intensity);
     glUniform1f(glGetUniformLocation(program, "u_tf2_intensity"), ctx.tf2_intensity);
     glUniform1f(glGetUniformLocation(program, "u_tf3_intensity"), ctx.tf3_intensity);
     glUniform1f(glGetUniformLocation(program, "u_tf4_intensity"), ctx.tf4_intensity);
     glUniform1f(glGetUniformLocation(program, "u_tf1_alpha"), ctx.tf1_alpha);
     glUniform1f(glGetUniformLocation(program, "u_tf2_alpha"), ctx.tf2_alpha);
     glUniform1f(glGetUniformLocation(program, "u_sample_rate"), ctx.sample_rate);
     glUniform1i(glGetUniformLocation(program, "u_cor_enable"), ctx.correction);
     glUniform1f(glGetUniformLocation(program, "u_cor"), ctx.correction_threshold);

     glActiveTexture(GL_TEXTURE0);
     glBindTexture(GL_TEXTURE_3D, rayCastVolume.volumeTexture);
     glUniform1i(glGetUniformLocation(program, "u_volumeTexture"), 0);

     glActiveTexture(GL_TEXTURE1);
     glBindTexture(GL_TEXTURE_2D, rayCastVolume.frontFaceTexture);
     glUniform1i(glGetUniformLocation(program, "u_frontFaceTexture"), 1);

     glActiveTexture(GL_TEXTURE2);
     glBindTexture(GL_TEXTURE_2D, rayCastVolume.backFaceTexture);
     glUniform1i(glGetUniformLocation(program, "u_backFaceTexture"), 2);

    glBindVertexArray(quadVAO.vao);
    glDrawArrays(GL_TRIANGLES, 0, quadVAO.numVertices);
    glBindVertexArray(ctx.defaultVAO);

    glUseProgram(0);
}

void display(Context &ctx)
{
    glClearColor(ctx.background.x, ctx.background.y, ctx.background.z, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render the front faces of the volume bounding box to a texture
    // via the frontFaceFBO
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
     glBindFramebuffer(GL_FRAMEBUFFER, ctx.rayCastVolume.frontFaceFBO);
     glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
     glClear(GL_COLOR_BUFFER_BIT);
    drawBoundingGeometry(ctx, ctx.boundingGeometryProgram, ctx.cubeVAO, ctx.rayCastVolume);
     glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Render the back faces of the volume bounding box to a texture
    // via the backFaceFBO
    // ...
     glCullFace(GL_FRONT);
     glBindFramebuffer(GL_FRAMEBUFFER, ctx.rayCastVolume.backFaceFBO);
     glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
     glClear(GL_COLOR_BUFFER_BIT);
    drawBoundingGeometry(ctx, ctx.boundingGeometryProgram, ctx.cubeVAO, ctx.rayCastVolume);
     glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // ...

    // Perform ray-casting
    // ...
     glCullFace(GL_BACK);
     glEnable(GL_DEPTH_TEST);
     drawRayCasting(ctx, ctx.rayCasterProgram, ctx.quadVAO, ctx.rayCastVolume);
}

void reloadShaders(Context *ctx)
{
    glDeleteProgram(ctx->boundingGeometryProgram);
    ctx->boundingGeometryProgram = loadShaderProgram(shaderDir() + "boundingGeometry.vert",
                                                     shaderDir() + "boundingGeometry.frag");
    glDeleteProgram(ctx->rayCasterProgram);
    ctx->rayCasterProgram = loadShaderProgram(shaderDir() + "rayCaster.vert",
                                              shaderDir() + "rayCaster.frag");
}

void mouseButtonPressed(Context *ctx, int button, int x, int y)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        ctx->trackball.center = glm::vec2(x, y);
        trackballStartTracking(ctx->trackball, glm::vec2(x, y));
    }
}

void mouseButtonReleased(Context *ctx, int button, int x, int y)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        trackballStopTracking(ctx->trackball);
    }
}

void moveTrackball(Context *ctx, int x, int y)
{
    if (ctx->trackball.tracking) {
        trackballMove(ctx->trackball, glm::vec2(x, y));
    }
}

void errorCallback(int /*error*/, const char* description)
{
    std::cerr << description << std::endl;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Forward event to GUI
    ImGui_ImplGlfwGL3_KeyCallback(window, key, scancode, action, mods);
    if (ImGui::GetIO().WantCaptureKeyboard) { return; }  // Skip other handling

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        reloadShaders(ctx);
    }
}

void charCallback(GLFWwindow* window, unsigned int codepoint)
{
    // Forward event to GUI
    ImGui_ImplGlfwGL3_CharCallback(window, codepoint);
    if (ImGui::GetIO().WantTextInput) { return; }  // Skip other handling
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    // Forward event to GUI
    ImGui_ImplGlfwGL3_MouseButtonCallback(window, button, action, mods);
    if (ImGui::GetIO().WantCaptureMouse) { return; }  // Skip other handling

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    if (action == GLFW_PRESS) {
        mouseButtonPressed(ctx, button, x, y);
    }
    else {
        mouseButtonReleased(ctx, button, x, y);
    }
}

void cursorPosCallback(GLFWwindow* window, double x, double y)
{
    if (ImGui::GetIO().WantCaptureMouse) { return; }  // Skip other handling   

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    moveTrackball(ctx, x, y);
}

void resizeCallback(GLFWwindow* window, int width, int height)
{
    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    ctx->width = width;
    ctx->height = height;
    ctx->aspect = float(width) / float(height);
    ctx->trackball.radius = double(std::min(width, height)) / 2.0;
    ctx->trackball.center = glm::vec2(width, height) / 2.0f;
    glViewport(0, 0, width, height);

    // Resize FBO textures to match window size
    glBindTexture(GL_TEXTURE_2D, ctx->rayCastVolume.frontFaceTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, ctx->width, ctx->height,
                 0, GL_RGBA, GL_UNSIGNED_SHORT, nullptr);
    glBindTexture(GL_TEXTURE_2D, ctx->rayCastVolume.backFaceTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, ctx->width, ctx->height,
                 0, GL_RGBA, GL_UNSIGNED_SHORT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}
/*
  Set context parameters to default values.  
*/
void loadDefault(Context &ctx) {

    if(ctx.dataset_current == 0) {
        ctx.rayCastVolume.volume.spacing *= 0.008f;
        ctx.tf4 = glm::vec3(0.7f, 0.5f, 0.5f);
        ctx.tf3 = glm::vec3(1.0f, 0.5f, 0.5f);
        ctx.tf2 = glm::vec3(1.0f, 1.0f, 0.8f);
        ctx.tf1 = glm::vec3(1.0f, 1.0f, 1.0f); 
        ctx.tf4_intensity = 0.000f;
        ctx.tf3_intensity = 0.100f;
        ctx.tf2_intensity = 0.400f;
        ctx.tf1_intensity = 0.500f;
        ctx.tf1_alpha = 0.747f;
        ctx.tf2_alpha = 0.853f;
        ctx.sample_rate = 50.0f;
        ctx.correction_enable = true;
        ctx.correction = 1;
        ctx.correction_threshold = 0.01;
    }
    else if(ctx.dataset_current == 1) {
        // TODO: Set decent default values
        ctx.rayCastVolume.volume.spacing *= 0.003f;
        ctx.tf4 = glm::vec3(0.7f, 0.5f, 0.5f);
        ctx.tf3 = glm::vec3(1.0f, 0.5f, 0.5f);
        ctx.tf2 = glm::vec3(1.0f, 1.0f, 0.8f);
        ctx.tf1 = glm::vec3(1.0f, 1.0f, 1.0f); 
        ctx.tf4_intensity = 0.000f;
        ctx.tf3_intensity = 0.100f;
        ctx.tf2_intensity = 0.400f;
        ctx.tf1_intensity = 0.500f;
        ctx.tf1_alpha = 0.747f;
        ctx.tf2_alpha = 0.853f;
        ctx.sample_rate = 50.0f;
        ctx.correction_enable = true;
        ctx.correction = 1;
        ctx.correction_threshold = 0.01;
    }
    else if(ctx.dataset_current == 2) {
        // TODO: Set decent default values
        ctx.rayCastVolume.volume.spacing *= 0.005f;
        ctx.tf4 = glm::vec3(0.7f, 0.5f, 0.5f);
        ctx.tf3 = glm::vec3(1.0f, 0.5f, 0.5f);
        ctx.tf2 = glm::vec3(1.0f, 1.0f, 0.8f);
        ctx.tf1 = glm::vec3(1.0f, 1.0f, 1.0f); 
        ctx.tf4_intensity = 0.000f;
        ctx.tf3_intensity = 0.100f;
        ctx.tf2_intensity = 0.400f;
        ctx.tf1_intensity = 0.500f;
        ctx.tf1_alpha = 0.747f;
        ctx.tf2_alpha = 0.853f;
        ctx.sample_rate = 50.0f;
        ctx.correction_enable = true;
        ctx.correction = 1;
        ctx.correction_threshold = 0.01;
        }
    else if(ctx.dataset_current == 3) { 
        // TODO: Set decent default values
        ctx.rayCastVolume.volume.spacing *= 0.008f;
        ctx.tf4 = glm::vec3(0.7f, 0.5f, 0.5f);
        ctx.tf3 = glm::vec3(1.0f, 0.5f, 0.5f);
        ctx.tf2 = glm::vec3(1.0f, 1.0f, 0.8f);
        ctx.tf1 = glm::vec3(1.0f, 1.0f, 1.0f); 
        ctx.tf4_intensity = 0.000f;
        ctx.tf3_intensity = 0.100f;
        ctx.tf2_intensity = 0.400f;
        ctx.tf1_intensity = 0.500f;
        ctx.tf1_alpha = 0.747f;
        ctx.tf2_alpha = 0.853f;
        ctx.sample_rate = 50.0f;
        ctx.correction_enable = true;
        ctx.correction = 1;
        ctx.correction_threshold = 0.01;
        }
    else {
        // Should not occur
        ctx.rayCastVolume.volume.spacing *= 0.008f;  // FIXME
    }

}

/* ImGui GUI for tweaking the rendering parameters. 
   Crude attempt at front-end development to look similar to
   https://studentportalen.uu.se/uusp-webapp/rest/spring/webpagefiles/files/inline/350369/38bd2716-9532-44b6-a9bd-367edba004d2.png
*/
void runGUI(Context &ctx) {
	End();
	/*
    ImGui::Begin("TweakBar");
    ImGui::Spacing();
    ImGui::ListBox("Dataset", &ctx.dataset_current, &ctx.dataset[0], 4, -1);
    if(ctx.dataset_current != ctx.dataset_changed) {
        // Reload volume and set some parameters to arbitrary defaults
        loadRayCastVolume(ctx, (volumeDataDir() + ctx.dataset[ctx.dataset_current]), &ctx.rayCastVolume);
        loadDefault(ctx);
        ctx.dataset_changed = ctx.dataset_current;
    }
    ImGui::Spacing();
    //ImGui::InputFloat("Volume spacing", &ctx.rayCastVolume.volume.spacing, 0.0f, 0.0f, -1, 0);
    ImGui::SliderFloat("Step size", &ctx.step_size, 0.005f, 1.0f, "%.3f", 1.0f);
    ImGui::Spacing();
    if (ImGui::Button("Mode")) {
        if(ctx.mode == 1) {
            ctx.mode = 0;
        }
        else {
            ctx.mode = 1;
        }
    }
    if (ctx.mode == 0) {
        ImGui::SameLine();
        ImGui::Text("Alpha Blending");
        ImGui::Separator();
        ImGui::PushItemWidth(-140);
        ImGui::ColorEdit3("TF color 1", &ctx.tf1[0]);
        ImGui::SliderFloat("TF color 1 point", &ctx.tf1_intensity, 0.0f, 1.0f, "%.3f", 1.0f);
        ImGui::ColorEdit3("TF color 2", &ctx.tf2[0]);
        ImGui::SliderFloat("TF color 2 point", &ctx.tf2_intensity, 0.0f, 1.0f, "%.3f", 1.0f);
        ImGui::ColorEdit3("TF color 3", &ctx.tf3[0]);
        ImGui::SliderFloat("TF color 3 point", &ctx.tf3_intensity, 0.0f, 1.0f, "%.3f", 1.0f);
        ImGui::ColorEdit3("TF color 4", &ctx.tf4[0]);
        ImGui::SliderFloat("TF color 4 point", &ctx.tf4_intensity, 0.0f, 1.0f, "%.3f", 1.0f);
        ImGui::PushItemWidth(-140);
        ImGui::SliderFloat("TF alpha 1", &ctx.tf1_alpha, 0.0f, 1.0f, "%.3f", 1.0f);
        ImGui::SliderFloat("TF alpha 2", &ctx.tf2_alpha, 0.0f, 1.0f, "%.3f", 1.0f);
        ImGui::Spacing();
        ImGui::SliderFloat("Sample rate", &ctx.sample_rate, 1.0f, 2000.0f, "%.0f", 1.0f);
        ImGui::Separator();
    }
    else { // ctx.mode == 1
        ImGui::SameLine();
        ImGui::Text("Maximum Intensity Projection");
    }
    ImGui::Spacing();
    ImGui::Checkbox("Cube removal", &ctx.correction_enable);
    if (ctx.correction_enable) {
        ctx.correction = 1;
        ImGui::SliderFloat("Discard threshold", &ctx.correction_threshold, 0.0f, 0.1f, "%.3f", 1.0f);
    }
    else {
        ctx.correction = 0;
    }
    ImGui::Spacing();
    ImGui::ColorEdit3("Background", &ctx.background[0]);*/
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
    ctx.aspect = float(ctx.width) / float(ctx.height);
    ctx.window = glfwCreateWindow(ctx.width, ctx.height, "Volume rendering", nullptr, nullptr);
    glfwMakeContextCurrent(ctx.window);
    glfwSetWindowUserPointer(ctx.window, &ctx);
    glfwSetKeyCallback(ctx.window, keyCallback);
    glfwSetCharCallback(ctx.window, charCallback);
    glfwSetMouseButtonCallback(ctx.window, mouseButtonCallback);
    glfwSetCursorPosCallback(ctx.window, cursorPosCallback);
    glfwSetFramebufferSizeCallback(ctx.window, resizeCallback);

    // Load OpenGL functions
    glewExperimental = true;
    GLenum status = glewInit();
    if (status != GLEW_OK) {
        std::cerr << "Error: " << glewGetErrorString(status) << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    // Initialize GUI
    ImGui_ImplGlfwGL3_Init(ctx.window, false /*do not install callbacks*/);

    // Initialize rendering
    glGenVertexArrays(1, &ctx.defaultVAO);
    glBindVertexArray(ctx.defaultVAO);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    init(ctx);

	loadDefault(ctx);


    // Start rendering loop
    while (!glfwWindowShouldClose(ctx.window)) {
        glfwPollEvents();
        ctx.elapsed_time = glfwGetTime();
        ImGui_ImplGlfwGL3_NewFrame();
        runGUI(ctx); // Call used for running GUI (shocker)
        display(ctx);
        ImGui::Render();
        glfwSwapBuffers(ctx.window);
    }

    // Shutdown
    glfwDestroyWindow(ctx.window);
    glfwTerminate();
    std::exit(EXIT_SUCCESS);
}
