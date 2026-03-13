#pragma once
#include "TransformationMatrix.h"
#include "Mesh/VertexResource.h"
#include "Data/Render/GPUData/MaterialForGPU.h"
using MaterialID = int;

struct RenderData
{
    MeshBuffer* mesh;
    MaterialID materialID;
    TransformationMatrix transformData;
    uint32_t psoID;
    uint32_t subMeshIndex;

	bool CheckSame(const RenderData target);
};

