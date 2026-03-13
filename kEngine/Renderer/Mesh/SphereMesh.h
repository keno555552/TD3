#pragma once
#include "Data/Render/CPUData/VertexData.h"
#include "Transform.h"
#include "TransformationMatrix.h"
#include "VertexResource.h"
#include "Model.h"
#include "Config.h"

class SphereMesh :public Model
{
public:
	ID3D12Resource* CreateVertexResource_(ID3D12Device* device)override;
	ID3D12Resource* CreateIndexResource_(ID3D12Device* device)override;
	int GetVertexNum() { return VertexNum_; }

private:
	int kMaxSudivision_ = 18;
	int kSudivision_ = 0;
	int VertexNum_ = 0;
};

