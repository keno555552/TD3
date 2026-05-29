#include "VectorUtility.h"

Vector3 Perpendicular(const Vector3& vector) {
	if (vector.x != 0.0f || vector.y != 0.0f) {
		return { -vector.y, vector.x, 0.0f }; // 1
	}
	return Vector3(0.0f, -vector.z, vector.y);
}

Vector3 ReflectVector3(const Vector3& input, const Vector3& normal) {
	Vector3 i = input;
	Vector3 n = normal;
	return  (i + Dot(i, n) * n * -2.0f);
}