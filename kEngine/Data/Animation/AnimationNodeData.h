#pragma once
#include <vector>
#include "MathsIncluder.h"
#include "Data/Animation/Keyframe.h"

/// アニメーションのノードの補間方法(gltfの補間方法に合わせる)
enum class InterpolationMode {
	LINEARITY,
	STEP,
	CUBICSPLINE,
};


/// 一つのアニメーションのノードデータ
struct AnimationNodeData {

	int animationNodeID{};
	std::string name;

	std::vector <KeyFrameVector3> scaleList{};
	std::vector <KeyFrameQuaternion> rotateList{};
	std::vector <KeyFrameVector3> translationList{};
	AnimationNodeData* parent_ = nullptr;

	enum class KeyFrameType {
		SCALE,
		ROTATE,
		TRANSLATION,
		NumOfType,
	};
	void AddKeyFrame(int keyFrameType, float time_ = 0);
};
