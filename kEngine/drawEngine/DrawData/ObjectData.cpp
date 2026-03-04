#include "ObjectData.h"

Matrix4x4 ObjectPart::UpdateWorldMatrix() {
	Matrix4x4 localMatrix = MakeAffineMatrix(
		transform.scale,
		transform.rotate,
		transform.translate
	);
	Matrix4x4 parentMatrix = Identity();
	ObjectPart* parent = parentPart;
	while (parent != nullptr) {
		Matrix4x4 local = MakeAffineMatrix(
			parent->transform.scale,
			parent->transform.rotate,
			parent->transform.translate
		);
		parentMatrix = local * parentMatrix;
		parent = parent->parentPart;
	}

	return worldAnchorPointMatrix = localMatrix * parentMatrix;
}

bool ObjectPart::operator==(const ObjectPart& target) {
	if (transform.translate != target.transform.translate) return false;
	if (transform.rotate != target.transform.rotate) return false;
	if (transform.scale != target.transform.scale) return false;
	if (transformationMatrix != target.transformationMatrix) return false;
	if (*materialConfig != *(target.materialConfig)) return false;
    return true;
}
