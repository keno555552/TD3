#include "RenderData.h"

bool RenderData::CheckSame(const RenderData target) {
	if (mesh != target.mesh)return false;
	if (materialID != target.materialID)return false;
	if (transformData != target.transformData)return false;
	if (psoID != target.psoID)return false;
	if (subMeshIndex != target.subMeshIndex)return false;
	return true;
}
