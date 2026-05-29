#pragma once
#include "Data/Geometry/Shape/Line/Segment.h"
#include "Vector3.h"

/// <summary>
/// 正射影ベクトル
/// </summary>
/// <param name="v1">目標の線</param>
/// <param name="v2">点</param>
/// <returns></returns>
Vector3 Project(const Vector3& v1, const Vector3& v2);

/// 最接近点
Vector3 ClosestPoint(const Vector3& point, const Segment& segment);
