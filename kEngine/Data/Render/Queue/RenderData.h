#pragma once
#include "TransformationMatrix.h"
#include "Mesh/VertexResource.h"
#include "Data/Render/GPUData/MaterialForGPU.h"
#include "Renderer/PSOManager/PSOKeys.h"
using MaterialID = int;

struct RenderData
{
    MeshBuffer* mesh;
    MaterialID materialID;
    TransformationMatrix transformData;
    PSOKey psoKey;
    uint32_t subMeshIndex;

	bool CheckSame(const RenderData target);
};

