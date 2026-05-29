#pragma once
#include "Data/Render/CPUData/VertexData.h"
#include "LinearAlgebra/Transform.h"
#include "LinearAlgebra/TransformationMatrix.h"
#include "VertexResource.h"
#include "Model.h"
#include "Config.h"

class SphereMesh :public Model
{
public:
	ID3D12Resource* CreateVertexResource_(ID3D12Device* device)override;
	ID3D12Resource* CreateIndexResource_(ID3D12Device* device)override;
	int GetVertexNum() { return VertexNum_; }
	void SetLat(int divideNum) { kLatitudeSegments_ = divideNum; };
	void SetLong(int divideNum) { kLongitudeSegments_ = divideNum; };

private:
	int kDefaultDivision_ = config::GetDefaultSphereDivideNum();
	int kLatitudeSegments_ = 0;   // 緯度
	int kLongitudeSegments_ = 0;  // 經度
	int VertexNum_ = 0;
};

