#include "TransformationMatrix.h"

bool TransformationMatrix::operator==(const TransformationMatrix target) {
    if (WVP != target.WVP)return false;
    if (world != target.world)return false;
	if (WorldInverseTranspose != target.WorldInverseTranspose)return false;
    return true;
}

bool TransformationMatrix::operator!=(const TransformationMatrix target) {
    return !(*this == target);
}
