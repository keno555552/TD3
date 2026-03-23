#pragma once
#include "Vector3.h"
#include "Vector4.h"

struct DebugLineGPU
{
	Vector3 startPoint;
	Vector3 endPoint;

	Vector4 color;

	float padding[2];
};