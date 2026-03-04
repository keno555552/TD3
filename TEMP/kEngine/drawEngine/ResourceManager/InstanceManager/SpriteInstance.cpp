#include "SpriteInstance.h"

bool SpriteInstance::CheckSame(const SpriteInstance target) {
	if (position != target.position)return false;
	if (scale != target.scale)return false;
	if (rotate != target.rotate)return false;

	if (anchorPoint != target.anchorPoint)return false;
	if (cornerData.CheckSame(cornerData))return false;

	if (cropLT != target.cropLT)return false;
	if (cropSize != target.cropSize)return false;

	if (materialConfigIndex != target.materialConfigIndex)return false;
	if (resourceHandle != target.resourceHandle)return false;
	if (layer != target.layer)return false;
	return true;
}