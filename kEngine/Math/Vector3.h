#pragma once
#include <math.h>

typedef struct Vector3 {

	///定義
	float x, y, z;

	///演算子再定義
	Vector3 operator+(const Vector3& target);
	Vector3 operator-(const Vector3& target);
	Vector3 operator*(const Vector3& target);
	Vector3 operator/(const Vector3& target);
	Vector3 operator*(const float& target);
	Vector3 operator*=(const float& target);
	friend Vector3 operator*(float scalar, const Vector3& vec);
	Vector3 operator/(const float& target);
	Vector3 operator+=(const Vector3& target);
	Vector3 operator*=(const Vector3& target);
	bool operator==(const Vector3& target)const;
	bool operator!=(const Vector3& target)const;


	///int
	Vector3 operator*(const int& target);
	friend Vector3 operator*(int scalar, const Vector3& vec);

	///ヘルム
	Vector3 helmholtz();
	///正規化
	Vector3 normalize();
}Vector3;

// 加算
Vector3 Add(const Vector3& v1, const Vector3& v2);

// 減算
Vector3 Subtract(const Vector3& v1, const Vector3& v2);

// スカラー倍
Vector3 Multiply(const float& scalar, const Vector3& v);

// 内積
float Dot(const Vector3& v1, const Vector3& v2);

// 長さ（ノルム）
float Length(const Vector3& v);

// 正規化
Vector3 Normalize(const Vector3& v);

/// <summary>
/// cross積を求める
/// </summary>
/// <param name="v1">線A</param>
/// <param name="v2">線B</param>
/// <returns></returns>
Vector3 Cross(const Vector3& v1, const Vector3& v2);

