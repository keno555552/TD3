#include "CornerData.h"

CornerData CreateDefaultCornerData() {
	CornerData	coner = { { {  0,	 0 },
							{  0, 1 },
							{ 1,1 },
							{ 1,	 0 } } };
	return coner;
}

bool CornerData::CheckSame(const CornerData target) {
	for (int i = 0; i < 4; i++) {
		if (coner[i].x != target.coner[i].x || coner[i].y != target.coner[i].y) {
			return false;
		}
	}
	return true;
}

bool CheckCornerDataNull(const CornerData& target) {
	for (int i = 0; i < 4; i++) {
		if (target.coner[i].x != 0.0f || target.coner[i].y != 0.0f) {
			return false;
		}
	}
	return true;
}

bool CheckCornerDataDefault(const CornerData& target) {
	CornerData defaultData = CreateDefaultCornerData();
	for (int i = 0; i < 4; i++) {
		if (target.coner[i].x != defaultData.coner[i].x || target.coner[i].y != defaultData.coner[i].y) {
			return false;
		}
	}
	return true;
}