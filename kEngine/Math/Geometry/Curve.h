#pragma once
#include "Geometry/Interpolation.h"
#include "Vector2.h"
#include "Vector3.h"

/// <summary>
/// 2次元2次ベジェ曲線の点計算
/// </summary>
/// <param name="p0">制御点0</param>
/// <param name="p1">制御点1</param>
/// <param name="p2">制御点2</param>
/// <param name="p3">制御点2</param>
/// <param name="time">補間用変数t</param>
/// <param name="clock">総分割数</param>
/// <returns>曲線上にtの対応点</returns>
Vector2 Bezier(const Vector2 p0, const Vector2 p1, const Vector2 p2, float time, float clock);

/// <summary>
/// 3次元2次ベジェ曲線の点計算
/// </summary>
/// <param name="p0">制御点0</param>
/// <param name="p1">制御点1</param>
/// <param name="p2">制御点2</param>
/// <param name="p3">制御点2</param>
/// <param name="time">補間用変数t</param>
/// <param name="clock">総分割数</param>
/// <returns>曲線上にtの対応点</returns>
Vector3 Bezier(const Vector3 p0, const Vector3 p1, const Vector3 p2, float time, float clock);


