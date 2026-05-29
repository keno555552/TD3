#include "RenderData.h"

bool RenderData::CheckSame(const RenderData target) {
	if (mesh != target.mesh)return false;
	if (materialID != target.materialID)return false;
	if (transformData != target.transformData)return false;
	if (psoKey != target.psoKey)return false;
	if (subMeshIndex != target.subMeshIndex)return false;
	return true;
}
