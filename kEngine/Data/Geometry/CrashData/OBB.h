#pragma once
#include "Vector3.h"

/// OBB
struct OBB final {
	Vector3 oriertations[3];
	Vector3 center;
	Vector3 size;
};