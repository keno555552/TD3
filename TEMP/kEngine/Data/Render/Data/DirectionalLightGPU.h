#pragma once
#include "Vector4.h"
#include "Vector3.h"

struct DirectionalLightGPU
{
	Vector4 color;
	Vector3 direction;
	float intensity;
};

