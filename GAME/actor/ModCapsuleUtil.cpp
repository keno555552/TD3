#include "ModCapsuleUtil.h"
#include "Data/Render/CPUData/VertexData.h"
#include <cmath>
#include <algorithm>

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
  Vector3 outward = NormalizeSafe(direction, fallbackNormal);

  // 方向だけから表面点を取りたい場合は、
  // 軸上の「中央」を基準にするのが一番安定する
  const Vector3 axisMid = Multiply(0.5f, Add(capsule.start, capsule.end));

  return Add(axisMid, Multiply(capsule.radius, outward));
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

static Vector3 TransformPointByMatrixLocal(const Matrix4x4 &m, const Vector3 &p) {
  Vector3 result{};
  result.x = p.x * m.m[0][0] + p.y * m.m[1][0] + p.z * m.m[2][0] + m.m[3][0];
  result.y = p.x * m.m[0][1] + p.y * m.m[1][1] + p.z * m.m[2][1] + m.m[3][1];
  result.z = p.x * m.m[0][2] + p.y * m.m[1][2] + p.z * m.m[2][2] + m.m[3][2];
  return result;
}

ModCapsule CalculateMinimalCapsuleFromVertices(const std::vector<VertexData>& vertices, const Matrix4x4& localMatrix) {
  ModCapsule capsule{};
  if (vertices.empty()) {
    return capsule;
  }

  float minX = FLT_MAX;
  float maxX = -FLT_MAX;
  float minY = FLT_MAX;
  float maxY = -FLT_MAX;
  float minZ = FLT_MAX;
  float maxZ = -FLT_MAX;

  for (size_t i = 0; i < vertices.size(); ++i) {
    const Vector4& pos4 = vertices[i].position;
    Vector3 localPos = {pos4.x, pos4.y, pos4.z};
    Vector3 transformed = TransformPointByMatrixLocal(localMatrix, localPos);

    if (transformed.x < minX) minX = transformed.x;
    if (transformed.x > maxX) maxX = transformed.x;

    if (transformed.y < minY) minY = transformed.y;
    if (transformed.y > maxY) maxY = transformed.y;

    if (transformed.z < minZ) minZ = transformed.z;
    if (transformed.z > maxZ) maxZ = transformed.z;
  }

  // もし頂点が存在しなかった場合のフォールバック（FLT_MAXのままなら）
  if (minY > maxY) {
      minX = 0.0f; maxX = 0.0f;
      minY = 0.0f; maxY = 0.0f;
      minZ = 0.0f; maxZ = 0.0f;
  }

  // Yの幅が0の場合の安全対策
  if (maxY - minY < 0.0001f) {
    maxY = minY + 0.0001f;
  }

  const float halfWidthX = (maxX - minX) * 0.5f;
  const float halfWidthZ = (maxZ - minZ) * 0.5f;
  capsule.radius = (std::max)(halfWidthX, halfWidthZ);
  capsule.radiusX = halfWidthX;
  capsule.radiusZ = halfWidthZ;

  const float midX = (minX + maxX) * 0.5f;
  const float midZ = (minZ + maxZ) * 0.5f;

  // 3. 実際のカプセルは「半球」が出っ張るので、startとendはradius分だけ内側に寄せる
  capsule.start = {midX, minY + capsule.radius, midZ};
  capsule.end = {midX, maxY - capsule.radius, midZ};

  // もし長さが短すぎて radius * 2 より小さい場合は、球に近い形にする
  if (capsule.start.y > capsule.end.y) {
    float midY = (minY + maxY) * 0.5f;
    capsule.start = {midX, midY, midZ};
    capsule.end = {midX, midY, midZ};
    // 半径をY方向の長さで補正（Y軸方向もカバーできるように）
    float halfHeight = (maxY - minY) * 0.5f;
    if (halfHeight > capsule.radius) {
      capsule.radius = halfHeight;
    }
  }

  return capsule;
}

} // namespace ModCapsuleUtil