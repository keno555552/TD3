#pragma once
#include "MathsIncluder.h"
#include "Data/Geometry/Shape/crashIncluder.h"
#include "Object/Object.h"
#include "LinearAlgebra/Quaternion.h"

enum class AnimationType {
	LINEARITY,
	EASY_IN,
	EASY_OUT,
	EASY_IN_OUT,
	EASY_IN_BACK,
	EASY_OUT_BACK,
	STEP,
	CUBICSPLINE,
};

struct KeyframeVector3 {
	Vector3 value;
	float time;
};

struct KeyframeQuaternion {
	Quaternion value;
	float time;
};

//struct KeyFrame {
//	int index_{};
//	float time_{};
//	AnimationType animationType_{};
//	float easeRate_{};
//	Object transformData{};
//	HitBox hitBox_{};
//};

template<typename tValue>
struct KeyFrame {
	float time_{};
	tValue value_{};
	AnimationType animationType_{};
	float easeRate_{};
};

using KeyFrameVector3 = KeyFrame<Vector3>;
using KeyFrameQuaternion = KeyFrame<Quaternion>;