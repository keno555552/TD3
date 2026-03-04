#pragma once
#include <stdint.h>
#include "Vector4.h"
#include "Matrix4x4.h"
#include "MaterialConfig.h"

struct Material
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

	bool operator==(const Material target);
	bool operator!=(const Material target);
	void inputMaterialConfig(MaterialConfig target);
};

