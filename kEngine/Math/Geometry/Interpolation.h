#pragma once
#include "MathsIncluder.h"

/// <summary>
/// 線形補間
/// </summary>
/// <param name="a">A point Vector2構造体</param>
/// <param name="b">C point Vector2構造体</param>
/// <param name="c">補間用変数t</param>
/// <param name="t">総分割数</param>
/// <returns></returns>
float Lerp(float a, float b, float c, float t);

//void Draw3DLine(const Line& line, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, int color);

//void DrawGrid(const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix);


Vector3 CubicSpline(
    const Vector3& v0, const Vector3& outTan0,
    const Vector3& v1, const Vector3& inTan1,
    float t, float dt
);

Quaternion CubicSplineQuat(
    const Quaternion& v0, const Quaternion& outTan0,
    const Quaternion& v1, const Quaternion& inTan1,
    float t, float dt
);