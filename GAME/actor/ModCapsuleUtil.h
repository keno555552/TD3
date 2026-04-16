#pragma once
#include "kEngine/Math/Vector3.h"

enum class ModCapsuleAttachSide {
  PosX = 0,
  NegX,
  PosY,
  NegY,
  PosZ,
  NegZ,
};

struct ModCapsule {
  Vector3 start{0.0f, 0.0f, 0.0f};
  Vector3 end{0.0f, 0.0f, 0.0f};
  float radius = 0.0f;
};

struct ModCapsuleClosestPoints {
  Vector3 pointOnA{0.0f, 0.0f, 0.0f};
  Vector3 pointOnB{0.0f, 0.0f, 0.0f};
  float distanceSq = 0.0f;
};

namespace ModCapsuleUtil {

/// 軸方向を返す。長さが短すぎる場合は fallback を返す
Vector3 NormalizeSafe(const Vector3 &v, const Vector3 &fallback);

/// 0.0f ～ 1.0f に clamp する
float Clamp01(float t);

/// 線分上の最近接点を返す
Vector3 ClosestPointOnSegment(const Vector3 &point, const Vector3 &a,
                              const Vector3 &b);

/// 2線分間の最近接点ペアを返す
ModCapsuleClosestPoints ClosestPointsBetweenSegments(const Vector3 &a0,
                                                     const Vector3 &a1,
                                                     const Vector3 &b0,
                                                     const Vector3 &b1);

/// カプセル同士の最近接情報を返す
ModCapsuleClosestPoints ClosestPointsBetweenCapsules(const ModCapsule &a,
                                                     const ModCapsule &b);

/// カプセル同士が重なっているか
bool IntersectCapsules(const ModCapsule &a, const ModCapsule &b,
                       float extraMargin = 0.0f);

/// 2カプセル表面間の符号付き距離
/// 負ならめり込み
float SignedDistanceCapsuleToCapsule(const ModCapsule &a, const ModCapsule &b);

/// 指定方向側のカプセル表面点を返す
/// direction がゼロに近い場合は fallbackNormal を使う
Vector3 GetSurfacePointTowardDirection(const ModCapsule &capsule,
                                       const Vector3 &direction,
                                       const Vector3 &fallbackNormal);

/// targetPosition に一番近い軸位置を基準に、preferredNormal 側の表面点を返す
Vector3 GetSurfacePointTowardPosition(const ModCapsule &capsule,
                                      const Vector3 &targetPosition,
                                      const Vector3 &preferredNormal);

/// 接続面 enum からローカル法線相当の方向を返す
Vector3 GetAttachSideNormal(ModCapsuleAttachSide side);

} // namespace ModCapsuleUtil