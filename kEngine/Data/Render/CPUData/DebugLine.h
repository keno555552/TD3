#pragma once
#include "LinearAlgebra/Vector3.h"
#include "LinearAlgebra/Vector4.h"

struct DebugLine
{
	Vector3 startPoint{};
	Vector3 endPoint{};
	Vector4 color{};
};