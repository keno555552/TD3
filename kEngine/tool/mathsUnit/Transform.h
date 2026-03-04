#pragma once
#include "Vector3.h"

typedef struct Transform {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
}Transform;

Transform CreateDefaultTransform();
