#include "cgVolume.h"

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>

namespace {

// Struct for VTK header info
struct VTKHeader {
    bool binary;
    glm::ivec3 dimensions;
    glm::vec3 origin;
    glm::vec3 spacing;
    std::string datatype;

    VTKHeader() :
        binary(true),
        dimensions(glm::ivec3(0, 0, 0)),
        origin(glm::vec3(0.0f, 0.0f, 0.0f)),
        spacing(glm::vec3(0.0f, 0.0f, 0.0f)),
        datatype("")
    {}
};

// Check if file is a valid VTK file
bool isVTKFile(const std::string &filename)
{
    std::ifstream f(filename, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "Could not open " << filename << std::endl;
        return false;
    }

    std::string checkvtk;
    f >> checkvtk; // #
    f >> checkvtk; // vtk
    f.close();
    if (!(checkvtk == "vtk" || checkvtk == "VTK")) {
        return false;
    }

    return true;
}

// Extract data format (ASCII or BINARY) from header strings
bool extractFormat(const std::vector<std::string> &headerLines, VTKHeader *header)
{
    for (auto it = headerLines.begin(); it != headerLines.end(); ++it) {
        if (it->substr(0, 6) == "BINARY") {
            header->binary = true;
            return true;
        }
        else if (it->substr(0, 5) == "ASCII") {
            header->binary = false;
            return true;
        }
    }
    return false;
}

// Extract volume dimensions (width, height, depth) from header
// strings
bool extractDimensions(const std::vector<std::string> &headerLines, VTKHeader *header)
{
    for (auto it = headerLines.begin(); it != headerLines.end(); ++it) {
        if (it->substr(0, 10) == "DIMENSIONS") {
            int width, height, depth;
            sscanf(it->c_str(), "%*s %d %d %d", &width, &height, &depth);
            header->dimensions = glm::ivec3(width, height, depth);
            return true;
        }
    }
    return false;
}

// Extract volume origin (ox, oy, oz) from header strings
bool extractOrigin(const std::vector<std::string> &headerLines, VTKHeader *header)
{
    for (auto it = headerLines.begin(); it != headerLines.end(); ++it) {
        if (it->substr(0, 6) == "ORIGIN") {
            float ox, oy, oz;
            sscanf(it->c_str(), "%*s %f %f %f", &ox, &oy, &oz);
            header->origin = glm::vec3(ox, oy, oz);
            return true;
        }
    }
    return false;
}

// Extract voxel spacing (sx, sy, sz) from header strings
bool extractSpacing(const std::vector<std::string> &headerLines, VTKHeader *header)
{
    for (auto it = headerLines.begin(); it != headerLines.end(); ++it) {
        if (it->substr(0, 7) == "SPACING") {
            float sx, sy, sz;
            sscanf(it->c_str(), "%*s %f %f %f", &sx, &sy, &sz);
            header->spacing = glm::vec3(sx, sy, sz);
            return true;
        }
    }
    return false;
}

// Extract voxel data type from header strings
bool extractDataType(const std::vector<std::string> &headerLines, VTKHeader *header)
{
    for (auto it = headerLines.begin(); it != headerLines.end(); ++it) {
        if (it->substr(0, 7) == "SCALARS") {
            char buffer[20];
            sscanf(it->c_str(), "%*s %*s %s", buffer);
            std::string typestring(buffer);
            header->datatype = typestring;

            if (typestring == "unsigned_char") {
                header->datatype = "uint8";
            }
            else if (typestring == "unsigned_short") {
                header->datatype = "uint16";
            }
            else if (typestring == "short") {
                header->datatype = "int16";
            }
            else if (typestring == "unsigned_int") {
                header->datatype = "uint32";
            }
            else if (typestring == "float") {
                header->datatype = "float32";
            }
            else { // unsupported or invalid data type
                return false;
            }
            return true;
        }
    }
    return false;
}

// Check endianness of target architecture
bool isLittleEndian()
{
    union {
        unsigned long l;
        unsigned char c[sizeof(unsigned long)];
    } data;
    data.l = 1;
    return data.c[0] == 1;
}

// Swap byte order of 2-byte element
void swap2Bytes(unsigned char* &ptr)
{
    unsigned char tmp;
    tmp = ptr[0]; ptr[0] = ptr[1]; ptr[1] = tmp;
}

// Swap byte order of 4-byte element
void swap4Bytes(unsigned char* &ptr)
{
    unsigned char tmp;
    tmp = ptr[0]; ptr[0] = ptr[3]; ptr[3] = tmp;
    tmp = ptr[1]; ptr[1] = ptr[2]; ptr[2] = tmp;
}

// Swap byte order of image data elements
template<typename T>
void swapByteOrder(std::vector<T> *imageData)
{
    int numElements = imageData->size();
    int elementSizeInBytes = sizeof(T);
    unsigned char *elementPointer = reinterpret_cast<unsigned char *>(&(*imageData)[0]);
    switch (elementSizeInBytes) {
    case 2: // uint16, int16
        for (int i = 0; i < numElements; i++, elementPointer += elementSizeInBytes) {
            swap2Bytes(elementPointer);
        }
        break;
    case 4: // uint32, float32
        for (int i = 0; i < numElements; i++, elementPointer += elementSizeInBytes) {
            swap4Bytes(elementPointer);
        }
        break;
    default:
        break;
    }
}

// Read image data in binary format
template<typename T>
void readVTKBinary(std::ifstream &is, std::vector<T> *imageData, int n)
{
    if (isLittleEndian()) {
        is.read(reinterpret_cast<char *>(&(*imageData)[0]), sizeof(T) * n);
        swapByteOrder(imageData);
    }
    else {
        is.read(reinterpret_cast<char *>(&(*imageData)[0]), sizeof(T) * n);
    }
}

// Read image data in ASCII format
template<typename T>
void readVTKASCII(std::ifstream &is, std::vector<T> *imageData, int n)
{
    T value;
    for(int i = 0; i < n; i++) {
        is >> value;
        imageData->push_back(value);
    }
}

// Read the header part (the first ten lines) of the file
bool readHeader(const std::string filename, VTKHeader *header)
{
    std::ifstream VTKFile(filename, std::ios::binary);
    if (!VTKFile.is_open()) {
        std::cerr << "Could not open " << filename << std::endl;
        return false;
    }

    // Read header
    int numHeaderLines = 10;
    std::vector<std::string> headerLines;
    for (int i = 0; i < numHeaderLines; i++) {
        std::string line;
        std::getline(VTKFile, line);
        if (line.empty()) {
            VTKFile.close();
            return false;
        }
        else {
            headerLines.push_back(line);
        }
    }

    // Extract header information
    if (!extractFormat(headerLines, header)) {
        return false;
    }
    if (!extractDimensions(headerLines, header)) {
        return false;
    }
    if (!extractOrigin(headerLines, header)) {
        return false;
    }
    if (!extractSpacing(headerLines, header)) {
        return false;
    }
    if (!extractDataType(headerLines, header)) {
        return false;
    }

    return true;
}

// Read the data part of the file
template<typename VoxelType>
bool readData(const std::string filename, const VTKHeader &header, std::vector<VoxelType> *imageData)
{
    std::ifstream VTKFile(filename, std::ios::binary);
    if (!VTKFile.is_open()) {
        std::cerr << "Could not open " << filename << std::endl;
        return false;
    }

    // Jump to data
    int numHeaderLines = 10;
    for (int i = 0; i < numHeaderLines; i++) {
        std::string line;
        std::getline(VTKFile, line);
        if (line.empty()) {
            VTKFile.close();
            return false;
        }
    }

    // Read data
    glm::ivec3 dimensions = header.dimensions;
    int numElements = dimensions[0] * dimensions[1] * dimensions[2];
    imageData->clear();
    imageData->reserve(numElements);
    if (header.binary) {
        imageData->resize(numElements);
        readVTKBinary(VTKFile, imageData, numElements);
    }
    else {
        readVTKASCII(VTKFile, imageData, numElements);
    }

    return true;
}

} // namespace



namespace cg {

// Read a volume image in the legacy VTK StructuredPoints format
// from a file. Assumes that the file starts with a ten line header
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
bool volumeLoadVTK(VolumeBase *volume, const std::string &filename)
{
    if (!isVTKFile(filename)) {
        return false;
    }

    // Read header
    VTKHeader header;
    if (!readHeader(filename, &header)) {
        return false;
    }

    // Read data
    if (header.datatype == "uint8") {
        std::vector<uint8_t> imageData;
        if (!readData(filename, header, &imageData)) {
            return false;
        }
        size_t nBytes = imageData.size() * sizeof(imageData[0]);
        volume->data.resize(nBytes);
        std::memcpy(&volume->data[0], &imageData[0], nBytes);
    }
    else if (header.datatype == "uint16") {
        std::vector<uint16_t> imageData;
        if (!readData(filename, header, &imageData)) {
            return false;
        }
        size_t nBytes = imageData.size() * sizeof(imageData[0]);
        volume->data.resize(nBytes);
        std::memcpy(&volume->data[0], &imageData[0], nBytes);
    }
    else if (header.datatype == "int16") {
        std::vector<int16_t> imageData;
        if (!readData(filename, header, &imageData)) {
            return false;
        }
        size_t nBytes = imageData.size() * sizeof(imageData[0]);
        volume->data.resize(nBytes);
        std::memcpy(&volume->data[0], &imageData[0], nBytes);
    }
    else if (header.datatype == "uint32") {
        std::vector<uint32_t> imageData;
        if (!readData(filename, header, &imageData)) {
            return false;
        }
        size_t nBytes = imageData.size() * sizeof(imageData[0]);
        volume->data.resize(nBytes);
        std::memcpy(&volume->data[0], &imageData[0], nBytes);
    }
    else if (header.datatype == "float32") {
        std::vector<float> imageData;
        if (!readData(filename, header, &imageData)) {
            return false;
        }
        size_t nBytes = imageData.size() * sizeof(imageData[0]);
        volume->data.resize(nBytes);
        std::memcpy(&volume->data[0], &imageData[0], nBytes);
    }
    else {
        return false;
    }

    volume->dimensions = header.dimensions;
    volume->origin = header.origin;
    volume->spacing = header.spacing;
    volume->datatype = header.datatype;

    return true;
}

} // namespace cg
