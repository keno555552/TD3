#pragma once
#include "MathsIncluder.h"

enum class CornerName{
	TOP_LEFT = 0,
	BOTTOM_LEFT,
	BOTTOM_RIGHT,
	TOP_RIGHT
};

struct CornerData;
CornerData CreateDefaultCornerData();

struct CornerData{
	Vector2	coner[4] = { CreateDefaultCornerData().coner[0],
						 CreateDefaultCornerData().coner[1],
						 CreateDefaultCornerData().coner[2],
						 CreateDefaultCornerData().coner[3], };
		
	bool CheckSame(const CornerData target);
};

bool CheckCornerDataNull(const CornerData& target);

bool CheckCornerDataDefault(const CornerData& target);
