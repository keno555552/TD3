#pragma once

struct Vector3;
struct Matrix4x4;
struct Quaternion {

	///定義
	float x, y, z, w;

	///演算子再定義
	Quaternion operator+(const Quaternion& target);
	Quaternion operator-(const Quaternion& target);
	Quaternion operator*(const Quaternion& target);
	Quaternion operator*(const float& target);
	Quaternion operator/(const Quaternion& target);
	Quaternion operator=(const Quaternion& target);
	Quaternion operator-() const;

};

/// <summary>
/// Matrix4x4を描写する
/// </summary>
/// <param name="x">左上のposX</param>
/// <param name="y">左上のposY</param>
/// <param name="matrix">描写したいMatrix3x3</param>
void QuaternionScreenPrintf(int x,int y,Quaternion& quaternion,const char* name);

// 内積
float Dot(const Quaternion& q1, const Quaternion& q2);
// 積
Quaternion Multiply(const Quaternion& m1, const Quaternion& m2);
// 単位行列の作成
Quaternion IdentityQuaternion();
// 共役
Quaternion Conjugate(const Quaternion& target);
// ノルム
float Norm(const Quaternion& target);
// 単位化
Quaternion Normalize(const Quaternion& target);
// 逆行列
Quaternion Inverse(const Quaternion& target);

// 球面線形補間
Quaternion Slerp(const Quaternion& q1, const Quaternion& q2, float t);


// 任意軸回転を表すQuaternionの生成
Quaternion MakeRotateAxisAngleQuaternion(
	const Vector3& axis, float angle);

// ベクトルをQuaternionで回転させた結果のベクトルを求める
Vector3 RotateVector(const Vector3& vector, const Quaternion& quaternion);

// オイラー角からQuaternionを求める
Quaternion EulerToQuaternion(const Vector3& euler);

// Quaternionからオイラー角を求める
Vector3 QuaternionToEuler(const Quaternion& quaternion);

// Quaternionから回転行列を求める
Matrix4x4 MakeRotateMatrix(const Quaternion& quaternion);


