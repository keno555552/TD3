#pragma once

typedef struct Vector2 {

	///定義
	float x, y;

	///演算子再定義
	Vector2 operator+(const Vector2& target);
	Vector2 operator-(const Vector2& target);
	Vector2 operator*(const Vector2& target);
	Vector2 operator/(const Vector2& target);
	Vector2 operator*(const float& target);
	friend Vector2 operator*(float scalar, const Vector2& vec);
	Vector2 operator/(const float& target);

	///int
	Vector2 operator*(const int& target);
	friend Vector2 operator*(int scalar, const Vector2& vec);

	bool operator==(const Vector2& target);
	bool operator!=(const Vector2& target);
}Vector2;


