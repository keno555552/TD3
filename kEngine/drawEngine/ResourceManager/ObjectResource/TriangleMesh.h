#pragma once
#include "VertexData.h"
#include "Transform.h"
#include "TransformationMatrix.h"
#include "VertexResource.h"
#include "Config.h"
#include "Model.h"
#include <d3d12.h>

class TriangleMesh : public Model
{
public:
	ID3D12Resource* CreateVertexResource_(ID3D12Device* device)override;
};

