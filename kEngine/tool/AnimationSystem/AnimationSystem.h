#pragma once
#include <vector>
#include "kEngine.h"
#include "Object/Object.h"
#include "AnimationUnit.h"


class AnimationSystem
{
public:


private:
	std::vector<KeyFrame> keyFrameList_{};

	float time_ = 0.0f;
};

