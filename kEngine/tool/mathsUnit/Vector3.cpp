#include "Vector3.h"

Vector3 Vector3::operator+(const Vector3& target) {
	return { x + target.x, y + target.y , z + target.z };
}
Vector3 Vector3::operator-(const Vector3& target) {
	return { x - target.x, y - target.y , z - target.z };
}

Vector3 Vector3::operator*(const Vector3& target) {
	return { x * target.x, y * target.y };
}

Vector3 Vector3::operator/(const Vector3& target) {
	return { x / target.x, y / target.y, z / target.z };
}

Vector3 Vector3::operator*(const float& target) {
	return { x * target, y * target, z * target };
}

Vector3 Vector3::operator*=(const float& target)
{
	x *= target;
	y *= target;
	z *= target;
	return *this;
}

Vector3 operator*(float scalar, const Vector3& vec) {
	return { vec.x * scalar, vec.y * scalar, vec.z * scalar };
}

Vector3 Vector3::operator/(const float& target) {
	return { x / target, y / target, z / target };
}

Vector3 Vector3::operator+=(const Vector3& target) {
	*this = *this + target;
	return *this;
}


Vector3 Vector3::operator*=(const Vector3& target) {
	*this = *this * target;
	return *this;
}

bool Vector3::operator==(const Vector3& target) const{
	if (x != target.x)return false;
	if (y != target.y)return false;
	if (z != target.z)return false;
	return true;
}

bool Vector3::operator!=(const Vector3& target) const {
	return !(*this == target);
}




Vector3 Vector3::operator*(const int& target) {
	return { x * (float)target, y * (float)target, z * (float)target };
}


Vector3 operator*(int scalar, const Vector3& vec) {
	return { vec.x * (float)scalar, vec.y * (float)scalar , vec.z * (float)scalar };
}

Vector3 Vector3::helmholtz() {
	return { sqrtf(x * x),sqrtf(y * y),sqrtf(z * z) };
}

Vector3 Vector3::normalize() {
	Vector3 length = helmholtz();
	if (length.x != 0 && length.y != 0 && length.z != 0) {
		return { x / length.x, y / length.y, z / length.z };
	}
	return Vector3();
}

Vector3 Add(const Vector3& V1, const Vector3& V2) {
	return { V1.x + V2.x, V1.y + V2.y , V1.z + V2.z };
}

Vector3 Subtract(const Vector3& V1, const Vector3& V2) {
	return { V1.x - V2.x, V1.y - V2.y , V1.z - V2.z };
}

Vector3 Multiply(const float& scalar, const Vector3& v) {
	return { scalar * v.x, scalar * v.y, scalar * v.z };
}

float Dot(const Vector3& V1, const Vector3& V2) {
	return V1.x * V2.x + V1.y * V2.y + V1.z * V2.z;
}

float Length(const Vector3& v) {
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vector3 Normalize(const Vector3& v) {
	float length = Length(v);
	if (length != 0) {
		return { v.x / length, v.y / length, v.z / length };
	}
	return Vector3();
}

Vector3 Cross(const Vector3& v1, const Vector3& v2) {
	return { (v1.y * v2.z) - (v1.z * v2.y),
			 (v1.z * v2.x) - (v1.x * v2.z),
			 (v1.x * v2.y) - (v1.y * v2.x) };
}