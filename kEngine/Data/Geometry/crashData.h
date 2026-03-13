#pragma once
#include "MathsIncluder.h"

/// 球体
typedef struct Sphere {
	Vector3 center;
	float radius;
}Sphere;


struct HitBox final {
	Vector2 midPos;
	float width;
	float height;
};

struct AABB final {
	Vector3 min;
	Vector3 max;
};

struct OBB final {
	Vector3 oriertations[3];
	Vector3 center;
	Vector3 size;
};

struct Capsule final {
	Vector3 start;
	Vector3 end;
	float radius;
};