#pragma once
#include "Vector3.h"
#include "Vector4.h"

typedef struct Matrix4x4 {

	///定義
	float m[4][4];

	///単位行列
	Matrix4x4 Identity();
	///逆行列
	Matrix4x4 Inverse();
	///転置行列
	Matrix4x4 Transpose();

	///演算子再定義
	Matrix4x4 operator+(const Matrix4x4& target);
	Matrix4x4 operator-(const Matrix4x4& target);
	Matrix4x4 operator*(const Matrix4x4& target);
	Matrix4x4& operator=(const Matrix4x4& target);
	Matrix4x4 operator*(const float& target);
	friend Matrix4x4 operator*(float target, const Matrix4x4& vec);

	bool operator==(const Matrix4x4& target)const;
	bool operator!=(const Matrix4x4& target)const;

}Matrix4x4;

#pragma region 陣列転換

/// <summary>
/// 普通のVector3からMatrix4x4への変換
/// </summary>
/// <param name="translate">Vector3</param>
/// <returns>結果Matrix4x4</returns>
Matrix4x4 MakeTranslateMatrix4x4(const Vector3 translate);

/// <summary>
/// 移動のVector3からMatrix4x4への変換
/// </summary>
/// <param name="translate">Vector3</param>
/// <returns>結果Matrix4x4</returns>
Vector3 MakeTranslateVector3(const Matrix4x4 translate);

#pragma endregion

#pragma region 陣列表示
///// <summary>
///// Matrix4x4を描写する
///// </summary>
///// <param name="x">左上のposX</param>
///// <param name="y">左上のposY</param>
///// <param name="matrix">描写したいMatrix3x3</param>
//void Matrix4ScreenPrintf(int x, int y, Matrix4x4 matrix);
#pragma endregion

#pragma region 陣列計算

/// <summary>
/// 
/// </summary>
/// <returns></returns>
Matrix4x4 Identity();

/// <summary>
/// 逆行列を計算する(元は変わらない)
/// </summary>
/// <param name="matrix">目標Matrix4x4</param>
/// <returns>結果Matrix4x4</returns>
Matrix4x4 Inverse(Matrix4x4 matrix);

/// <summary>
/// 転置行列を計算する(元は変わらない)
/// </summary>
/// <param name="tranpose_">目標Matrix4x4</param>
/// <returns>結果Matrix4x4</returns>
Matrix4x4 Transpose(Matrix4x4 tranpose);

/// <summary>
/// 拡縮のVector3からMatrix4x4への変換
/// </summary>
/// <param name="scole">Vector3</param>
/// <returns>結果Matrix4x4</returns>
Matrix4x4 MakeScaleMatrix4x4(const Vector3 scole);

/// <summary>
/// 回転のVector3からMatrix4x4への変換
/// </summary>
/// <param name="rotate">Vector3</param>
/// <returns>結果Matrix4x4</returns>
Matrix4x4 MakeRotateMatrix4x4(const Vector3 rotate);

/// <summary>
/// 移動のVector3からMatrix4x4への変換
/// </summary>
/// <param name="translate">Vector3</param>
/// <returns>結果Matrix4x4</returns>
Matrix4x4 MakeTranslateMatrix(const Vector3 translate);


Vector3 ExtractScale(const Matrix4x4 matrix4x4);

Vector3 ExtractRotate(const Matrix4x4 matrix4x4);

Vector3 ExtractTranslate(const Matrix4x4 matrix4x4);


#pragma endregion

#pragma region カメラ改変陣列

/// <summary>
/// Matrix4x4のアフィン変換(Vector3から)
/// </summary>
/// <param name="scole">Vector3</param>
/// <param name="rotate">Vector3</param>
/// <param name="translate">Vector3</param>
/// <returns>結果Matrix4x4</returns>
Matrix4x4 MakeAffineMatrix(Vector3 scole, Vector3 rotate, Vector3 translate);

/// <summary>
/// 最大4つのMatrix4x4を掛け算する
/// </summary>
/// <param name="matrix1">Matrix4x4 その1</param>
/// <param name="matrix2">Matrix4x4 その2</param>
/// <param name="matrix3">Matrix4x4 その3(有無可)</param>
/// <param name="matrix4">Matrix4x4 その4(有無可)</param>
/// <returns>結果Matrix3x3</returns>
Matrix4x4 MatrixMix(Matrix4x4* matrix1,
	Matrix4x4* matrix2,
	Matrix4x4* matrix3 = nullptr,
	Matrix4x4* matrix4 = nullptr);

/// <summary>
/// ローカル座標からのワルド座標へ改変
/// </summary>
/// <returns>結果Matrix4x4</returns>
Matrix4x4 MakeWorldMatrix(Matrix4x4 origin);

/// <summary>
/// ビュー変換数列生成
/// </summary>
/// <returns>結果Matrix4x4</returns>
Matrix4x4 MakeViewMatrix(Vector3 scole, Vector3 rotate, Vector3 translate);

/// <summary>
/// 正射影変換数列生成
/// </summary>
/// <returns>結果Matrix4x4</returns>
Matrix4x4 MakeProjectionMatrix(float  leftO, float  rightO,
	float   topO, float bottomO,
	float  nearO, float    farO);

/// <summary>
/// 透視投影変換数列生成
/// </summary>
/// <returns>結果Matrix4x4</returns>
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio,
	float nearClip, float farClip);

/// <summary>
/// ビューポート変換数列生成
/// </summary>
/// <returns>結果Matrix4x4</returns>
Matrix4x4 MakeViewportMatrix(float   left, float   top,
	float  width, float  height,
	float   maxD, float    minD);

/// <summary>
/// ビューポート変換数列生成
/// </summary>
/// <returns>結果Matrix4x4</returns>
Matrix4x4 MakeOrthographicMatrix(float   left, float   top,
	float  right   , float  bottom,
	float  nearClip, float  farClip);

/// <summary>
///　3Dカメラの総まとめ変換
/// </summary>
/// <param name="obj">もの自身のワルド座標</param>
/// <param name="tranformMatrix">変換Matrix</param>
/// <returns></returns>
Vector3 viewFinalTransform(Vector3 obj, Matrix4x4 tranformMatrix);


/// <summary>
/// Matrix4x4で方向ベクトルを変換する
/// </summary>
/// <param name="dir"> 方向ベクトル</param>
/// <param name="m"> 変換Matrix4x4</param>
/// <returns>結果Vector3</returns>
Vector3 TransformDirection(Vector3 dir, const Matrix4x4& m);
#pragma endregion


