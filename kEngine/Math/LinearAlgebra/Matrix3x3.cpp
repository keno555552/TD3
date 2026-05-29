#include "Matrix3x3.h"

Matrix3x3 operator+(const Matrix3x3&A, const Matrix3x3& B)
{
	Matrix3x3 resuit = {};

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			resuit.m[i][j] = A.m[i][j] + B.m[i][j];
		}
	}

	return resuit;
}

Matrix3x3 operator-(const Matrix3x3& A, const Matrix3x3& B)
{
	Matrix3x3 resuit = {};

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			resuit.m[i][j] = A.m[i][j] - B.m[i][j];
		}
	}

	return resuit;
}

Matrix3x3 operator*(const Matrix3x3& A, const Matrix3x3& B)
{
	Matrix3x3 result = {};
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < 3; k++) {
				result.m[i][j] += A.m[i][k] * B.m[k][j];
			}
		}
	}
	return result;
}

////////////////////////////////////////////////////////////////////////////陣列変換

Matrix3x3 MakeTranslateMatrix3x3(const Vector2 translate) {
	return { 1,0,0,
			 0,1,0,
			  translate.x,translate.y,1 };
}

Vector2 MakeTranslateVector2(const Matrix3x3 translate) {
	return { translate.m[2][0],translate.m[2][1] };
}

////////////////////////////////////////////////////////////////////////////陣列計算

Vector2 Vector2xMatrix3x3(const Vector2 vector, const Matrix3x3 matrix) {
	Vector2 result;
	result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + 1.0f * matrix.m[2][0];
	result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + 1.0f * matrix.m[2][1];
	float w = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + 1.0f * matrix.m[2][2];
	assert(w != 0.0f);
	result.x /= w;
	result.y /= w;
	return result;
}

Matrix3x3 MakeRotateMatrix(const float theta) {
	return{ cosf(theta) ,sinf(theta),0,
			-sinf(theta),cosf(theta),0,
			0,0,1 };
}

Matrix3x3 Inverse(Matrix3x3 matrix) {
	float determinant = (matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][2] +
		matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][0] +
		matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][1])
		-
		(matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][0] +
			matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][2] +
			matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][1]);

	return Matrix3x3{ (matrix.m[1][1] * matrix.m[2][2] - matrix.m[1][2] * matrix.m[2][1]) / determinant,
					 -(matrix.m[0][1] * matrix.m[2][2] - matrix.m[0][2] * matrix.m[2][1]) / determinant,
					  (matrix.m[0][1] * matrix.m[1][2] - matrix.m[0][2] * matrix.m[1][1]) / determinant,

					 -(matrix.m[1][0] * matrix.m[2][2] - matrix.m[1][2] * matrix.m[2][0]) / determinant,
					  (matrix.m[0][0] * matrix.m[2][2] - matrix.m[0][2] * matrix.m[2][0]) / determinant,
					 -(matrix.m[0][0] * matrix.m[1][2] - matrix.m[0][2] * matrix.m[1][0]) / determinant,

					  (matrix.m[1][0] * matrix.m[2][1] - matrix.m[1][1] * matrix.m[2][0]) / determinant,
					 -(matrix.m[0][0] * matrix.m[2][1] - matrix.m[0][1] * matrix.m[2][0]) / determinant,
					  (matrix.m[0][0] * matrix.m[1][1] - matrix.m[0][1] * matrix.m[1][0]) / determinant };
}


Matrix3x3 Transpose(Matrix3x3 matrix) {

	return Matrix3x3{ matrix.m[0][0],matrix.m[1][0],matrix.m[2][0],
					  matrix.m[0][1],matrix.m[1][1],matrix.m[2][1],
					  matrix.m[0][2],matrix.m[1][2],matrix.m[2][2] };
}

////////////////////////////////////////////////////////////////////////////陣列表示

//void MatrixScreenPrintf(int x, int y, Matrix3x3 matrix) {
//	int kColumnWidth = 80;
//	int kRowHeight = 40;
//	for (int row = 0; row < 3; ++row) {
//		for (int column = 0; column < 3; ++column) {
//			Novice::ScreenPrintf(
//				x + column * kColumnWidth,
//				y + row * kRowHeight,
//				"%.02f",
//				matrix.m[row][column]
//			);
//		}
//	}
//}

////////////////////////////////////////////////////////////////////////////陣列結合

//Matrix3x3 MakeAffineMatrix(Vector2 scale, float rotate, Vector2 translate) {
//	return { scale.x * cosf(rotate),scale.x * sinf(rotate),0,
//			 -scale.y * sinf(rotate),scale.y * cosf(rotate),0,
//			  translate.x,translate.y,1 };
//}

Matrix3x3 MatrixMix(Matrix3x3 matrix1, Matrix3x3 matrix2)
{
	Matrix3x3 result = matrix1 * matrix2;
	return result;
}

Matrix3x3 MatrixMix(Matrix3x3 matrix1, Matrix3x3 matrix2, Matrix3x3 matrix3)
{
	Matrix3x3 result = matrix1 * matrix2 * matrix3;
	return result;
}

Matrix3x3 MatrixMix(Matrix3x3 matrix1, Matrix3x3 matrix2, Matrix3x3 matrix3, Matrix3x3 matrix4)
{
	Matrix3x3 result = matrix1 * matrix2 * matrix3 * matrix4;
	return result;
}

////////////////////////////////////////////////////////////////////////////world改変陣列

Matrix3x3 MakeWorldMatrix(Matrix3x3 origin) {
	return origin;
}

Matrix3x3 MakeViewMatrix(Matrix3x3 origin, Vector2 pos)
{
	Vector2 move = pos;
	Matrix3x3 move2 = MakeTranslateMatrix3x3(move);
	Matrix3x3 result = MatrixMix(origin, move2);
	return Inverse(result);
}

Matrix3x3 MakeOrthographicMatrix(Vector2 leftTop, Vector2 rightBottom)
{
	return{ 2.0f / (rightBottom.x - leftTop.x),0.0f,0.0f,
			0.0f,2.0f / (leftTop.y - rightBottom.y),0.0f,
			(leftTop.x + rightBottom.x) / (leftTop.x - rightBottom.x),(leftTop.y + rightBottom.y) / (rightBottom.y - leftTop.y),1.0f };
}

Matrix3x3 MakeViewportMatrix(Vector2 leftTop, Vector2 rightBottom)
{
	return{ rightBottom.x / 2.0f,0.0f,0.0f,
			0.0f,-(rightBottom.y / 2.0f),0.0f,
			leftTop.x + (rightBottom.x / 2.0f),leftTop.y + (rightBottom.y / 2.0f),1.0f };
}