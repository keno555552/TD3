#pragma once
#include "Vector3.h"

/// <summary>
/// Vector3の垂直点を返す
/// </summary>
/// <param name="vector">元のVector3</param>
/// <returns></returns>
Vector3 Perpendicular(const Vector3& vector);

/// <summary>
/// Vector3 反射計算
/// </summary>
/// <param name="input"></param>
/// <param name="normal"></param>
/// <returns></returns>
Vector3 ReflectVector3(const Vector3& input, const Vector3& normal);
