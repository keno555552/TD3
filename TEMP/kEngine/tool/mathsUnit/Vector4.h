#pragma once

struct Matrix4x4; // forward declaration

struct Vector4
{
	float x;
	float y;
	float z;
	float w;

	Vector4 operator/= (const float& target);
	Vector4 operator+(const Vector4& target);
	Vector4 operator-(const Vector4& target);
	Vector4 operator=(const Vector4& target);
	Vector4 operator*(const Matrix4x4& target);
	friend Vector4 operator*(const Vector4& v, const Matrix4x4& m);

	bool operator==(const Vector4& target)const;
	bool operator!=(const Vector4& target)const;

	void ColorBy255();
	void ColorBy1();
};

// Row-vector (v * M) multiplication
Vector4 Multiply(const Vector4& v, const Matrix4x4& m);