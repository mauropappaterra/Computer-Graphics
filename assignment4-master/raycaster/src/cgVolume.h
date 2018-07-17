#pragma once

#include <vector>
#include <string>
#include <cstdint>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace cg {

// Base struct for volume images
struct VolumeBase {
    glm::ivec3 dimensions;  // volume dimensions
    glm::vec3 origin;  // volume origin
    glm::vec3 spacing;  // voxel spacing
    std::string datatype;  // voxel data type string
    std::vector<std::uint8_t> data;  // voxel data
};

// Template struct for typed volume images
template <typename VoxelType>
struct Volume {
    VolumeBase base;
    typedef VoxelType voxel_type;

    // Overridden operator for element access (no bounds checking!)
    VoxelType &operator()(int x, int y, int z);
};

// Typed volume images
typedef Volume<std::uint8_t> VolumeUInt8;
typedef Volume<std::uint16_t> VolumeUInt16;
typedef Volume<std::int16_t> VolumeInt16;
typedef Volume<std::uint32_t> VolumeUInt32;
typedef Volume<float> VolumeFloat32;



// Overridden operator for element access (no bounds checking!)
template<typename VoxelType>
inline VoxelType &Volume<VoxelType>::operator()(int x, int y, int z)
{
    int index = (base.dimensions.x * base.dimensions.y * z) +
                (base.dimensions.x * y) + x;
    return reinterpret_cast<VoxelType *>(&base.data[0])[index];
}

// Computes the extent (dimensions*spacing) of the volume image
inline glm::vec3 volumeComputeExtent(const VolumeBase &volume)
{
    return glm::vec3(volume.dimensions) * volume.spacing;
}

// Computes the model matrix for the volume image. This matrix can be
// used during rendering to scale a 2-unit cube to the size of
// the volume image. Assumes that the cube is centered at origin.
inline glm::mat4 volumeComputeModelMatrix(const VolumeBase &volume)
{
    glm::vec3 extent = volumeComputeExtent(volume);
    return glm::translate(glm::mat4(), volume.origin) *
           glm::scale(glm::mat4(), 0.5f * extent);
}

// Reads a volume image in the legacy VTK StructuredPoints format
// from a file. Returns true on success, false otherwise. Possible
// datatypes are: "uint8", "uint16", "int16", "uint32", and "float32".
// Assumes that the file starts with a ten line header
// section followed by a data section in ASCII or binary format,
// i.e.:
//
// # vtk DataFile Version x.x\n
// Some information about the file\n
// BINARY\n
// DATASET STRUCTURED_POINTS\n
// DIMENSIONS 128 128 128\n
// ORIGIN 0.0 0.0 0.0\n
// SPACING 1.0 1.0 1.0\n
// POINT_DATA 2097152\n
// SCALARS image_data unsigned_char\n
// LOOKUP_TABLE default\n
// raw data........\n
bool volumeLoadVTK(VolumeBase *volume, const std::string &filename);

} // namespace cg
