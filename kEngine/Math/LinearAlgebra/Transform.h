#pragma once
#include "Vector3.h"
#include "Quaternion.h"

class DrawDataCollector;
class AnimationManager;
class AnimationUnit;
class Transform {
public:
	Vector3 scale{1.0f,1.0f,1.0f};
	Vector3 rotate{};
	Vector3 translate{};

private:
	friend class DrawDataCollector;
	friend class AnimationManager;
	friend class AnimationUnit;

	/// 回転のクォータニオン関連
	bool eulerDirty = true;
	Vector3 lastRotate{};
	Quaternion rotateQuat{};

	/// 動画Transform関連
	bool isAnimated = false;
	Vector3 aniScale{ 1.0f,1.0f,1.0f };
	Quaternion aniRotate{};
	Vector3 aniTranslate{};

private:
	bool CheckRotateDirty();
	void UpdateRotateQuat();
	void CopyAniTranFrom(Transform& target);
};

Transform CreateDefaultTransform();
