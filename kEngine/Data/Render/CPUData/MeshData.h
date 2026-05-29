#pragma once
#include <string>
#include <vector>
#include "VertexData.h"


struct MeshData {

    std::string name;
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;
    uint32_t nodeIndex = (uint32_t)-1;
    uint32_t materialIndex;
    uint32_t skinIndex = (uint32_t)-1; // -1 = 沒有蒙皮
};