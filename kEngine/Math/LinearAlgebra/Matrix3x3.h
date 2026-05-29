#pragma once
#include "Vector2.h"
#include <cmath>
#include <cassert>

typedef struct Matrix3x3 {

	///定義
	float m[3][3];

}Matrix3x3;

///演算子再定義
Matrix3x3 operator+(const Matrix3x3& A, const Matrix3x3& B);
Matrix3x3 operator-(const Matrix3x3& A, const Matrix3x3& B);
Matrix3x3 operator*(const Matrix3x3& A, const Matrix3x3& B);

////////////////////////////////////////////////////////////////////////////陣列変換

/// <summary>
/// Vector2からMatrix3x3への変換
/// </summary>
/// <param name="translate">Vector2</param>
/// <returns>結果Matrix3x3</returns>
Matrix3x3 MakeTranslateMatrix3x3(const Vector2 translate);

/// <summary>
/// Matrix3x3からVector2への変換
/// </summary>
/// <param name="translate">Matrix3x3</param>
/// <returns>結果Vector2</returns>
Vector2 MakeTranslateVector2(const Matrix3x3 translate);

////////////////////////////////////////////////////////////////////////////陣列計算

/// <summary>
/// Matrix3x3掛けるVector2
/// </summary>
/// <param name="changeMatrix">変換したいMatrix3x3</param>
/// <param name="vector">Vector2</param>
/// <returns>結果Matrix3x3</returns>
Vector2 Vector2xMatrix3x3(const Vector2 vector, const Matrix3x3 matrix);

/// <summary>
/// 回転行列行列
/// </summary>
/// <param name="theta">回転角度(radian)</param>
/// <returns></returns>
Matrix3x3 MakeRotateMatrix(const float theta);

/// <summary>
/// Matrix3x3の逆行列を探す
/// </summary>
/// <param name="matrix">元のMatrix3x3</param>
/// <returns>結果Matrix3x3</returns>
Matrix3x3 Inverse(Matrix3x3 matrix);

/// <summary>
/// Matrix3x3の転置行列を探す
/// </summary>
/// <param name="matrix">元のMatrix3x3</param>
/// <returns>結果Matrix3x3</returns>
Matrix3x3 Transpose(Matrix3x3 matrix);

////////////////////////////////////////////////////////////////////////////陣列表示

///// <summary>
///// Matrix3x3を描写する
///// </summary>
///// <param name="x">左上のposX</param>
///// <param name="y">左上のposY</param>
///// <param name="matrix">描写したいMatrix3x3</param>
//void MatrixScreenPrintf(int x, int y, Matrix3x3 matrix);

////////////////////////////////////////////////////////////////////////////陣列結合

///// <summary>
///// 使わない、例の結合用関数
///// </summary>
///// <param name="scale"></param>
///// <param name="rotate"></param>
///// <param name="translate"></param>
///// <returns></returns>
//Matrix3x3 MakeAffineMatrix(Vector2 scale, float rotate, Vector2 translate);

/// <summary>
/// 二つのMatrix3x3を掛け算する
/// </summary>
/// <param name="matrix1">Matrix3x3 その1</param>
/// <param name="matrix2">Matrix3x3 その2</param>
/// <returns>結果Matrix3x3</returns>
Matrix3x3 MatrixMix(Matrix3x3 matrix1, Matrix3x3 matrix2);

/// <summary>
/// 三つのMatrix3x3を掛け算する
/// </summary>
/// <param name="matrix1">Matrix3x3 その1</param>
/// <param name="matrix2">Matrix3x3 その2</param>
/// <param name="matrix2">Matrix3x3 その3</param>
/// <returns>結果Matrix3x3</returns>
Matrix3x3 MatrixMix(Matrix3x3 matrix1, Matrix3x3 matrix2, Matrix3x3 matrix3);

/// <summary>
/// 四つのMatrix3x3を掛け算する
/// </summary>
/// <param name="matrix1">Matrix3x3 その1</param>
/// <param name="matrix2">Matrix3x3 その2</param>
/// <param name="matrix2">Matrix3x3 その3</param>
/// <param name="matrix2">Matrix3x3 その4</param>
/// <returns>結果Matrix3x3</returns>
Matrix3x3 MatrixMix(Matrix3x3 matrix1, Matrix3x3 matrix2, Matrix3x3 matrix3, Matrix3x3 matrix4);


////////////////////////////////////////////////////////////////////////////world改変陣列

/// <summary>
/// ローカル座標からのワルド座標へ改変
/// </summary>
/// <returns></returns>
Matrix3x3 MakeWorldMatrix(Matrix3x3 origin);

/// <summary>
/// ビュー変換数列生成
/// </summary>
/// <returns></returns>
Matrix3x3 MakeViewMatrix(Matrix3x3 origin, Vector2 pos);

/// <summary>
/// 正射影変換数列生成
/// </summary>
/// <returns></returns>
Matrix3x3 MakeOrthographicMatrix(Vector2 leftTop, Vector2 rightBottom);

/// <summary>
/// ビューポート変換数列生成
/// </summary>
/// <returns></returns>
Matrix3x3 MakeViewportMatrix(Vector2 leftTop, Vector2 rightBottom);


