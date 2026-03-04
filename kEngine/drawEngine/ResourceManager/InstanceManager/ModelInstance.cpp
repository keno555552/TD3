#include "ModelInstance.h"

bool ModelInstance::CheckSame(const ModelInstance target) {
	if (transformData != target.transformData)return false;
	if (materialConfigIndex != target.materialConfigIndex)return false;
	if (resourceHandle != target.resourceHandle)return false;
	if (modelHandle != target.modelHandle)return false;
	if (modelGroupHandle != target.modelGroupHandle)return false;
	return true;
}