#pragma once
#include "MathsIncluder.h"
#include "Data/Physics/PhysicalObject.h"

struct Spring {
	Vector3 anchor;
	PhysicalObject end; // バネの片端の物体
	float naturalLength; //自然長
	float  stiffness; //剛性。バネ定数k
	float dampingCoefficient; //減衰係数
};