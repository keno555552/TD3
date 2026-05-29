#include "CircularMation.h"
#define M_PI 3.1415926f
#include <cmath>

Vector3 uniformCircularMotion(Vector3 center, float radius, float time, float maxtime, CircleMotionPlane plane) {
	center;
	Vector3 result = {};

	float omega = 2.0f * M_PI / maxtime; // 角速度を計算
	float nowAngle = omega * time; // 現在の角度を計算

	if (plane == CircleMotionPlane::XY) {
		result.x = -std::sinf(nowAngle) * radius * omega;
		result.y = std::cosf(nowAngle) * radius * omega;
		result.z = 0;  // Z座標は変化しない
		return result + center;
	}
	if (plane == CircleMotionPlane::XZ) {
		result.x = -std::sinf(nowAngle) * radius * omega;
		result.z = std::cosf(nowAngle) * radius * omega;
		result.y = 0;  // y座標は変化しない		    
		return result + center;
	}
	if (plane == CircleMotionPlane::YZ) {
		result.y = -std::sinf(nowAngle) * radius * omega;
		result.z = std::cosf(nowAngle) * radius * omega;
		result.x = 0;  // x座標は変化しない
		return result + center;
	}
	return Vector3();
}