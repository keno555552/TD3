#include "Vector2.h"

Vector2 Vector2::operator+(const Vector2& target)
{
	return { x + target.x, y + target.y };
}

Vector2 Vector2::operator-(const Vector2& target)
{
	return { x - target.x, y - target.y };
}

Vector2 Vector2::operator*(const Vector2& target)
{
	return { x * target.x, y * target.y };
}

Vector2 Vector2::operator/(const Vector2& target)
{
	return { x / target.x, y / target.y };
}

Vector2 Vector2::operator*(const float& target)
{
	return { x * target, y * target };
}

Vector2 operator*(float scalar, const Vector2& vec)
{
	return { vec.x * scalar, vec.y * scalar };
}

Vector2 Vector2::operator/(const float& target)
{
	return { x / target, y / target };
}

Vector2 Vector2::operator*(const int& target)
{
	return { x * (float)target, y * (float)target };
}

bool Vector2::operator==(const Vector2& target) {
	if (x != target.x)return false;
	if (y != target.y)return false;
	return true;
}

bool Vector2::operator!=(const Vector2& target) {
	return !(*this == target);		
}

Vector2 operator*(int scalar, const Vector2& vec)
{
	return { vec.x * (float)scalar, vec.y * (float)scalar };
}
