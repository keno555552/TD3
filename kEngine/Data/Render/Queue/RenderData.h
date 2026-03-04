#pragma once
#include "TransformationMatrix.h"
#include "VertexResource.h"
#include "material.h"
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

