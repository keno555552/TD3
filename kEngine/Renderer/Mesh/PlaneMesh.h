#pragma once
#include "Data/Render/CPUData/VertexData.h"
#include "Transform.h"
#include "TransformationMatrix.h"
#include "VertexResource.h"
#include "Config.h"
#include "Model.h"

class PlaneMesh :public Model
{
public:

	ID3D12Resource* CreateVertexResource_(ID3D12Device* device)override;
	ID3D12Resource* CreateIndexResource_(ID3D12Device* device)override;

private:
	enum {
		TOP_LEFT = 0,
		TOP_RIGHT,
		BOTTOM_LEFT,
		BOTTOM_RIGHT
	};

	Vector4 p[4]{
		{ -0.5f,  0.5f, 0.0f, 1.0f }, // TL
		{  0.5f,  0.5f, 0.0f, 1.0f }, // TR
		{ -0.5f, -0.5f, 0.0f, 1.0f }, // BL
		{  0.5f, -0.5f, 0.0f, 1.0f }  // BR
	};
};