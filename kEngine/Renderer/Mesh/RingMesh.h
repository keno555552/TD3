#pragma once
#include "Data/Render/CPUData/VertexData.h"
#include "LinearAlgebra/Transform.h"
#include "LinearAlgebra/TransformationMatrix.h"
#include "VertexResource.h"
#include "Model.h"
#include "Config.h"

class RingMesh :public Model
{
public:
	ID3D12Resource* CreateVertexResource_(ID3D12Device* device)override;
	ID3D12Resource* CreateIndexResource_(ID3D12Device* device)override;
	int GetVertexNum() { return MeshBuffer::vertexNum_; }
	void SetRingDivide(int divide) { kRingDivide_ = divide; }
	void SetOuterRadius(float radius) { kOuterRadius_ = radius; }
	void SetInnerRadius(float radius) { kInnerRadius_ = radius; }

private:
	int kRingDivide_ = 32;
	float kOuterRadius_ = 1.0f;
	float kInnerRadius_ = 0.2f;
};

