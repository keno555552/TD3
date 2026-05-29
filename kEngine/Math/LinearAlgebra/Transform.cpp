#include "Transform.h"

Transform CreateDefaultTransform() {
	Transform defaultTransform;
	defaultTransform.scale = { 1.0f,1.0f,1.0f };
	defaultTransform.rotate = { 0.0f,0.0f,0.0f };
	defaultTransform.translate = { 0.0f,0.0f,0.0f };

	return defaultTransform;
}

bool Transform::CheckRotateDirty() {
	if (lastRotate != rotate) {
		lastRotate = rotate;
		eulerDirty = true;
		return true;
	}
	eulerDirty = false;
	return false;
}

void Transform::UpdateRotateQuat() {
	rotateQuat = EulerToQuaternion(rotate);
	eulerDirty = false;
}

void Transform::CopyAniTranFrom(Transform& target) {
	aniScale = target.aniScale;
	aniRotate = target.aniRotate;
	aniTranslate = target.aniTranslate;

	isAnimated = target.isAnimated;
}
