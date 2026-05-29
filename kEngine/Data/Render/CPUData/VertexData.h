#pragma once
#include "LinearAlgebra/Vector4.h"
#include "LinearAlgebra/Vector3.h"
#include "LinearAlgebra/Vector2.h"

struct VertexData
{
	Vector4 position;	// 頂点座標
	Vector2 texcoord;	// テクスチャ座標
	Vector3 normal;
};

