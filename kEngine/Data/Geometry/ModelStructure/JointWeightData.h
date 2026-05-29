#pragma once
#include <vector>
#include <memory>
#include "Matrix4x4.h"
#include "Data/Geometry/ModelStructure/VertexWeightData.h"

struct JointWeightData
{
	Matrix4x4 inverseBindPoseMatrix{};
	std::vector<VertexWeightData> vertexWeights;
};

