#pragma once
#include "LinearAlgebra/Vector4.h"
#include "LinearAlgebra/Vector3.h"

struct DirectionalLightGPU
{
	Vector4 color;
	Vector3 direction;
	float intensity;
};

