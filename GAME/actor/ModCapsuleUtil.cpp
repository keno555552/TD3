#include "ModCapsuleUtil.h"
#include <cmath>

namespace {

float ClampFloatLocal(float value, float minValue, float maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

} // namespace

namespace ModCapsuleUtil {

Vector3 NormalizeSafe(const Vector3 &v, const Vector3 &fallback) {
  const float len = Length(v);
  if (len < 0.0001f) {
    return fallback;
  }

  const float inv = 1.0f / len;
  return {v.x * inv, v.y * inv, v.z * inv};
}

float Clamp01(float t) { return ClampFloatLocal(t, 0.0f, 1.0f); }

Vector3 ClosestPointOnSegment(const Vector3 &point, const Vector3 &a,
                              const Vector3 &b) {
  const Vector3 ab = Subtract(b, a);
  const float abLenSq = Dot(ab, ab);
  if (abLenSq <= 0.000001f) {
    return a;
  }

  const Vector3 ap = Subtract(point, a);
  float t = Dot(ap, ab) / abLenSq;
  t = Clamp01(t);

  return Add(a, Multiply(t, ab));
}

ModCapsuleClosestPoints ClosestPointsBetweenSegments(const Vector3 &a0,
                                                     const Vector3 &a1,
                                                     const Vector3 &b0,
                                                     const Vector3 &b1) {
  ModCapsuleClosestPoints result{};

  const Vector3 d1 = Subtract(a1, a0);
  const Vector3 d2 = Subtract(b1, b0);
  const Vector3 r = Subtract(a0, b0);

  const float a = Dot(d1, d1);
  const float e = Dot(d2, d2);
  const float f = Dot(d2, r);

  float s = 0.0f;
  float t = 0.0f;

  if (a <= 0.000001f && e <= 0.000001f) {
    result.pointOnA = a0;
    result.pointOnB = b0;
    const Vector3 diff = Subtract(result.pointOnA, result.pointOnB);
    result.distanceSq = Dot(diff, diff);
    return result;
  }

  if (a <= 0.000001f) {
    s = 0.0f;
    t = Clamp01(f / e);
  } else {
    const float c = Dot(d1, r);

    if (e <= 0.000001f) {
      t = 0.0f;
      s = Clamp01(-c / a);
    } else {
      const float b = Dot(d1, d2);
      const float denom = a * e - b * b;

      if (fabsf(denom) > 0.000001f) {
        s = Clamp01((b * f - c * e) / denom);
      } else {
        s = 0.0f;
      }

      t = (b * s + f) / e;

      if (t < 0.0f) {
        t = 0.0f;
        s = Clamp01(-c / a);
      } else if (t > 1.0f) {
        t = 1.0f;
        s = Clamp01((b - c) / a);
      }
    }
  }

  result.pointOnA = Add(a0, Multiply(s, d1));
  result.pointOnB = Add(b0, Multiply(t, d2));

  const Vector3 diff = Subtract(result.pointOnA, result.pointOnB);
  result.distanceSq = Dot(diff, diff);
  return result;
}

ModCapsuleClosestPoints ClosestPointsBetweenCapsules(const ModCapsule &a,
                                                     const ModCapsule &b) {
  return ClosestPointsBetweenSegments(a.start, a.end, b.start, b.end);
}

bool IntersectCapsules(const ModCapsule &a, const ModCapsule &b,
                       float extraMargin) {
  const ModCapsuleClosestPoints closest = ClosestPointsBetweenCapsules(a, b);
  const float radiusSum = a.radius + b.radius + extraMargin;
  return closest.distanceSq <= radiusSum * radiusSum;
}

float SignedDistanceCapsuleToCapsule(const ModCapsule &a, const ModCapsule &b) {
  const ModCapsuleClosestPoints closest = ClosestPointsBetweenCapsules(a, b);
  const float centerDistance = sqrtf(closest.distanceSq);
  return centerDistance - (a.radius + b.radius);
}

Vector3 GetSurfacePointTowardDirection(const ModCapsule &capsule,
                                       const Vector3 &direction,
                                       const Vector3 &fallbackNormal) {
  const Vector3 axis = Subtract(capsule.end, capsule.start);
  const float axisLenSq = Dot(axis, axis);

  Vector3 dir = NormalizeSafe(direction, fallbackNormal);

  Vector3 axisPoint = capsule.start;
  if (axisLenSq > 0.000001f) {
    const float t =
        Clamp01(Dot(Subtract(dir, {0.0f, 0.0f, 0.0f}), axis) / axisLenSq);
    axisPoint = Add(capsule.start, Multiply(t, axis));
  }

  return Add(axisPoint, Multiply(capsule.radius, dir));
}

Vector3 GetSurfacePointTowardPosition(const ModCapsule &capsule,
                                      const Vector3 &targetPosition,
                                      const Vector3 &preferredNormal) {
  const Vector3 axisPoint =
      ClosestPointOnSegment(targetPosition, capsule.start, capsule.end);

  Vector3 outward = Subtract(targetPosition, axisPoint);
  outward = NormalizeSafe(outward, preferredNormal);

  return Add(axisPoint, Multiply(capsule.radius, outward));
}

Vector3 GetAttachSideNormal(ModCapsuleAttachSide side) {
  switch (side) {
  case ModCapsuleAttachSide::PosX:
    return {1.0f, 0.0f, 0.0f};
  case ModCapsuleAttachSide::NegX:
    return {-1.0f, 0.0f, 0.0f};
  case ModCapsuleAttachSide::PosY:
    return {0.0f, 1.0f, 0.0f};
  case ModCapsuleAttachSide::NegY:
    return {0.0f, -1.0f, 0.0f};
  case ModCapsuleAttachSide::PosZ:
    return {0.0f, 0.0f, 1.0f};
  case ModCapsuleAttachSide::NegZ:
    return {0.0f, 0.0f, -1.0f};
  default:
    return {0.0f, 1.0f, 0.0f};
  }
}

} // namespace ModCapsuleUtil