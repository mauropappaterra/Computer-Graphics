#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/constants.hpp>
#include <glm/gtx/quaternion.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

// Struct for representing a virtual 3D trackball that can be used for
// object or camera rotation
struct Trackball {
    double radius;
    glm::vec2 center;
    bool tracking;
    glm::vec3 vStart;
    glm::quat qStart;
    glm::quat qCurrent;

    Trackball() : radius(1.0),
                  center(glm::vec2(0.0f, 0.0f)),
                  tracking(false),
                  vStart(glm::vec3(0.0f, 0.0f, 1.0f)),
                  qStart(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
                  qCurrent(glm::quat(1.0f, 0.0f, 0.0f, 0.0f))
    {}
};

// Struct for Wavefront (OBJ) triangle meshes that are indexed and has
// per-vertex normals
struct OBJMesh {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<std::uint32_t> indices;
};

// Helper functions
namespace {
glm::vec3 mapMousePointToUnitSphere(glm::vec2 point, double radius, glm::vec2 center)
{
    float x = point[0] - center[0];
    float y = -point[1] + center[1];
    float z = 0.0f;
    if (x * x + y * y < radius * radius / 2.0f) {
        z = std::sqrt(radius * radius - (x * x + y * y));
    }
    else {
        z = (radius * radius / 2.0f) / std::sqrt(x * x + y * y);
    }
    return glm::normalize(glm::vec3(x, y, z));
}

void computeNormals(const std::vector<glm::vec3> &vertices,
                    const std::vector<std::uint32_t> &indices,
                    std::vector<glm::vec3> *normals)
{
    normals->resize(vertices.size(), glm::vec3(0.0f, 0.0f, 0.0f));

    // Compute per-vertex normals by averaging the unnormalized face normals
    std::uint32_t vertexIndex0, vertexIndex1, vertexIndex2;
    glm::vec3 normal;
    int numIndices = indices.size();
    for (int i = 0; i < numIndices; i += 3) {
        vertexIndex0 = indices[i];
        vertexIndex1 = indices[i + 1];
        vertexIndex2 = indices[i + 2];
        normal = glm::cross(vertices[vertexIndex1] - vertices[vertexIndex0],
                            vertices[vertexIndex2] - vertices[vertexIndex0]);
        (*normals)[vertexIndex0] += normal;
        (*normals)[vertexIndex1] += normal;
        (*normals)[vertexIndex2] += normal;
    }

    int numNormals = normals->size();
    for (int i = 0; i < numNormals; i++) {
        (*normals)[i] = glm::normalize((*normals)[i]);
    }
}
} // namespace

// Start trackball tracking
void trackballStartTracking(Trackball &trackball, glm::vec2 point)
{
    trackball.vStart = mapMousePointToUnitSphere(point, trackball.radius, trackball.center);
    trackball.qStart = glm::quat(trackball.qCurrent);
    trackball.tracking = true;
}

// Stop trackball tracking
void trackballStopTracking(Trackball &trackball)
{
    trackball.tracking = false;
}

// Rotate trackball from, e.g., mouse movement
void trackballMove(Trackball &trackball, glm::vec2 point)
{
    glm::vec3 vCurrent = mapMousePointToUnitSphere(point, trackball.radius, trackball.center);
    glm::vec3 rotationAxis = glm::cross(trackball.vStart, vCurrent);
    float dotProduct = std::max(std::min(glm::dot(trackball.vStart, vCurrent), 1.0f), -1.0f);
    float rotationAngle = std::acos(dotProduct);
    float eps = 0.01f;
    if (rotationAngle < eps) {
        trackball.qCurrent = glm::quat(trackball.qStart);
    }
    else {
        // Note: here we provide rotationAngle in radians. Older versions
        // of GLM (0.9.3 or earlier) require the angle in degrees.
        glm::quat q = glm::angleAxis(rotationAngle, rotationAxis);
        q = glm::normalize(q);
        trackball.qCurrent = glm::normalize(glm::cross(q, trackball.qStart));
    }
}

// Get trackball orientation in matrix form
glm::mat4 trackballGetRotationMatrix(Trackball &trackball)
{
    return glm::mat4_cast(trackball.qCurrent);
}

// Read an OBJMesh from an .obj file
bool objMeshLoad(OBJMesh &mesh, const std::string &filename)
{
    const std::string VERTEX_LINE("v ");
    const std::string FACE_LINE("f ");

    // Open OBJ file
    std::ifstream f(filename.c_str());
    if (!f.is_open()) {
        std::cerr << "Could not open " << filename << std::endl;
        return false;
    }

    // Extract vertices and indices
    std::string line;
    glm::vec3 vertex;
    std::uint32_t vertexIndex0, vertexIndex1, vertexIndex2;
    while (!f.eof()) {
        std::getline(f, line);
        if (line.substr(0, 2) == VERTEX_LINE) {
            std::istringstream vertexLine(line.substr(2));
            vertexLine >> vertex.x;
            vertexLine >> vertex.y;
            vertexLine >> vertex.z;
            mesh.vertices.push_back(vertex);
        }
        else if (line.substr(0, 2) == FACE_LINE) {
            std::istringstream faceLine(line.substr(2));
            faceLine >> vertexIndex0;
            faceLine >> vertexIndex1;
            faceLine >> vertexIndex2;
            mesh.indices.push_back(vertexIndex0 - 1);
            mesh.indices.push_back(vertexIndex1 - 1);
            mesh.indices.push_back(vertexIndex2 - 1);
        }
        else {
            // Ignore line
        }
    }

    // Close OBJ file
    f.close();

    // Compute normals
    computeNormals(mesh.vertices, mesh.indices, &mesh.normals);

    // Display log message
    std::cout << "Loaded OBJ file " << filename << std::endl;
    int numTriangles = mesh.indices.size() / 3;
    std::cout << "Number of triangles: " << numTriangles << std::endl;

    return true;
}
