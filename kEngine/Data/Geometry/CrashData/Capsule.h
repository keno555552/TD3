#pragma once
#include "Vector3.h"

/// カプセル
struct Capsule final {
	Vector3 start;
	Vector3 end;
	float radius;
};