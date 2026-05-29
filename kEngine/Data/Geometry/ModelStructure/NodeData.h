#pragma once
#include <cassert>
#include <vector>
#include <string>
#include "Matrix4x4.h"
#include "Quaternion.h"
#include "Vector3.h"

struct NodeData
{
	std::string name;

	Vector3 translation;
	Quaternion rotation;
	Vector3 scale;
	
	Matrix4x4 localMatrix = Identity();
	Matrix4x4 globalMatrix = Identity();

	uint32_t parentIndex = (uint32_t)-1;

	uint32_t meshIndex = (uint32_t)-1;
	uint32_t skinIndex = (uint32_t)-1;

	std::vector<uint32_t> children;
};

