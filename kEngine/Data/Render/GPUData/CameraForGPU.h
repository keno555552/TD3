#pragma once
#include "Vector3.h"

struct CameraForGPU
{
	Vector3 worldPosition; // カメラのワールド座標位置
	float padding;       // パディング（16バイトアライメントのため）
};

