#pragma once
#include "Vector2.h"
#include "Vector3.h"
#include "DrawData/CornerData.h"

struct SpriteInstance {
	Vector3 position{};
	Vector2 scale{ 1.0f,1.0f };
	Vector3 rotate{};

	Vector2 anchorPoint{};
	CornerData cornerData{ CreateDefaultCornerData() };

	Vector2 cropLT{};
	Vector2 cropSize{};

	int materialConfigIndex{};
	int resourceHandle{};
	int layer{};
	int drawState{};

	bool CheckSame(const SpriteInstance target);
};