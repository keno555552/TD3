#include "Quaternion.h"

#include <math.h>
#include "MathsIncluder.h"

#ifdef USE_NOVICE
#include "Novice.h"
#endif


Quaternion Quaternion::operator+(const Quaternion& target) {
	return { x + target.x, y + target.y, z + target.z , w + target.w };
}

Quaternion Quaternion::operator-(const Quaternion& target) {
	return { x - target.x, y - target.y, z - target.z , w - target.w };
}

Quaternion Quaternion::operator*(const Quaternion& target) {
	return {
		w * target.x + x * target.w + y * target.z - z * target.y,
		w * target.y - x * target.z + y * target.w + z * target.x,
		w * target.z + x * target.y - y * target.x + z * target.w,
		w * target.w - x * target.x - y * target.y - z * target.z
	};
}

Quaternion Quaternion::operator*(const float& target) {
	return { x * target, y * target, z * target , w * target };
}

Quaternion Quaternion::operator/(const Quaternion& target) {
	return { x / target.x, y / target.y, z / target.z , w / target.w };
}

Quaternion Quaternion::operator-() const {
	return { -x, -y, -z, -w };
}

Quaternion Quaternion::operator=(const Quaternion& target) {
	if (this != &target) {
		x = target.x;
		y = target.y;
		z = target.z;
		w = target.w;
	}
	return *this;
}

float Dot(const Quaternion& q1, const Quaternion& q2) {
	return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
}

Quaternion Multiply(const Quaternion& m1, const Quaternion& m2) {
	Quaternion result = m1;
	return result * m2;
}

#ifdef USE_NOVICE
void QuaternionScreenPrintf(int x, int y, Quaternion& quaternion, const char* name) {

	if (name == nullptr) {
		Novice::ScreenPrintf(x, y, "%.02f   %.02f   %.02f   %.02f", quaternion.x, quaternion.y, quaternion.z, quaternion.w);
	} else {
		Novice::ScreenPrintf(x, y, "%.02f   %.02f   %.02f   %.02f   %s", quaternion.x, quaternion.y, quaternion.z, quaternion.w, name);
	}
}
#endif // 

Quaternion IdentityQuaternion() {
	return { 0.0f, 0.0f, 0.0f, 1.0f };
}

Quaternion Conjugate(const Quaternion& target) {
	return { -target.x,-target.y,-target.z,target.w };
}

float Norm(const Quaternion& target) {
	return sqrtf(target.x * target.x + target.y * target.y + target.z * target.z + target.w * target.w);
}

Quaternion Normalize(const Quaternion& target) {
	float norm = Norm(target);
	if (norm != 0) {
		return { target.x / norm, target.y / norm, target.z / norm, target.w / norm };
	}
	return { 0,0,0,0 };
}

Quaternion Inverse(const Quaternion& target) {
	float sqNorm = target.x * target.x + target.y * target.y + target.z * target.z + target.w * target.w;

	if (sqNorm <= 1e-12f) {
		return { 0,0,0,0 };
	}

	Quaternion c = Conjugate(target);
	float invSq = 1.0f / sqNorm;
	return { c.x * invSq, c.y * invSq, c.z * invSq, c.w * invSq };
}

Quaternion Slerp(const Quaternion& q1, const Quaternion& q2, float t) {
	Quaternion q1n = Normalize(q1);
	Quaternion q2n = Normalize(q2);

	float dot = Dot(q1, q2);

	if (dot < 0) {
		q1n = -q1n;
		dot = -dot;
	}

	const float EPSILON = 1e-6f;
	if (dot > 1.0f - EPSILON) {
		// 線形補間の後に正規化
		Quaternion result = q1n * (1.0f - t) + q2n * t;
		return Normalize(result);
	}

	float theta = acosf(dot);
	float sinTheta = sinf(theta);

	float w1 = sinf((1.0f - t) * theta) / sinTheta;
	float w2 = sinf(t * theta) / sinTheta;

	return q1n * w1 + q2n * w2;
}

Quaternion MakeRotateAxisAngleQuaternion(const Vector3& axis, float angle) {
	Vector3 nAxis = Normalize(axis);
	if (nAxis.x == 0 && nAxis.y == 0 && nAxis.z == 0) {
		return IdentityQuaternion();
	}

	float halfAngle = angle * 0.5f;
	float sinHalfAngle = sinf(halfAngle);
	float cosHalfAngle = cosf(halfAngle);
	return {
		nAxis.x * sinHalfAngle,
		nAxis.y * sinHalfAngle,
		nAxis.z * sinHalfAngle,
		cosHalfAngle
	};
}

Vector3 RotateVector(const Vector3& vector, const Quaternion& quaternion) {
	Quaternion q = Normalize(quaternion); 
	Quaternion p = { vector.x, vector.y, vector.z, 0.0f };
	Quaternion qInv = Conjugate(q); 

	Quaternion result = q * p * qInv;

	return { result.x, result.y, result.z };


}

Quaternion EulerToQuaternion(const Vector3& euler) {
	float cx = cosf(euler.x * 0.5f);
	float sx = sinf(euler.x * 0.5f);
	float cy = cosf(euler.y * 0.5f);
	float sy = sinf(euler.y * 0.5f);
	float cz = cosf(euler.z * 0.5f);
	float sz = sinf(euler.z * 0.5f);

	Quaternion q;

	// xyz 順序
	//q.w = cz * cy * cx + sz * sy * sx;
	//q.x = cz * cy * sx - sz * sy * cx;
	//q.y = cz * sy * cx + sz * cy * sx;
	//q.z = sz * cy * cx - cz * sy * sx;

	q.w = cz * cy * cx + sz * sy * sx;
	q.x = cz * cy * sx - sz * sy * cx;
	q.y = cz * sy * cx + sz * cy * sx;
	q.z = sz * cy * cx - cz * sy * sx;

	return q;
}


Vector3 QuaternionToEuler(const Quaternion& q) {
	Vector3 euler;

	float x = q.x;
	float y = q.y;
	float z = q.z;
	float w = q.w;

	// yaw (Y)
	float siny = 2.0f * (w * y + x * z);
	float cosy = 1.0f - 2.0f * (y * y + x * x);
	euler.y = atan2f(siny, cosy);

	// pitch (X)
	float sinp = 2.0f * (w * x - y * z);
	sinp = sinp > 1.0f ? 1.0f : sinp;
	sinp = sinp < -1.0f ? -1.0f : sinp;
	euler.x = asinf(sinp);

	// roll (Z)
	float sinr = 2.0f * (w * z + x * y);
	float cosr = 1.0f - 2.0f * (z * z + y * y);
	euler.z = atan2f(sinr, cosr);

	return euler;
}

Matrix4x4 MakeRotateMatrix(const Quaternion& q) {
	float x = q.x;
	float y = q.y;
	float z = q.z;
	float w = q.w;

	float xx = x * x;
	float yy = y * y;
	float zz = z * z;
	float xy = x * y;
	float xz = x * z;
	float yz = y * z;
	float wx = w * x;
	float wy = w * y;
	float wz = w * z;

	// row-major, v' = v * M，右手系
	return {
		1.0f - 2.0f * (yy + zz),  2.0f * (xy - wz),        2.0f * (xz + wy),        0.0f,
		2.0f * (xy + wz),         1.0f - 2.0f * (xx + zz), 2.0f * (yz - wx),        0.0f,
		2.0f * (xz - wy),         2.0f * (yz + wx),        1.0f - 2.0f * (xx + yy), 0.0f,
		0.0f,                     0.0f,                    0.0f,                    1.0f
	};
}

