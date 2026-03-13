#pragma once
#include "Data/Render/CPUData/VertexData.h"
#include "Transform.h"
#include "TransformationMatrix.h"
#include "VertexResource.h"
#include "Config.h"
#include "Data/Render/CPUData/CornerData.h"

class SimpleSpriteMesh :public MeshBuffer
{
public:
	void SetSize(Vector2 RBpos);
	void SetSize(CornerData corner);
	void SetAnchor(Vector2 RBpos, Vector2 anchorPoint);
	void SetTexcoord(Vector2 textureSize,Vector2 cropLT, Vector2 cropSize);
	void ResetTexcoord();
	ID3D12Resource* CreateVertexResource_(ID3D12Device* device)override;
	void Mapping();
	ID3D12Resource* CreateIndexResource_(ID3D12Device* device)override;

	int GetVertexNum() { return 6; }
	bool CheckSize(Vector2 RBpos);
	bool CheckIsDefaultSize();
	void SetKeep(bool keep) { keep_ = keep; }
	bool GetKeep() { return keep_; }

	bool operator==(const SimpleSpriteMesh target);

private:
	CornerData conerData;
	Vector2 Texcoord[4]{	{0,0},
							{0,1},
							{1,1},
							{1,0}, };

	bool keep_ = false;
};

