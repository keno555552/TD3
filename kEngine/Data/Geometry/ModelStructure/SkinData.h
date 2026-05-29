#pragma once
#include <string>
#include "Data/Geometry/ModelStructure/JointWeightData.h"

struct SkinData
{
	int allVertexNum = 0;
	std::vector<int> jointIndices;
	std::vector<std::string> jointNames;
	std::vector<Matrix4x4> inverseBindPoseMatrices;
	std::vector<JointWeightData> skinClusterData;

	/// 三つのリストは同じ長さで、同じインデックスの要素が対応しているはず
	/// これで一つのmeshのスキニングに必要な情報が全部入っているはず
};

