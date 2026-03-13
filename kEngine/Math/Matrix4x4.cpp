#include "Matrix4x4.h"

Matrix4x4 Matrix4x4::Identity() {
	Matrix4x4 result = {};
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result.m[i][j] = 0;
			m[i][j] = 0;
			if (i == j) {
				result.m[i][j] = 1;
				m[i][j] = 1;
			}
		}
	}
	return result;
}

Matrix4x4 Matrix4x4::Inverse() {
	float determinant = m[0][0] * m[1][1] * m[2][2] * m[3][3]
		+ m[0][0] * m[1][2] * m[2][3] * m[3][1]
		+ m[0][0] * m[1][3] * m[2][1] * m[3][2]

		- m[0][0] * m[1][3] * m[2][2] * m[3][1]
		- m[0][0] * m[1][2] * m[2][1] * m[3][3]
		- m[0][0] * m[1][1] * m[2][3] * m[3][2]

		- m[0][1] * m[1][0] * m[2][2] * m[3][3]
		- m[0][2] * m[1][0] * m[2][3] * m[3][1]
		- m[0][3] * m[1][0] * m[2][1] * m[3][2]

		+ m[0][3] * m[1][0] * m[2][2] * m[3][1]
		+ m[0][2] * m[1][0] * m[2][1] * m[3][3]
		+ m[0][1] * m[1][0] * m[2][3] * m[3][2]

		+ m[0][1] * m[1][2] * m[2][0] * m[3][3]
		+ m[0][2] * m[1][3] * m[2][0] * m[3][1]
		+ m[0][3] * m[1][1] * m[2][0] * m[3][2]

		- m[0][3] * m[1][2] * m[2][0] * m[3][1]
		- m[0][2] * m[1][1] * m[2][0] * m[3][3]
		- m[0][1] * m[1][3] * m[2][0] * m[3][2]

		- m[0][1] * m[1][2] * m[2][3] * m[3][0]
		- m[0][2] * m[1][3] * m[2][1] * m[3][0]
		- m[0][3] * m[1][1] * m[2][2] * m[3][0]

		+ m[0][3] * m[1][2] * m[2][1] * m[3][0]
		+ m[0][2] * m[1][1] * m[2][3] * m[3][0]
		+ m[0][1] * m[1][3] * m[2][2] * m[3][0];

	return Matrix4x4{ (m[1][1] * m[2][2] * m[3][3]
					 + m[1][2] * m[2][3] * m[3][1]
					 + m[1][3] * m[2][1] * m[3][2]
					 - m[1][3] * m[2][2] * m[3][1]
					 - m[1][2] * m[2][1] * m[3][3]
					 - m[1][1] * m[2][3] * m[3][2]) / determinant,/// 11

					(-m[0][1] * m[2][2] * m[3][3]
					 - m[0][2] * m[2][3] * m[3][1]
					 - m[0][3] * m[2][1] * m[3][2]
					 + m[0][3] * m[2][2] * m[3][1]
					 + m[0][2] * m[2][1] * m[3][3]
					 + m[0][1] * m[2][3] * m[3][2]) / determinant,/// 12

					(+m[0][1] * m[1][2] * m[3][3]
					 + m[0][2] * m[1][3] * m[3][1]
					 + m[0][3] * m[1][1] * m[3][2]
					 - m[0][3] * m[1][2] * m[3][1]
					 - m[0][2] * m[1][1] * m[3][3]
					 - m[0][1] * m[1][3] * m[3][2]) / determinant,/// 13

					(-m[0][1] * m[1][2] * m[2][3]
					 - m[0][2] * m[1][3] * m[2][1]
					 - m[0][3] * m[1][1] * m[2][2]
					 + m[0][3] * m[1][2] * m[2][1]
					 + m[0][2] * m[1][1] * m[2][3]
					 + m[0][1] * m[1][3] * m[2][2]) / determinant,/// 14

					(-m[1][0] * m[2][2] * m[3][3]
					 - m[1][2] * m[2][3] * m[3][0]
					 - m[1][3] * m[2][0] * m[3][2]
					 + m[1][3] * m[2][2] * m[3][0]
					 + m[1][2] * m[2][0] * m[3][3]
					 + m[1][0] * m[2][3] * m[3][2]) / determinant,/// 21

					(+m[0][0] * m[2][2] * m[3][3]
					 + m[0][2] * m[2][3] * m[3][0]
					 + m[0][3] * m[2][0] * m[3][2]
					 - m[0][3] * m[2][2] * m[3][0]
					 - m[0][2] * m[2][0] * m[3][3]
					 - m[0][0] * m[2][3] * m[3][2]) / determinant,/// 22

					(-m[0][0] * m[1][2] * m[3][3]
					 - m[0][2] * m[1][3] * m[3][0]
					 - m[0][3] * m[1][0] * m[3][2]
					 + m[0][3] * m[1][2] * m[3][0]
					 + m[0][2] * m[1][0] * m[3][3]
					 + m[0][0] * m[1][3] * m[3][2]) / determinant,/// 23

					(+m[0][0] * m[1][2] * m[2][3]
					 + m[0][2] * m[1][3] * m[2][0]
					 + m[0][3] * m[1][0] * m[2][2]
					 - m[0][3] * m[1][2] * m[2][0]
					 - m[0][2] * m[1][0] * m[2][3]
					 - m[0][0] * m[1][3] * m[2][2]) / determinant,/// 24

					(+m[1][0] * m[2][1] * m[3][3]
					 + m[1][1] * m[2][3] * m[3][0]
					 + m[1][3] * m[2][0] * m[3][1]
					 - m[1][3] * m[2][1] * m[3][0]
					 - m[1][1] * m[2][0] * m[3][3]
					 - m[1][0] * m[2][3] * m[3][1]) / determinant,/// 31

					(-m[0][0] * m[2][1] * m[3][3]
					 - m[0][1] * m[2][3] * m[3][0]
					 - m[0][3] * m[2][0] * m[3][1]
					 + m[0][3] * m[2][1] * m[3][0]
					 + m[0][1] * m[2][0] * m[3][3]
					 + m[0][0] * m[2][3] * m[3][1]) / determinant,/// 32

					(+m[0][0] * m[1][1] * m[3][3]
					 + m[0][1] * m[1][3] * m[3][0]
					 + m[0][3] * m[1][0] * m[3][1]
					 - m[0][3] * m[1][1] * m[3][0]
					 - m[0][1] * m[1][0] * m[3][3]
					 - m[0][0] * m[1][3] * m[3][1]) / determinant,/// 33

					(-m[0][0] * m[1][1] * m[2][3]
					 - m[0][1] * m[1][3] * m[2][0]
					 - m[0][3] * m[1][0] * m[2][1]
					 + m[0][3] * m[1][1] * m[2][0]
					 + m[0][1] * m[1][0] * m[2][3]
					 + m[0][0] * m[1][3] * m[2][1]) / determinant,/// 34

					(-m[1][0] * m[2][1] * m[3][2]
					 - m[1][1] * m[2][2] * m[3][0]
					 - m[1][2] * m[2][0] * m[3][1]
					 + m[1][2] * m[2][1] * m[3][0]
					 + m[1][1] * m[2][0] * m[3][2]
					 + m[1][0] * m[2][2] * m[3][1]) / determinant,/// 41

					(+m[0][0] * m[2][1] * m[3][2]
					 + m[0][1] * m[2][2] * m[3][0]
					 + m[0][2] * m[2][0] * m[3][1]
					 - m[0][2] * m[2][1] * m[3][0]
					 - m[0][1] * m[2][0] * m[3][2]
					 - m[0][0] * m[2][2] * m[3][1]) / determinant,/// 42

					(-m[0][0] * m[1][1] * m[3][2]
					 - m[0][1] * m[1][2] * m[3][0]
					 - m[0][2] * m[1][0] * m[3][1]
					 + m[0][2] * m[1][1] * m[3][0]
					 + m[0][1] * m[1][0] * m[3][2]
					 + m[0][0] * m[1][2] * m[3][1]) / determinant,/// 43

					(+m[0][0] * m[1][1] * m[2][2]
					 + m[0][1] * m[1][2] * m[2][0]
					 + m[0][2] * m[1][0] * m[2][1]
					 - m[0][2] * m[1][1] * m[2][0]
					 - m[0][1] * m[1][0] * m[2][2]
					 - m[0][0] * m[1][2] * m[2][1]) / determinant /// 44
	};
}

Matrix4x4 Matrix4x4::Transpose() {
	Matrix4x4 result = {};
	Matrix4x4 a = *this;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result.m[i][j] = a.m[j][i];
			m[i][j] = a.m[j][i];

		}
	}
	return result;
}

Matrix4x4 Matrix4x4::operator+(const Matrix4x4& target) {
	Matrix4x4 resuit = {};
	for (int x = 0; x < 4; x++) {
		for (int y = 0; y < 4; y++) {
			resuit.m[x][y] = m[x][y] + target.m[x][y];
		}
	}


	return resuit;
}

Matrix4x4 Matrix4x4::operator-(const Matrix4x4& target) {
	Matrix4x4 resuit = {};
	for (int x = 0; x < 4; x++) {
		for (int y = 0; y < 4; y++) {
			resuit.m[x][y] = m[x][y] - target.m[x][y];
		}
	}
	return resuit;
}

Matrix4x4 Matrix4x4::operator*(const Matrix4x4& target) {
	//Matrix3x3 result = {};
	//for (int i = 0; i < 3; i++) {
	//	for (int j = 0; j < 3; j++) {
	//		for (int k = 0; k < 3; k++) {
	//			result.m[i][j] += m[i][k] * target.m[k][j];
	//		}
	//	}
	//}
	//return result;

	Matrix4x4 resuit = {};
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			for (int k = 0; k < 4; k++) {
				resuit.m[i][j] += m[i][k] * target.m[k][j];
			}
		}
	}
	return resuit;
}

Matrix4x4 Matrix4x4::operator*(const float& target) {
	Matrix4x4 result = {};
	for (int x = 0; x < 4; x++) {
		for (int y = 0; y < 4; y++) {
			result.m[x][y] = m[x][y] * target;
		}
	}
	return result;
}


Matrix4x4& Matrix4x4::operator=(const Matrix4x4& target) {
	Matrix4x4 result = {};
	for (int x = 0; x < 4; x++) {
		for (int y = 0; y < 4; y++) {
			this->m[x][y] = target.m[x][y];
		}
	}
	return *this;
}

bool Matrix4x4::operator==(const Matrix4x4& target)const {

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (m[i][j] != target.m[i][j])return false;
		}
	}
	return true;
}

bool Matrix4x4::operator!=(const Matrix4x4& target)const {
	return !(*this == target);
}

Matrix4x4 operator*(float target, const Matrix4x4& vec) {
	float f = target;
	Matrix4x4 v = vec;
	return v * f;
}


#pragma region 陣列転換
Matrix4x4 MakeTranslateMatrix4x4(const Vector3 translate) {
	return Matrix4x4{ 1.0f,		  0.0f,		   0.0f, 0.0f,
							 0.0f,		  1.0f,		   0.0f, 0.0f,
							 0.0f,		  0.0f,		   1.0f, 0.0f,
					  translate.x, translate.y,	translate.z, 1.0f };
}

Vector3 MakeTranslateVector3(const Matrix4x4 translate) {
	return Vector3(translate.m[3][0], translate.m[3][1], translate.m[3][2]);
}
#pragma endregion

#pragma region 陣列表示
//void Matrix4ScreenPrintf(int x, int y, Matrix4x4 matrix) {
//	int kColumnWidth = 80;
//	int kRowHeight = 40;
//	for (int row = 0; row < 4; ++row) {
//		for (int column = 0; column < 4; ++column) {
//			Novice::ScreenPrintf(
//				x + column * kColumnWidth,
//				y + row * kRowHeight,
//				"%.02f",
//				matrix.m[row][column]
//			);
//		}
//	}
//}
#pragma endregion

#pragma region 陣列計算

Matrix4x4 Identity() {
	Matrix4x4 result = {};
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result.m[i][j] = 0;
			if (i == j) {
				result.m[i][j] = 1;
			}
		}
	}
	return result;
}

Matrix4x4 Inverse(Matrix4x4 matrix) {
	float determinant = matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][2] * matrix.m[3][3]
		+ matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][3] * matrix.m[3][1]
		+ matrix.m[0][0] * matrix.m[1][3] * matrix.m[2][1] * matrix.m[3][2]///

		- matrix.m[0][0] * matrix.m[1][3] * matrix.m[2][2] * matrix.m[3][1]
		- matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][1] * matrix.m[3][3]
		- matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][3] * matrix.m[3][2]

		- matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][2] * matrix.m[3][3]
		- matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][3] * matrix.m[3][1]
		- matrix.m[0][3] * matrix.m[1][0] * matrix.m[2][1] * matrix.m[3][2]

		+ matrix.m[0][3] * matrix.m[1][0] * matrix.m[2][2] * matrix.m[3][1]
		+ matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][1] * matrix.m[3][3]
		+ matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][3] * matrix.m[3][2]

		+ matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][0] * matrix.m[3][3]
		+ matrix.m[0][2] * matrix.m[1][3] * matrix.m[2][0] * matrix.m[3][1]
		+ matrix.m[0][3] * matrix.m[1][1] * matrix.m[2][0] * matrix.m[3][2]

		- matrix.m[0][3] * matrix.m[1][2] * matrix.m[2][0] * matrix.m[3][1]
		- matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][0] * matrix.m[3][3]
		- matrix.m[0][1] * matrix.m[1][3] * matrix.m[2][0] * matrix.m[3][2]

		- matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][3] * matrix.m[3][0]
		- matrix.m[0][2] * matrix.m[1][3] * matrix.m[2][1] * matrix.m[3][0]
		- matrix.m[0][3] * matrix.m[1][1] * matrix.m[2][2] * matrix.m[3][0]

		+ matrix.m[0][3] * matrix.m[1][2] * matrix.m[2][1] * matrix.m[3][0]
		+ matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][3] * matrix.m[3][0]
		+ matrix.m[0][1] * matrix.m[1][3] * matrix.m[2][2] * matrix.m[3][0];

	return Matrix4x4{ (matrix.m[1][1] * matrix.m[2][2] * matrix.m[3][3]
					 + matrix.m[1][2] * matrix.m[2][3] * matrix.m[3][1]
					 + matrix.m[1][3] * matrix.m[2][1] * matrix.m[3][2]
					 - matrix.m[1][3] * matrix.m[2][2] * matrix.m[3][1]
					 - matrix.m[1][2] * matrix.m[2][1] * matrix.m[3][3]
					 - matrix.m[1][1] * matrix.m[2][3] * matrix.m[3][2]) / determinant,/// 11

					(-matrix.m[0][1] * matrix.m[2][2] * matrix.m[3][3]
					 - matrix.m[0][2] * matrix.m[2][3] * matrix.m[3][1]
					 - matrix.m[0][3] * matrix.m[2][1] * matrix.m[3][2]
					 + matrix.m[0][3] * matrix.m[2][2] * matrix.m[3][1]
					 + matrix.m[0][2] * matrix.m[2][1] * matrix.m[3][3]
					 + matrix.m[0][1] * matrix.m[2][3] * matrix.m[3][2]) / determinant,/// 12

					(+matrix.m[0][1] * matrix.m[1][2] * matrix.m[3][3]
					 + matrix.m[0][2] * matrix.m[1][3] * matrix.m[3][1]
					 + matrix.m[0][3] * matrix.m[1][1] * matrix.m[3][2]
					 - matrix.m[0][3] * matrix.m[1][2] * matrix.m[3][1]
					 - matrix.m[0][2] * matrix.m[1][1] * matrix.m[3][3]
					 - matrix.m[0][1] * matrix.m[1][3] * matrix.m[3][2]) / determinant,/// 13

					(-matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][3]
					 - matrix.m[0][2] * matrix.m[1][3] * matrix.m[2][1]
					 - matrix.m[0][3] * matrix.m[1][1] * matrix.m[2][2]
					 + matrix.m[0][3] * matrix.m[1][2] * matrix.m[2][1]
					 + matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][3]
					 + matrix.m[0][1] * matrix.m[1][3] * matrix.m[2][2]) / determinant,/// 14

					(-matrix.m[1][0] * matrix.m[2][2] * matrix.m[3][3]
					 - matrix.m[1][2] * matrix.m[2][3] * matrix.m[3][0]
					 - matrix.m[1][3] * matrix.m[2][0] * matrix.m[3][2]
					 + matrix.m[1][3] * matrix.m[2][2] * matrix.m[3][0]
					 + matrix.m[1][2] * matrix.m[2][0] * matrix.m[3][3]
					 + matrix.m[1][0] * matrix.m[2][3] * matrix.m[3][2]) / determinant,/// 21

					(+matrix.m[0][0] * matrix.m[2][2] * matrix.m[3][3]
					 + matrix.m[0][2] * matrix.m[2][3] * matrix.m[3][0]
					 + matrix.m[0][3] * matrix.m[2][0] * matrix.m[3][2]
					 - matrix.m[0][3] * matrix.m[2][2] * matrix.m[3][0]
					 - matrix.m[0][2] * matrix.m[2][0] * matrix.m[3][3]
					 - matrix.m[0][0] * matrix.m[2][3] * matrix.m[3][2]) / determinant,/// 22

					(-matrix.m[0][0] * matrix.m[1][2] * matrix.m[3][3]
					 - matrix.m[0][2] * matrix.m[1][3] * matrix.m[3][0]
					 - matrix.m[0][3] * matrix.m[1][0] * matrix.m[3][2]
					 + matrix.m[0][3] * matrix.m[1][2] * matrix.m[3][0]
					 + matrix.m[0][2] * matrix.m[1][0] * matrix.m[3][3]
					 + matrix.m[0][0] * matrix.m[1][3] * matrix.m[3][2]) / determinant,/// 23

					(+matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][3]
					 + matrix.m[0][2] * matrix.m[1][3] * matrix.m[2][0]
					 + matrix.m[0][3] * matrix.m[1][0] * matrix.m[2][2]
					 - matrix.m[0][3] * matrix.m[1][2] * matrix.m[2][0]
					 - matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][3]
					 - matrix.m[0][0] * matrix.m[1][3] * matrix.m[2][2]) / determinant,/// 24

					(+matrix.m[1][0] * matrix.m[2][1] * matrix.m[3][3]
					 + matrix.m[1][1] * matrix.m[2][3] * matrix.m[3][0]
					 + matrix.m[1][3] * matrix.m[2][0] * matrix.m[3][1]
					 - matrix.m[1][3] * matrix.m[2][1] * matrix.m[3][0]
					 - matrix.m[1][1] * matrix.m[2][0] * matrix.m[3][3]
					 - matrix.m[1][0] * matrix.m[2][3] * matrix.m[3][1]) / determinant,/// 31

					(-matrix.m[0][0] * matrix.m[2][1] * matrix.m[3][3]
					 - matrix.m[0][1] * matrix.m[2][3] * matrix.m[3][0]
					 - matrix.m[0][3] * matrix.m[2][0] * matrix.m[3][1]
					 + matrix.m[0][3] * matrix.m[2][1] * matrix.m[3][0]
					 + matrix.m[0][1] * matrix.m[2][0] * matrix.m[3][3]
					 + matrix.m[0][0] * matrix.m[2][3] * matrix.m[3][1]) / determinant,/// 32

					(+matrix.m[0][0] * matrix.m[1][1] * matrix.m[3][3]
					 + matrix.m[0][1] * matrix.m[1][3] * matrix.m[3][0]
					 + matrix.m[0][3] * matrix.m[1][0] * matrix.m[3][1]
					 - matrix.m[0][3] * matrix.m[1][1] * matrix.m[3][0]
					 - matrix.m[0][1] * matrix.m[1][0] * matrix.m[3][3]
					 - matrix.m[0][0] * matrix.m[1][3] * matrix.m[3][1]) / determinant,/// 33

					(-matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][3]
					 - matrix.m[0][1] * matrix.m[1][3] * matrix.m[2][0]
					 - matrix.m[0][3] * matrix.m[1][0] * matrix.m[2][1]
					 + matrix.m[0][3] * matrix.m[1][1] * matrix.m[2][0]
					 + matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][3]
					 + matrix.m[0][0] * matrix.m[1][3] * matrix.m[2][1]) / determinant,/// 34

					(-matrix.m[1][0] * matrix.m[2][1] * matrix.m[3][2]
					 - matrix.m[1][1] * matrix.m[2][2] * matrix.m[3][0]
					 - matrix.m[1][2] * matrix.m[2][0] * matrix.m[3][1]
					 + matrix.m[1][2] * matrix.m[2][1] * matrix.m[3][0]
					 + matrix.m[1][1] * matrix.m[2][0] * matrix.m[3][2]
					 + matrix.m[1][0] * matrix.m[2][2] * matrix.m[3][1]) / determinant,/// 41

					(+matrix.m[0][0] * matrix.m[2][1] * matrix.m[3][2]
					 + matrix.m[0][1] * matrix.m[2][2] * matrix.m[3][0]
					 + matrix.m[0][2] * matrix.m[2][0] * matrix.m[3][1]
					 - matrix.m[0][2] * matrix.m[2][1] * matrix.m[3][0]
					 - matrix.m[0][1] * matrix.m[2][0] * matrix.m[3][2]
					 - matrix.m[0][0] * matrix.m[2][2] * matrix.m[3][1]) / determinant,/// 42

					(-matrix.m[0][0] * matrix.m[1][1] * matrix.m[3][2]
					 - matrix.m[0][1] * matrix.m[1][2] * matrix.m[3][0]
					 - matrix.m[0][2] * matrix.m[1][0] * matrix.m[3][1]
					 + matrix.m[0][2] * matrix.m[1][1] * matrix.m[3][0]
					 + matrix.m[0][1] * matrix.m[1][0] * matrix.m[3][2]
					 + matrix.m[0][0] * matrix.m[1][2] * matrix.m[3][1]) / determinant,/// 43

					(+matrix.m[0][0] * matrix.m[1][1] * matrix.m[2][2]
					 + matrix.m[0][1] * matrix.m[1][2] * matrix.m[2][0]
					 + matrix.m[0][2] * matrix.m[1][0] * matrix.m[2][1]
					 - matrix.m[0][2] * matrix.m[1][1] * matrix.m[2][0]
					 - matrix.m[0][1] * matrix.m[1][0] * matrix.m[2][2]
					 - matrix.m[0][0] * matrix.m[1][2] * matrix.m[2][1]) / determinant /// 44
	};
}

Matrix4x4 Transpose(Matrix4x4 tranpose) {
	Matrix4x4 result = {};
	Matrix4x4 a = tranpose;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result.m[i][j] = a.m[j][i];
		}
	}
	return result;
}



Matrix4x4 MakeScaleMatrix4x4(const Vector3 scole) {
	return Matrix4x4{ scole.x,    0.0f,    0.0f, 0.0f,
						 0.0f, scole.y,    0.0f, 0.0f,
						 0.0f,    0.0f, scole.z, 0.0f,
						 0.0f,    0.0f,	   0.0f, 1.0f };
}

Matrix4x4 MakeRotateMatrix4x4(const Vector3 rotate) {
	Matrix4x4 rX = { 1.0f,		   0.0f,		   0.0f, 0.0f,
							   0.0f, cosf(rotate.x), sinf(rotate.x), 0.0f,
							   0.0f,-sinf(rotate.x), cosf(rotate.x), 0.0f,
							   0.0f,	       0.0f,		   0.0f, 1.0f };

	Matrix4x4 rY = { cosf(rotate.y),		   0.0f,-sinf(rotate.y), 0.0f,
							   0.0f,		   1.0f,		   0.0f, 0.0f,
					 sinf(rotate.y),		   0.0f, cosf(rotate.y), 0.0f,
							   0.0f,		   0.0f,		   0.0f, 1.0f };

	Matrix4x4 rZ = { cosf(rotate.z), sinf(rotate.z),		   0.0f, 0.0f,
					-sinf(rotate.z), cosf(rotate.z),		   0.0f, 0.0f,
							   0.0f, 	       0.0f,		   1.0f, 0.0f,
							   0.0f, 	       0.0f,		   0.0f, 1.0f };

	return Matrix4x4{ rX * (rY * rZ) };
}

Matrix4x4 MakeTranslateMatrix(const Vector3 translate) {
	return Matrix4x4{ 1.0f,		  0.0f,		   0.0f, 0.0f,
							 0.0f,		  1.0f,		   0.0f, 0.0f,
							 0.0f,		  0.0f,		   1.0f, 0.0f,
					  translate.x, translate.y,	translate.z, 1.0f };
}



Vector3 ExtractScale(const Matrix4x4 matrix4x4) {
	return Vector3{ matrix4x4.m[0][0], matrix4x4.m[1][1], matrix4x4.m[2][2] };
}

Vector3 ExtractRotate(const Matrix4x4 matrix4x4) {
	Vector3 rot;
	rot.y = (float)asin(-matrix4x4.m[0][2]);                  // Y (pitch)
	rot.x = (float)atan2(matrix4x4.m[1][2], matrix4x4.m[2][2]);         // X (yaw)
	rot.z = (float)atan2(matrix4x4.m[0][1], matrix4x4.m[0][0]);         // Z (roll)
	return rot;

}

Vector3 ExtractTranslate(const Matrix4x4 matrix4x4) {
	return Vector3{ matrix4x4.m[3][0], matrix4x4.m[3][1], matrix4x4.m[3][2] };
}


#pragma endregion

#pragma region カメラ改変陣列
Matrix4x4 MakeAffineMatrix(Vector3 scale, Vector3 rotate, Vector3 translate) {
	Matrix4x4 s = MakeScaleMatrix4x4(scale);

	Matrix4x4 r = MakeRotateMatrix4x4(rotate);

	Matrix4x4 t = MakeTranslateMatrix(translate);

	return Matrix4x4{ s * r * t };
}

Matrix4x4 MatrixMix(Matrix4x4* matrix1, Matrix4x4* matrix2, Matrix4x4* matrix3, Matrix4x4* matrix4) {
	if (matrix4 == nullptr) { return Matrix4x4{ *matrix1 * *matrix2 * *matrix3 }; } else if (matrix3 == nullptr) { return Matrix4x4{ *matrix1 * *matrix2 }; } else { return Matrix4x4{ *matrix1 * *matrix2 * *matrix3 * *matrix4 }; }
}

/////////////////////////////////////////////////////////////////

Matrix4x4 MakeWorldMatrix(Matrix4x4 origin) {
	return origin;
}

Matrix4x4 MakeViewMatrix(Vector3 scale, Vector3 rotate, Vector3 translate) {
	Matrix4x4 world = MakeAffineMatrix(scale, rotate, translate);
	return Inverse(world);
}

Matrix4x4 MakeProjectionMatrix(float left, float right, float top, float bottom, float nearZ, float farZ) {
	return {
				   2.0f / (right - left),							0.0f,					 0.0f, 0.0f,
									0.0f,		   2.0f / (top - bottom),					 0.0f, 0.0f,
									0.0f,							0.0f,   1.0f / (farZ - nearZ), 0.0f,
		 (left + right) / (left - right),(top + bottom) / (bottom - top),  nearZ / (nearZ - farZ), 1.0f
	};
}

Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspect, float nearZ, float farZ) {

	float yScale = 1.0f / tanf(fovY * 0.5f);
	float xScale = yScale / aspect;

	return {
		xScale, 0.0f,   0.0f,   0.0f,
		0.0f,   yScale, 0.0f,   0.0f,
		0.0f,   0.0f,   farZ / (farZ - nearZ), 1.0f,
		0.0f,   0.0f,  -(nearZ * farZ) / (farZ - nearZ), 0.0f
	};
}

Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minD, float maxD) {
	return { width / 2.0f,     			   0.0f,       0.0f,0.0f,
								 0.0f,     -(height / 2.0f),       0.0f,0.0f,
								 0.0f,     		       0.0f,maxD - minD,0.0f,
				left + (width / 2.0f),top + (height / 2.0f),       minD,1.0f };
}

Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearZ, float farZ) {

	return {
				  2.0f / (right - left),						   0.0f,	 				  0.0f,                 0.0f,
								   0.0f,	      2.0f / (top - bottom),	 				  0.0f,                 0.0f,
								   0.0f,						   0.0f,	 1.0f / (farZ - nearZ),					0.0f,
		(left + right) / (left - right),(top + bottom) / (bottom - top),	nearZ / (nearZ - farZ),					1.0f
	};
}

//Vector3 viewFinalTransform(Vector3 obj, Matrix4x4 transformMatrix) {
//	Matrix4x4 objMatrix = MakeTranslateMatrix(obj) * transformMatrix;
//	if (objMatrix.m[3][3] != 0) {
//		objMatrix.m[3][0] /= objMatrix.m[3][3];
//		objMatrix.m[3][1] /= objMatrix.m[3][3];
//		objMatrix.m[3][2] /= objMatrix.m[3][3];
//		objMatrix.m[3][3] = 1.0f;
//	} else {
//		//assert(0);
//	}
//	return MakeTranslateVector3(objMatrix);
//}

Vector3 viewFinalTransform(Vector3 obj, Matrix4x4 m) {

	float x = obj.x * m.m[0][0] + obj.y * m.m[1][0] + obj.z * m.m[2][0] + 1.0f * m.m[3][0];
	float y = obj.x * m.m[0][1] + obj.y * m.m[1][1] + obj.z * m.m[2][1] + 1.0f * m.m[3][1];
	float z = obj.x * m.m[0][2] + obj.y * m.m[1][2] + obj.z * m.m[2][2] + 1.0f * m.m[3][2];
	float w = obj.x * m.m[0][3] + obj.y * m.m[1][3] + obj.z * m.m[2][3] + 1.0f * m.m[3][3];

	if (w != 0.0f) {
		x /= w;
		y /= w;
		z /= w;
	}

	return { x, y, z };
}

Vector3 TransformDirection(Vector3 dir, const Matrix4x4& m) {

	float x = dir.x * m.m[0][0] + dir.y * m.m[1][0] + dir.z * m.m[2][0];
	float y = dir.x * m.m[0][1] + dir.y * m.m[1][1] + dir.z * m.m[2][1];
	float z = dir.x * m.m[0][2] + dir.y * m.m[1][2] + dir.z * m.m[2][2];

	return { x, y, z };
}

#pragma endregion
