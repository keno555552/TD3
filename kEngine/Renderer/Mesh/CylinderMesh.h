#pragma once
#include "Data/Render/CPUData/VertexData.h"
#include "LinearAlgebra/Transform.h"
#include "LinearAlgebra/TransformationMatrix.h"
#include "VertexResource.h"
#include "Model.h"
#include "Config.h"

class CylinderMesh : public Model
{
public:
	ID3D12Resource* CreateVertexResource_(ID3D12Device* device)override;
	ID3D12Resource* CreateIndexResource_(ID3D12Device* device)override;
	int GetVertexNum() { return MeshBuffer::vertexNum_; }
	void SetCylinderDivide(int divide) { kCylinderDivide_ = divide; }
	void SetTopRadius(float radius) { kTopRadius = radius; }
	void SetBottomRadius(float radius) { kBottomRadius = radius; }
	void SetHeight(float height) { kHeight = height; }
	void SetReverseY(bool isReverse) { isReverseY_ = isReverse; }

private:
	int kCylinderDivide_ = 32;
	float kTopRadius = 1.0f;
	float kBottomRadius = 1.0f;
	float kHeight = 3.0f;
	bool isReverseY_ = false;
};

