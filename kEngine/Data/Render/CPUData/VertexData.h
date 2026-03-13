#pragma once
#include "Vector4.h"
#include "Vector3.h"
#include "Vector2.h"

struct VertexData
{
	Vector4 position;	// 頂点座標
	Vector2 texcoord;	// テクスチャ座標
	Vector3 normal;
};

