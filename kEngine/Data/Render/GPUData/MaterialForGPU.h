#pragma once
#include <stdint.h>
#include "Vector4.h"
#include "Matrix4x4.h"
#include "Data/Render/CPUData/MaterialConfig.h"

struct MaterialForGPU
{
	Vector4 color;
	Vector4 color2;
	Vector4 color3;

	int32_t enableLighting;
	float padding[3];

	Matrix4x4 uvTransform;
	float shininess;
	float intensity;
	float heightScale;

	float extra[5];

	bool operator==(const MaterialForGPU target);
	bool operator!=(const MaterialForGPU target);
	void inputMaterialConfig(MaterialConfig target);
};

