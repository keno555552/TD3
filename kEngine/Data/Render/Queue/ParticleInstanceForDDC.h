#pragma once
#include "TransformationMatrix.h"
#include "Mesh/VertexResource.h"
#include "Renderer/PSOManager/PSOKeys.h"
using MaterialID = int;

struct ParticleInstanceForDDC
{
	MeshBuffer* mesh;
	PSOKey psoKey;
	MaterialID materialID;
	TransformationMatrix transMatrix{};
};