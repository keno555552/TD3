#include "ModScene.h"
#include "GAME/actor/ModAssemblyResolver.h"
#include "GAME/actor/ModAssemblyUtil.h"
#include "GAME/actor/ModObjectUtil.h"
#include "GAME/actor/prompt/PromptData.h"
#include "Math/Geometry/Collision/crashDecision.h"
#include <Windows.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <random>
#include <unordered_set>

namespace {

size_t ToIndex(ModBodyPart part) { return static_cast<size_t>(part); }

Vector3 Mul(const Vector3 &a, const Vector3 &b) {
  return {a.x * b.x, a.y * b.y, a.z * b.z};
}

Vector3 NormalizeSafeV(const Vector3 &v, const Vector3 &fallback) {
  const float len = Length(v);
  if (len < 0.0001f) {
    return fallback;
  }

  const float inv = 1.0f / len;
  return {v.x * inv, v.y * inv, v.z * inv};
}

float Max3(float a, float b, float c) {
  return (std::max)(a, (std::max)(b, c));
}

Vector4 MakeColor(float r, float g, float b, float a = 1.0f) {
  return {r, g, b, a};
}

Vector3 ZeroV() { return {0.0f, 0.0f, 0.0f}; }

int FindRoleIndexInModPoints(const std::vector<ModControlPoint> &points,
                             ModControlPointRole role) {
  for (size_t i = 0; i < points.size(); ++i) {
    if (points[i].role == role) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

const char *PartName(ModBodyPart part) {
  switch (part) {
  case ModBodyPart::ChestBody:
    return "ChestBody";
  case ModBodyPart::StomachBody:
    return "StomachBody";
  case ModBodyPart::Neck:
    return "Neck";
  case ModBodyPart::Head:
    return "Head";
  case ModBodyPart::LeftUpperArm:
    return "LeftUpperArm";
  case ModBodyPart::LeftForeArm:
    return "LeftForeArm";
  case ModBodyPart::RightUpperArm:
    return "RightUpperArm";
  case ModBodyPart::RightForeArm:
    return "RightForeArm";
  case ModBodyPart::LeftThigh:
    return "LeftThigh";
  case ModBodyPart::LeftShin:
    return "LeftShin";
  case ModBodyPart::RightThigh:
    return "RightThigh";
  case ModBodyPart::RightShin:
    return "RightShin";
  default:
    return "Unknown";
  }
}

std::string ModelPath(ModBodyPart part) {
  switch (part) {
  case ModBodyPart::ChestBody:
    return "GAME/resources/modBody/chest/chest.obj";

  case ModBodyPart::StomachBody:
    return "GAME/resources/modBody/stomach/stomach.obj";

  case ModBodyPart::Neck:
    return "GAME/resources/modBody/neck/neck.obj";

  case ModBodyPart::Head:
    return "GAME/resources/modBody/head/head.obj";

  case ModBodyPart::LeftUpperArm:
    return "GAME/resources/modBody/leftUpperArm/leftUpperArm.obj";
  case ModBodyPart::LeftForeArm:
    return "GAME/resources/modBody/leftForeArm/leftForeArm.obj";
  case ModBodyPart::RightUpperArm:
    return "GAME/resources/modBody/rightUpperArm/rightUpperArm.obj";
  case ModBodyPart::RightForeArm:
    return "GAME/resources/modBody/rightForeArm/rightForeArm.obj";
  case ModBodyPart::LeftThigh:
    return "GAME/resources/modBody/leftThighs/leftThighs.obj";
  case ModBodyPart::LeftShin:
    return "GAME/resources/modBody/leftShin/leftShin.obj";
  case ModBodyPart::RightThigh:
    return "GAME/resources/modBody/rightThighs/rightThighs.obj";
  case ModBodyPart::RightShin:
    return "GAME/resources/modBody/rightShin/rightShin.obj";
  default:
    return "GAME/resources/modBody/chest/chest.obj";
  }
}

int ResolveControlOwnerPartId(const ModAssemblyGraph &assembly, int partId) {
  const PartNode *node = assembly.FindNode(partId);
  if (node == nullptr) {
    return -1;
  }

  switch (node->part) {
  case ModBodyPart::Head:
    if (node->parentId >= 0) {
      const PartNode *parent = assembly.FindNode(node->parentId);
      if (parent != nullptr && parent->part == ModBodyPart::Neck) {
        return parent->id;
      }
    }
    break;

  case ModBodyPart::LeftForeArm:
    if (node->parentId >= 0) {
      const PartNode *parent = assembly.FindNode(node->parentId);
      if (parent != nullptr && parent->part == ModBodyPart::LeftUpperArm) {
        return parent->id;
      }
    }
    break;

  case ModBodyPart::RightForeArm:
    if (node->parentId >= 0) {
      const PartNode *parent = assembly.FindNode(node->parentId);
      if (parent != nullptr && parent->part == ModBodyPart::RightUpperArm) {
        return parent->id;
      }
    }
    break;

  case ModBodyPart::LeftShin:
    if (node->parentId >= 0) {
      const PartNode *parent = assembly.FindNode(node->parentId);
      if (parent != nullptr && parent->part == ModBodyPart::LeftThigh) {
        return parent->id;
      }
    }
    break;

  case ModBodyPart::RightShin:
    if (node->parentId >= 0) {
      const PartNode *parent = assembly.FindNode(node->parentId);
      if (parent != nullptr && parent->part == ModBodyPart::RightThigh) {
        return parent->id;
      }
    }
    break;

  default:
    break;
  }

  return partId;
}

bool IsSelectablePart(ModBodyPart part) { return part != ModBodyPart::Count; }

bool IsMouseLeftPressed() {
  return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
}

bool IsMouseRightPressed() {
  return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
}

bool IsMouseLeftTriggered() {
  static bool wasPressed = false;
  const bool nowPressed = IsMouseLeftPressed();
  const bool triggered = (!wasPressed && nowPressed);
  wasPressed = nowPressed;
  return triggered;
}

bool IsMouseLeftReleased() {
  static bool wasPressed = false;
  const bool nowPressed = IsMouseLeftPressed();
  const bool released = (wasPressed && !nowPressed);
  wasPressed = nowPressed;
  return released;
}
bool RayPlaneIntersectionZ(const Ray &ray, float planeZ, Vector3 *hitPoint) {
  const float epsilon = 0.0001f;

  if (fabsf(ray.direction.z) < epsilon) {
    return false;
  }

  const float t = (planeZ - ray.origin.z) / ray.direction.z;
  if (t < 0.0f) {
    return false;
  }

  if (hitPoint != nullptr) {
    hitPoint->x = ray.origin.x + ray.direction.x * t;
    hitPoint->y = ray.origin.y + ray.direction.y * t;
    hitPoint->z = ray.origin.z + ray.direction.z * t;
  }

  return true;
}

bool RayPlaneIntersection(const Ray &ray, const Vector3 &planePoint,
                          const Vector3 &planeNormal, Vector3 *hitPoint) {
  const float epsilon = 0.0001f;

  const float denom = Dot(ray.direction, planeNormal);
  if (fabsf(denom) < epsilon) {
    return false;
  }

  const Vector3 diff = Subtract(planePoint, ray.origin);
  const float t = Dot(diff, planeNormal) / denom;
  if (t < 0.0f) {
    return false;
  }

  if (hitPoint != nullptr) {
    *hitPoint = Add(ray.origin, Multiply(t, ray.direction));
  }

  return true;
}

Vector3 ClampDistance(const Vector3 &origin, const Vector3 &target,
                      float minLength, float maxLength,
                      const Vector3 &fallbackDir) {
  Vector3 diff = Subtract(target, origin);
  float length = Length(diff);

  Vector3 dir = fallbackDir;
  if (length > 0.0001f) {
    dir = NormalizeSafeV(diff, fallbackDir);
  }

  if (length < minLength) {
    return Add(origin, Multiply(minLength, dir));
  }
  if (length > maxLength) {
    return Add(origin, Multiply(maxLength, dir));
  }
  return target;
}

float ClampFloatLocal(float value, float minValue, float maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

float GetControlPointGizmoDrawRadius(float influenceRadius) {
  return ClampFloatLocal(influenceRadius * 0.45f, 0.035f, 0.12f);
}

float GetWheelScaleFactorFromDelta(int wheelDelta) {
  if (wheelDelta > 0) {
    return 1.08f;
  }
  if (wheelDelta < 0) {
    return 1.0f / 1.08f;
  }
  return 1.0f;
}

bool GetPickSegmentRoles(ModBodyPart part, ModControlPointRole &startRole,
                         ModControlPointRole &endRole) {
  switch (part) {
  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::RightUpperArm:
  case ModBodyPart::LeftThigh:
  case ModBodyPart::RightThigh:
  case ModBodyPart::Neck:
    startRole = ModControlPointRole::Root;
    endRole = ModControlPointRole::Bend;
    return true;

  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
  case ModBodyPart::Head:
    startRole = ModControlPointRole::Bend;
    endRole = ModControlPointRole::End;
    return true;

  case ModBodyPart::ChestBody:
    startRole = ModControlPointRole::Chest;
    endRole = ModControlPointRole::Belly;
    return true;

  case ModBodyPart::StomachBody:
    startRole = ModControlPointRole::Belly;
    endRole = ModControlPointRole::Waist;
    return true;

  default:
    return false;
  }
}

float DistancePointToSegmentSq(const Vector3 &point, const Vector3 &a,
                               const Vector3 &b) {
  const Vector3 ab = Subtract(b, a);
  const Vector3 ap = Subtract(point, a);

  const float abLenSq = Dot(ab, ab);
  if (abLenSq <= 0.000001f) {
    const Vector3 diff = Subtract(point, a);
    return Dot(diff, diff);
  }

  float t = Dot(ap, ab) / abLenSq;
  t = ClampFloatLocal(t, 0.0f, 1.0f);

  const Vector3 closest = Add(a, Multiply(t, ab));
  const Vector3 diff = Subtract(point, closest);
  return Dot(diff, diff);
}

bool IntersectRaySphereLocal(const Ray &ray, const Vector3 &center,
                             float radius, float *outT) {
  const Vector3 m = Subtract(ray.origin, center);
  const float a = Dot(ray.direction, ray.direction);
  const float b = Dot(m, ray.direction);
  const float c = Dot(m, m) - radius * radius;

  if (c <= 0.0f) {
    if (outT != nullptr) {
      *outT = 0.0f;
    }
    return true;
  }

  if (a <= 0.000001f) {
    return false;
  }

  const float discriminant = b * b - a * c;
  if (discriminant < 0.0f) {
    return false;
  }

  float t = (-b - sqrtf(discriminant)) / a;
  if (t < 0.0f) {
    t = (-b + sqrtf(discriminant)) / a;
    if (t < 0.0f) {
      return false;
    }
  }

  if (outT != nullptr) {
    *outT = t;
  }
  return true;
}

bool IntersectRayCapsule(const Ray &ray, const Vector3 &capsuleStart,
                         const Vector3 &capsuleEnd, float capsuleRadius,
                         float *outT) {
  const float epsilon = 0.0001f;
  const float rayDirLenSq = Dot(ray.direction, ray.direction);
  if (rayDirLenSq <= epsilon) {
    return false;
  }

  const Vector3 d = Subtract(capsuleEnd, capsuleStart);
  const float segLenSq = Dot(d, d);

  if (segLenSq <= epsilon) {
    return IntersectRaySphereLocal(ray, capsuleStart, capsuleRadius, outT);
  }

  const Vector3 m = Subtract(ray.origin, capsuleStart);
  const Vector3 n = ray.direction;

  const float md = Dot(m, d);
  const float nd = Dot(n, d);
  const float dd = segLenSq;
  const float mn = Dot(m, n);
  const float nn = Dot(n, n);

  const float a = dd * nn - nd * nd;
  const float k = Dot(m, m) - capsuleRadius * capsuleRadius;
  const float c = dd * k - md * md;

  float bestT = FLT_MAX;
  bool hit = false;

  if (fabsf(a) > epsilon) {
    const float b = dd * mn - nd * md;
    const float discriminant = b * b - a * c;

    if (discriminant >= 0.0f) {
      const float sqrtDiscriminant = sqrtf(discriminant);

      float t0 = (-b - sqrtDiscriminant) / a;
      float t1 = (-b + sqrtDiscriminant) / a;

      if (t0 > t1) {
        const float temp = t0;
        t0 = t1;
        t1 = temp;
      }

      if (t1 >= 0.0f) {
        if (t0 < 0.0f) {
          t0 = 0.0f;
        }

        const float candidates[2] = {t0, t1};
        for (int i = 0; i < 2; ++i) {
          const float t = candidates[i];
          const float s = md + t * nd;
          if (s >= 0.0f && s <= dd) {
            if (t < bestT) {
              bestT = t;
              hit = true;
            }
          }
        }
      }
    }
  }

  float sphereT = 0.0f;
  if (IntersectRaySphereLocal(ray, capsuleStart, capsuleRadius, &sphereT)) {
    if (sphereT < bestT) {
      bestT = sphereT;
      hit = true;
    }
  }

  if (IntersectRaySphereLocal(ray, capsuleEnd, capsuleRadius, &sphereT)) {
    if (sphereT < bestT) {
      bestT = sphereT;
      hit = true;
    }
  }

  if (!hit) {
    return false;
  }

  if (outT != nullptr) {
    *outT = bestT;
  }
  return true;
}

bool IsDescendantPartIdRecursive(const ModAssemblyGraph &assembly,
                                 int rootPartId, int checkPartId) {
  if (rootPartId == checkPartId) {
    return true;
  }

  const std::vector<int> children = assembly.GetChildren(rootPartId);
  for (size_t i = 0; i < children.size(); ++i) {
    if (IsDescendantPartIdRecursive(assembly, children[i], checkPartId)) {
      return true;
    }
  }
  return false;
}

Vector3 ClosestPointOnSegmentLocal(const Vector3 &point, const Vector3 &a,
                                   const Vector3 &b) {
  const Vector3 ab = Subtract(b, a);
  const float abLenSq = Dot(ab, ab);

  if (abLenSq <= 0.000001f) {
    return a;
  }

  float t = Dot(Subtract(point, a), ab) / abLenSq;
  t = ClampFloatLocal(t, 0.0f, 1.0f);

  return Add(a, Multiply(t, ab));
}

float DistancePointToCapsuleSurfaceSq(const Vector3 &point,
                                      const Vector3 &capsuleStart,
                                      const Vector3 &capsuleEnd,
                                      float capsuleRadius) {
  const float centerDistSq =
      DistancePointToSegmentSq(point, capsuleStart, capsuleEnd);

  const float centerDist = sqrtf((std::max)(0.0f, centerDistSq));
  const float surfaceDist = (std::max)(0.0f, centerDist - capsuleRadius);
  return surfaceDist * surfaceDist;
}

void BuildExpandedSegment(const Vector3 &startPos, const Vector3 &endPos,
                          float startRadius, float endRadius,
                          Vector3 *outExpandedStart, Vector3 *outExpandedEnd) {
  if (outExpandedStart == nullptr || outExpandedEnd == nullptr) {
    return;
  }

  const Vector3 rawSegment = Subtract(endPos, startPos);
  const float rawLength = Length(rawSegment);

  Vector3 segmentDir = {0.0f, -1.0f, 0.0f};
  if (rawLength > 0.0001f) {
    segmentDir = NormalizeSafeV(rawSegment, segmentDir);
  }

  const float safeStartRadius = (std::max)(startRadius, 0.01f);
  const float safeEndRadius = (std::max)(endRadius, 0.01f);

  *outExpandedStart = Subtract(startPos, Multiply(safeStartRadius, segmentDir));
  *outExpandedEnd = Add(endPos, Multiply(safeEndRadius, segmentDir));
}

Vector3 TransformPointByMatrixLocal(const Matrix4x4 &m, const Vector3 &p) {
  Vector3 result{};
  result.x = p.x * m.m[0][0] + p.y * m.m[1][0] + p.z * m.m[2][0] + m.m[3][0];
  result.y = p.x * m.m[0][1] + p.y * m.m[1][1] + p.z * m.m[2][1] + m.m[3][1];
  result.z = p.x * m.m[0][2] + p.y * m.m[1][2] + p.z * m.m[2][2] + m.m[3][2];
  return result;
}

float GetDefaultSegmentRadiusForPartRoles(ModBodyPart part,
                                          ModControlPointRole startRole,
                                          ModControlPointRole endRole) {
  switch (part) {
  case ModBodyPart::ChestBody:
    if (startRole == ModControlPointRole::Chest &&
        endRole == ModControlPointRole::Belly) {
      return (0.12f + 0.10f) * 0.5f;
    }
    break;

  case ModBodyPart::StomachBody:
    if (startRole == ModControlPointRole::Belly &&
        endRole == ModControlPointRole::Waist) {
      return (0.10f + 0.12f) * 0.5f;
    }
    break;

  case ModBodyPart::Neck:
    if (startRole == ModControlPointRole::Root &&
        endRole == ModControlPointRole::Bend) {
      return (0.09f + 0.08f) * 0.5f;
    }
    break;

  case ModBodyPart::Head:
    if (startRole == ModControlPointRole::Bend &&
        endRole == ModControlPointRole::End) {
      return (0.08f + 0.11f) * 0.5f;
    }
    break;

  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::RightUpperArm:
    if (startRole == ModControlPointRole::Root &&
        endRole == ModControlPointRole::Bend) {
      return (0.09f + 0.08f) * 0.5f;
    }
    break;

  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
    if (startRole == ModControlPointRole::Bend &&
        endRole == ModControlPointRole::End) {
      return (0.08f + 0.08f) * 0.5f;
    }
    break;

  case ModBodyPart::LeftThigh:
  case ModBodyPart::RightThigh:
    if (startRole == ModControlPointRole::Root &&
        endRole == ModControlPointRole::Bend) {
      return (0.10f + 0.09f) * 0.5f;
    }
    break;

  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
    if (startRole == ModControlPointRole::Bend &&
        endRole == ModControlPointRole::End) {
      return (0.09f + 0.09f) * 0.5f;
    }
    break;

  default:
    break;
  }

  return 0.10f;
}

Vector3 CrossLocal(const Vector3 &a, const Vector3 &b) {
  return {
      a.y * b.z - a.z * b.y,
      a.z * b.x - a.x * b.z,
      a.x * b.y - a.y * b.x,
  };
}

float AbsDotLocal(const Vector3 &a, const Vector3 &b) {
  const float d = Dot(a, b);
  return (d >= 0.0f) ? d : -d;
}

bool IntersectRaySegmentBox(const Ray &ray, const ModSceneSegmentBox &box,
                            float *outT) {
  const float epsilon = 0.0001f;

  const Vector3 p = Subtract(box.center, ray.origin);

  const Vector3 axes[3] = {box.axisX, box.axisY, box.axisZ};
  const float extents[3] = {box.halfWidth, box.halfLength, box.halfDepth};

  float tMin = -FLT_MAX;
  float tMax = FLT_MAX;

  for (int i = 0; i < 3; ++i) {
    const float e = Dot(axes[i], p);
    const float f = Dot(axes[i], ray.direction);

    if (fabsf(f) > epsilon) {
      float t1 = (e + extents[i]) / f;
      float t2 = (e - extents[i]) / f;

      if (t1 > t2) {
        const float tmp = t1;
        t1 = t2;
        t2 = tmp;
      }

      if (t1 > tMin) {
        tMin = t1;
      }
      if (t2 < tMax) {
        tMax = t2;
      }

      if (tMin > tMax) {
        return false;
      }
      if (tMax < 0.0f) {
        return false;
      }
    } else {
      if (-e - extents[i] > 0.0f || -e + extents[i] < 0.0f) {
        return false;
      }
    }
  }

  float hitT = tMin;
  if (hitT < 0.0f) {
    hitT = tMax;
  }
  if (hitT < 0.0f) {
    return false;
  }

  if (outT != nullptr) {
    *outT = hitT;
  }
  return true;
}

bool IntersectSegmentBoxes(const ModSceneSegmentBox &a,
                           const ModSceneSegmentBox &b,
                           float extraMargin = 0.0f) {
  const Vector3 A[3] = {a.axisX, a.axisY, a.axisZ};
  const Vector3 B[3] = {b.axisX, b.axisY, b.axisZ};

  const float aExt[3] = {
      a.halfWidth + extraMargin,
      a.halfLength + extraMargin,
      a.halfDepth + extraMargin,
  };

  const float bExt[3] = {
      b.halfWidth + extraMargin,
      b.halfLength + extraMargin,
      b.halfDepth + extraMargin,
  };

  float R[3][3];
  float AbsR[3][3];

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      R[i][j] = Dot(A[i], B[j]);
      AbsR[i][j] = fabsf(R[i][j]) + 0.0001f;
    }
  }

  const Vector3 tWorld = Subtract(b.center, a.center);
  const float t[3] = {
      Dot(tWorld, A[0]),
      Dot(tWorld, A[1]),
      Dot(tWorld, A[2]),
  };

  float ra = 0.0f;
  float rb = 0.0f;

  for (int i = 0; i < 3; ++i) {
    ra = aExt[i];
    rb = bExt[0] * AbsR[i][0] + bExt[1] * AbsR[i][1] + bExt[2] * AbsR[i][2];
    if (fabsf(t[i]) > ra + rb) {
      return false;
    }
  }

  for (int j = 0; j < 3; ++j) {
    ra = aExt[0] * AbsR[0][j] + aExt[1] * AbsR[1][j] + aExt[2] * AbsR[2][j];
    rb = bExt[j];
    if (fabsf(t[0] * R[0][j] + t[1] * R[1][j] + t[2] * R[2][j]) > ra + rb) {
      return false;
    }
  }

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      ra = aExt[(i + 1) % 3] * AbsR[(i + 2) % 3][j] +
           aExt[(i + 2) % 3] * AbsR[(i + 1) % 3][j];
      rb = bExt[(j + 1) % 3] * AbsR[i][(j + 2) % 3] +
           bExt[(j + 2) % 3] * AbsR[i][(j + 1) % 3];

      const float value = fabsf(t[(i + 2) % 3] * R[(i + 1) % 3][j] -
                                t[(i + 1) % 3] * R[(i + 2) % 3][j]);

      if (value > ra + rb) {
        return false;
      }
    }
  }

  return true;
}

Vector2 MakeVector2(float x, float y) { return {x, y}; }

bool PointInRect(const Vector2 &point, const Vector2 &center,
                 const Vector2 &size) {
  const float left = center.x - size.x * 0.5f;
  const float right = center.x + size.x * 0.5f;
  const float top = center.y - size.y * 0.5f;
  const float bottom = center.y + size.y * 0.5f;

  return point.x >= left && point.x <= right && point.y >= top &&
         point.y <= bottom;
}

float GetEditMinRatio() { return 1.0f / 3.0f; }

float GetEditMaxRatio() { return 2.0f; }

void MakeEditRangeFromBase(float baseValue, float *outMinValue,
                           float *outMaxValue) {
  if (outMinValue == nullptr || outMaxValue == nullptr) {
    return;
  }

  const float safeBase = (std::max)(baseValue, 0.0001f);
  *outMinValue = safeBase * GetEditMinRatio();
  *outMaxValue = safeBase * GetEditMaxRatio();
}

} // namespace

ModScene::ModScene(kEngine *system) {
  // エンジン本体を保持する
  system_ = system;

  // シーン用ライトを作成して登録する
  light1_ = new Light;
  light1_->direction = {-0.5f, -1.0f, -0.3f};
  light1_->color = {1.0f, 1.0f, 1.0f};
  light1_->intensity = 1.0f;
  system_->AddLight(light1_);

  // 通常カメラとデバッグカメラを作成する
  debugCamera_ = system_->CreateDebugCamera();
  camera_ = system_->CreateCamera();

  // どちらのカメラも初期位置をそろえておく
  debugCamera_->SetTranslate({0.0f, 0.0f, -8.0f});
  debugCamera_->SetDefaultTransform(debugCamera_->GetTransform());

  camera_->SetTranslate({0.0f, 0.0f, -8.0f});
  camera_->SetDefaultTransform(camera_->GetTransform());

  // 初期状態ではデバッグカメラを使用する
  usingCamera_ = debugCamera_;
  system_->SetCamera(usingCamera_);

  // PromptScene で決まったお題文を取得する
  selectedPrompt_ = PromptData::GetSelectedPrompt();

  // 前シーンから共有されている改造データを取得する
  customizeData_ = ModBody::CopySharedCustomizeData();
  if (customizeData_ == nullptr) {
    customizeData_ = ModBody::CreateDefaultCustomizeData();
  }

  // 初期部位構成を作成し、保存データがあれば見た目へ読み込む
  SetupModObjects();
  LoadCustomizeData();
  EnsureValidSelection();

  // フェードインを開始する
  fade_.Initialize(system_);
  fade_.StartFadeIn();

  // 操作点表示用の白テクスチャを読み込む
  controlPointGizmoTextureHandle_ =
      system_->LoadTexture("GAME/resources/texture/white100x100.png");

  orbitTarget_ = ComputeOrbitTarget();
  orbitYaw_ = 0.0f;
  orbitPitch_ = 0.0f;
  orbitDistance_ = 8.0f;

  InitializeNpcModProgress();

  // テキスト描画用のビットマップフォントを初期化する
  bitmapFont_.Initialize(system);

  // 画面UIを初期化する
  InitializeScreenUi();

  pendingFailureOutcome_ = SceneOutcome::NONE;
}

ModScene::~ModScene() {
  // 作成したカメラを破棄する
  system_->DestroyCamera(camera_);
  system_->DestroyCamera(debugCamera_);

  // 登録したライトを解除して解放する
  system_->RemoveLight(light1_);
  delete light1_;

  // 使用していないマテリアルをクリーンアップする
  ResourceManager::GetInstance()->CleanupUnusedMaterials();

  // ビットマップフォントをクリーンアップする
  bitmapFont_.Cleanup();
}

void ModScene::Update() {

  // 使用中カメラを更新する
  CameraPart();

  // 画面UIの状態を更新する
  UpdateScreenUi();

  // 失敗メニューが開いている場合は、3D操作は行わずにメニューの更新と遷移処理だけ行う
  if (isFailureMenuOpen_) {
    UpdateNotifications();
    UpdateFailureMenuInputMod();
    fade_.Update(usingCamera_);

    if (isStartTransition_ && fade_.IsFinished()) {
      if (customizeData_ != nullptr) {
        ModBody::SetSharedCustomizeData(*customizeData_);
      }
      outcome_ = pendingFailureOutcome_;
    }
    return;
  }

  // 画面上の追加ボタンが押されたら、そのフレームは3D操作へ流さない
  if (TryHandleAddButtonClick()) {
    UpdateModObjects();
    SyncCustomizeDataFromScene();

    if (system_->GetTriggerOn(DIK_0)) {
      useDebugCamera_ = !useDebugCamera_;
    }

    if (!fade_.IsBusy() && system_->GetTriggerOn(DIK_SPACE)) {
      fade_.StartFadeOut();
      isStartTransition_ = true;
    }

    fade_.Update(usingCamera_);

    if (isStartTransition_ && fade_.IsFinished()) {
      if (customizeData_ != nullptr) {
        ModBody::SetSharedCustomizeData(*customizeData_);
      }
      outcome_ = SceneOutcome::NEXT;
    }
    return;
  }

  // このフレームで構造変更が発生したかを記録する
  bool assemblyChanged = false;

  // 構造変更があった場合は Object 一覧と選択状態を同期し直す
  if (assemblyChanged) {
    SyncObjectsWithAssembly();
    LoadCustomizeData();
    EnsureValidSelection();
    ClearControlPointSelection();
  }

  // 操作点の選択とドラッグ移動を処理する
  UpdateControlPointEditing();

  // 現在の構造とパラメータを見た目へ反映する
  UpdateModObjects();

  // NPCの裏改造進行を更新する
  UpdateNpcProgress();

  if (isStartTransition_) {
    // 胴体(id=1)と頭(id=4)の操作点を確認する
    for (const auto &pair : modBodies_) {
      const auto &points = pair.second.GetControlPoints();
      if (!points.empty()) {
        for (size_t pi = 0; pi < points.size(); ++pi) {
        }
      }
    }
  }

  // 現在のシーン状態を共有データへ書き戻す
  SyncCustomizeDataFromScene();

  // カメラ切り替えを行う
  if (system_->GetTriggerOn(DIK_0)) {
    useDebugCamera_ = !useDebugCamera_;
  }

  // Space 入力で次シーンへのフェードアウトを開始する
  if (!fade_.IsBusy() && system_->GetTriggerOn(DIK_SPACE)) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
  }

  // フェード演出を更新する
  fade_.Update(usingCamera_);

  // フェードアウト完了後に共有データを保存して次シーンへ進む
  if (isStartTransition_ && fade_.IsFinished()) {
    if (customizeData_ != nullptr) {
      for (size_t i = 0; i < customizeData_->controlPointSnapshots.size();
           ++i) {
        const auto &s = customizeData_->controlPointSnapshots[i];
      }
      ModBody::SetSharedCustomizeData(*customizeData_);
    }
    outcome_ = SceneOutcome::NEXT;
  }
}

void ModScene::Draw() {
  // 改造部位 Object を描画する
  DrawModObjects();

  // 操作点表示球を描画する
  DrawControlPointGizmos();

#ifdef USE_IMGUI
  // シーン共通の簡易操作説明を表示する
  ImGui::Begin("Scene");
  ImGui::Text("ModScene");
  ImGui::Text("Selected Prompt:");
  ImGui::Text("%s", selectedPrompt_.c_str());
  ImGui::End();

  // 改造用UI本体を表示する
  DrawModGui();

  // NPC進行確認用
  ImGui::Begin("NpcModProgress");

  ImGui::Text("NPC Count : %zu", npcProgress_.size());
  ImGui::Text("Notification Count : %zu", notifications_.size());
  ImGui::Text("NpcGoalCountInMod : %d", npcGoalCountInMod_);
  ImGui::Separator();

  for (size_t i = 0; i < npcProgress_.size(); ++i) {
    const NpcModProgress &npc = npcProgress_[i];

    float remainTime = npc.totalTime - npc.elapsedTime;
    if (remainTime < 0.0f) {
      remainTime = 0.0f;
    }

    ImGui::PushID(static_cast<int>(i));
    ImGui::Text("%s", npc.name.c_str());
    ImGui::Text("  elapsedTime      : %.2f", npc.elapsedTime);
    ImGui::Text("  totalTime        : %.2f", npc.totalTime);
    ImGui::Text("  remainTime       : %.2f", remainTime);
    ImGui::Text("  isFinished       : %s", npc.isFinished ? "true" : "false");
    ImGui::Text("  hasStartedMoving : %s",
                npc.hasStartedMoving ? "true" : "false");
    ImGui::Text("  moveElapsedTime  : %.2f", npc.moveElapsedTime);
    ImGui::Text("  hasReachedGoal   : %s",
                npc.hasReachedGoal ? "true" : "false");
    ImGui::Separator();
    ImGui::PopID();
  }

  ImGui::End();
#endif

  bitmapFont_.BeginFrame();
  // 画面固定の追加ボタン・ゴミ箱UIを描画する
  if (!isFailureMenuOpen_) {
    DrawScreenUi();
  }

  // 失敗メニューが開いていない場合は、NPC進行通知を描画する
  DrawStartNotifications();

  // 失敗メニューが開いている場合は、メニューを描画する
  DrawFailureMenuMod();

  // フェードを描画する
  fade_.Draw();
}

void ModScene::CameraPart() {
  UpdateOrbitCamera();

  if (useDebugCamera_) {
    usingCamera_ = debugCamera_;
  } else {
    usingCamera_ = camera_;
  }

  system_->SetCamera(usingCamera_);
}

Vector3 ModScene::ComputeOrbitTarget() const {
  // まず胴体中心を優先して見る
  const int chestIndex = FindTorsoControlPointIndex(ModControlPointRole::Chest);
  const int bellyIndex = FindTorsoControlPointIndex(ModControlPointRole::Belly);
  const int waistIndex = FindTorsoControlPointIndex(ModControlPointRole::Waist);

  if (chestIndex >= 0 && bellyIndex >= 0 && waistIndex >= 0) {
    const Vector3 chest =
        GetTorsoControlPointWorldPosition(ModControlPointRole::Chest);
    const Vector3 belly =
        GetTorsoControlPointWorldPosition(ModControlPointRole::Belly);
    const Vector3 waist =
        GetTorsoControlPointWorldPosition(ModControlPointRole::Waist);

    Vector3 center{};
    center.x = (chest.x + belly.x + waist.x) / 3.0f;
    center.y = (chest.y + belly.y + waist.y) / 3.0f;
    center.z = (chest.z + belly.z + waist.z) / 3.0f;
    return center;
  }

  // 胴体が取れないときは選択部位を見る
  if (selectedPartId_ >= 0 && modObjects_.count(selectedPartId_) > 0) {
    Object *object = modObjects_.at(selectedPartId_).get();
    if (object != nullptr) {
      return ModObjectUtil::ComputeObjectRootWorldTranslate(object);
    }
  }

  // 何も無ければ原点少し上
  return {0.0f, 0.5f, 0.0f};
}

void ModScene::UpdateOrbitCamera() {
  if (debugCamera_ == nullptr || camera_ == nullptr) {
    return;
  }

#ifdef USE_IMGUI
  const bool wantCaptureMouse = ImGui::GetIO().WantCaptureMouse;
#else
  const bool wantCaptureMouse = false;
#endif

  const bool blockCameraInput =
      wantCaptureMouse || ShouldBlockDebugCameraMouseControl();

  static bool wasRightPressed = false;
  static POINT prevCursorPos{};

  POINT currentCursorPos{};
  GetCursorPos(&currentCursorPos);

  const bool rightPressed = IsMouseRightPressed();

  // 注視ターゲットだけは毎フレーム更新
  orbitTarget_ = ComputeOrbitTarget();

  // --------------------------------
  // 右クリック開始時：
  // 「今のカメラ位置」から orbit パラメータを逆算する
  // これで初期位置は変えない
  // --------------------------------
  if (!blockCameraInput && rightPressed && !wasRightPressed) {
    Vector3 currentPos = debugCamera_->GetTransform().translate;
    Vector3 offset = Subtract(currentPos, orbitTarget_);

    orbitDistance_ = Length(offset);
    if (orbitDistance_ < 0.0001f) {
      orbitDistance_ = 0.0001f;
    }

    orbitYaw_ = atan2f(offset.x, -offset.z);

    const float horizontalLength =
        sqrtf(offset.x * offset.x + offset.z * offset.z);
    orbitPitch_ = atan2f(offset.y, horizontalLength);

    prevCursorPos = currentCursorPos;
  }

  // --------------------------------
  // 右ドラッグ中だけ orbit 回転
  // --------------------------------
  if (!blockCameraInput && rightPressed) {
    if (wasRightPressed) {
      const float deltaX =
          static_cast<float>(currentCursorPos.x - prevCursorPos.x);
      const float deltaY =
          static_cast<float>(currentCursorPos.y - prevCursorPos.y);

      orbitYaw_ -= deltaX * orbitRotateSpeed_;
      orbitPitch_ += deltaY * orbitRotateSpeed_;
      orbitPitch_ = (std::clamp)(orbitPitch_, -1.2f, 1.2f);
    }

    prevCursorPos = currentCursorPos;

    const float cosPitch = cosf(orbitPitch_);
    const float sinPitch = sinf(orbitPitch_);
    const float cosYaw = cosf(orbitYaw_);
    const float sinYaw = sinf(orbitYaw_);

    Vector3 cameraPos{};
    cameraPos.x = orbitTarget_.x + orbitDistance_ * cosPitch * sinYaw;
    cameraPos.y = orbitTarget_.y + orbitDistance_ * sinPitch;
    cameraPos.z = orbitTarget_.z - orbitDistance_ * cosPitch * cosYaw;

    Vector3 toTarget = Subtract(orbitTarget_, cameraPos);
    Vector3 dir = NormalizeSafeV(toTarget, {0.0f, 0.0f, 1.0f});

    Vector3 cameraRotate{};
    cameraRotate.y = atan2f(dir.x, dir.z);

    const float horizontalLength = sqrtf(dir.x * dir.x + dir.z * dir.z);
    cameraRotate.x = atan2f(-dir.y, horizontalLength);

    cameraRotate.z = 0.0f;

    debugCamera_->SetTranslate(cameraPos);
    debugCamera_->SetRotation(cameraRotate);

    camera_->SetTranslate(cameraPos);
    camera_->SetRotation(cameraRotate);
  }

  // --------------------------------
  // ホイール zoom も右押し中だけ反映
  // 初期位置を勝手に変えないため
  // --------------------------------
  if (!blockCameraInput) {
    const int wheelDelta = system_->GetMouseScrollOrigin();
    if (wheelDelta != 0) {
      orbitDistance_ -=
          (static_cast<float>(wheelDelta) / 120.0f) * orbitZoomSpeed_;
      orbitDistance_ =
          (std::clamp)(orbitDistance_, orbitMinDistance_, orbitMaxDistance_);

      const float cosPitch = cosf(orbitPitch_);
      const float sinPitch = sinf(orbitPitch_);
      const float cosYaw = cosf(orbitYaw_);
      const float sinYaw = sinf(orbitYaw_);

      Vector3 cameraPos{};
      cameraPos.x = orbitTarget_.x + orbitDistance_ * cosPitch * sinYaw;
      cameraPos.y = orbitTarget_.y + orbitDistance_ * sinPitch;
      cameraPos.z = orbitTarget_.z - orbitDistance_ * cosPitch * cosYaw;

      debugCamera_->SetTranslate(cameraPos);
      camera_->SetTranslate(cameraPos);
    }
  }

  wasRightPressed = rightPressed;
}

void ModScene::SetupModObjects() {
  // 初期人型構造を作成する
  assembly_.InitializeDefaultHumanoid();

  // 胴体共有操作点を初期化する
  ResetTorsoControlPoints();

  // Graph に合わせて Object 一覧を生成する
  SyncObjectsWithAssembly();

  // 初期レイアウトを整える
  SetupInitialLayout();

  // 改造パラメータを初期化する
  ResetModBodies();

  // SetupModObjects() の末尾の defaultControlPointSnapshots 保存部分を差し替え
  if (customizeData_ != nullptr) {
    customizeData_->defaultControlPointSnapshots.clear();

    // まず torsoControlPoints_ から胴体のデフォルト操作点を保存する
    {
      int chestPartId = -1;
      for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
        const PartNode *node = assembly_.FindNode(orderedPartIds_[i]);
        if (node != nullptr && node->part == ModBodyPart::ChestBody) {
          chestPartId = orderedPartIds_[i];
          break;
        }
      }

      if (chestPartId >= 0) {
        for (size_t pi = 0; pi < torsoControlPoints_.size(); ++pi) {
          const auto &point = torsoControlPoints_[pi];
          ModControlPointSnapshot snap;
          snap.ownerPartId = chestPartId;
          snap.ownerPartType = ModBodyPart::ChestBody;
          snap.role = point.role;
          snap.localPosition = point.localPosition;
          snap.radius = point.radius;
          snap.movable = point.movable;
          snap.isConnectionPoint = point.isConnectionPoint;
          snap.acceptsParent = point.acceptsParent;
          snap.acceptsChild = point.acceptsChild;
          customizeData_->defaultControlPointSnapshots.push_back(snap);
        }
      }
    }

    // 残りの部位は modBodies_ から（胴体はスキップ）
    for (const auto &id : orderedPartIds_) {
      if (modBodies_.count(id) == 0)
        continue;
      const PartNode *node = assembly_.FindNode(id);
      if (node == nullptr)
        continue;

      // ChestBody と StomachBody は torsoControlPoints_ で処理済み
      if (node->part == ModBodyPart::ChestBody ||
          node->part == ModBodyPart::StomachBody) {
        continue;
      }

      const auto &points = modBodies_[id].GetControlPoints();
      for (const auto &point : points) {
        ModControlPointSnapshot snap;
        snap.ownerPartId = id;
        snap.ownerPartType = node->part;
        snap.role = point.role;
        snap.localPosition = point.localPosition;
        snap.radius = point.radius;
        snap.movable = point.movable;
        snap.isConnectionPoint = point.isConnectionPoint;
        snap.acceptsParent = point.acceptsParent;
        snap.acceptsChild = point.acceptsChild;
        customizeData_->defaultControlPointSnapshots.push_back(snap);
      }
    }
  }
}

void ModScene::SetupInitialLayout() {
  // 現在の部位ID一覧を取得する
  orderedPartIds_ = assembly_.GetNodeIdsSorted();

  // すべての部位スケールを等倍で初期化する
  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    assembly_.SetPartScale(orderedPartIds_[i], {1.0f, 1.0f, 1.0f});
  }
}

void ModScene::SyncObjectsWithAssembly() {
  // 現在の構造から部位ID一覧を更新する
  orderedPartIds_ = assembly_.GetNodeIdsSorted();

  // 現在生存している部位ID集合を作る
  std::unordered_set<int> alive;
  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    alive.insert(orderedPartIds_[i]);
  }

  // 既に削除された部位に対応する Object やデータを消す
  for (std::unordered_map<int, std::unique_ptr<Object>>::iterator it =
           modObjects_.begin();
       it != modObjects_.end();) {
    if (alive.count(it->first) == 0) {
      modBodies_.erase(it->first);
      modModelHandles_.erase(it->first);
      it = modObjects_.erase(it);
    } else {
      ++it;
    }
  }

  // 新しく追加された部位の Object を生成する
  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    if (modObjects_.count(id) > 0) {
      continue;
    }

    const PartNode *node = assembly_.FindNode(id);
    if (node == nullptr) {
      continue;
    }
    CreateObjectForNode(id, *node);
  }

  // 選択状態を有効なものへ補正する
  EnsureValidSelection();
}

void ModScene::CreateObjectForNode(int partId, const PartNode &node) {
  // 部位種類に対応するモデルパスを取得する
  const std::string path = ModelPath(node.part);

  // モデルを読み込み、ハンドルを保持する
  modModelHandles_[partId] = system_->SetModelObj(path);

  // Object を生成してモデルデータを設定する
  std::unique_ptr<Object> object = std::make_unique<Object>();
  object->IntObject(system_);
  object->CreateModelData(modModelHandles_[partId]);
  object->mainPosition.transform = CreateDefaultTransform();

  // 管理コンテナへ登録し、対応する ModBody も初期化する
  modObjects_[partId] = std::move(object);
  modBodies_[partId].Initialize(modObjects_[partId].get(), node.part);

  if (node.part == ModBodyPart::Head && modObjects_[partId] != nullptr) {
    // Logger::Log("HEAD CREATE");
    // Logger::Log("partId = %d", partId);
    // Logger::Log("objectParts size = %zu",
    //             modObjects_[partId]->objectParts_.size());
  }
}

void ModScene::ApplyAssemblyToSceneHierarchy() {
  for (std::unordered_map<int, std::unique_ptr<Object>>::iterator it =
           modObjects_.begin();
       it != modObjects_.end(); ++it) {
    Object *object = it->second.get();
    if (object == nullptr) {
      continue;
    }

    object->followObject_ = nullptr;
    object->mainPosition.parentPart = nullptr;
  }

  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];

    const PartNode *node = assembly_.FindNode(id);
    if (node == nullptr || modObjects_.count(id) == 0) {
      continue;
    }

    Object *object = modObjects_[id].get();
    if (object == nullptr) {
      continue;
    }

    Vector3 localTranslate = node->localTransform.translate;
    Vector3 localRotate = node->localTransform.rotate;

    if (node->parentId >= 0 && modObjects_.count(node->parentId) > 0) {
      Object *parentObject = modObjects_[node->parentId].get();
      if (parentObject != nullptr) {
        object->followObject_ = &parentObject->mainPosition;
        object->mainPosition.parentPart = &parentObject->mainPosition;
      }

      // すべての親子接続をここで統一的に解決する
      localTranslate = ResolveAttachedLocalTranslate(*node);
    }

    object->mainPosition.transform.translate = localTranslate;
    // object->mainPosition.transform.rotate = localRotate;
    object->mainPosition.transform.scale = {1.0f, 1.0f, 1.0f};
  }
}

void ModScene::LoadCustomizeData() {
  if (customizeData_ == nullptr) {
    return;
  }

  if (!customizeData_->partInstances.empty()) {
    for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
      const int id = orderedPartIds_[i];
      if (modBodies_.count(id) == 0) {
        continue;
      }

      bool found = false;
      for (size_t j = 0; j < customizeData_->partInstances.size(); ++j) {
        const ModPartInstanceData &instance = customizeData_->partInstances[j];
        if (instance.partId != id) {
          continue;
        }

        modBodies_[id].SetParam(instance.param);

        if (assembly_.FindNode(id) != nullptr) {
          assembly_.SetPartLocalTranslate(id,
                                          instance.localTransform.translate);
        }

        found = true;
        break;
      }

      // partId 一致が無い部位は「新規追加部位」とみなして、
      // 旧方式 partParams へはフォールバックしない。
      // ModBody::Initialize / Reset で入っている初期値をそのまま使う。
      if (!found) {
        ModBodyPartParam param = modBodies_[id].GetParam();
        param.count = 1;
        param.enabled = true;
        modBodies_[id].SetParam(param);
      }
    }

    return;
  }

  // 新方式データが無い場合だけ旧方式にフォールバックする
  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    const PartNode *node = assembly_.FindNode(id);
    if (node == nullptr) {
      continue;
    }

    if (modBodies_.count(id) == 0) {
      continue;
    }

    const size_t index = ToIndex(node->part);
    if (index >= static_cast<size_t>(ModBodyPart::Count)) {
      continue;
    }

    ModBodyPartParam param = customizeData_->partParams[index];
    param.count = 1;

    // 旧方式しかないときは、最低限見えるようにする
    if (param.count <= 0) {
      param.scale = {1.0f, 1.0f, 1.0f};
      param.length = 1.0f;
      param.enabled = true;
      param.count = 1;
    }

    modBodies_[id].SetParam(param);
  }
}

void ModScene::SyncCustomizeDataFromScene() {
  // 共有データが無い場合は何もしない
  if (customizeData_ == nullptr) {
    return;
  }

  // 現在のシーン状態から新方式の partInstances を作り直す
  customizeData_->partInstances.clear();
  customizeData_->partInstances.reserve(orderedPartIds_.size());

  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    const PartNode *node = assembly_.FindNode(id);
    if (node == nullptr || modBodies_.count(id) == 0) {
      continue;
    }

    // 部位ごとの構造情報と改造パラメータを保存用データへ詰める
    ModPartInstanceData instance;
    instance.partId = id;
    instance.partType = node->part;
    instance.parentId = node->parentId;
    instance.parentConnectorId = node->parentConnectorId;
    instance.selfConnectorId = node->selfConnectorId;
    instance.localTransform = node->localTransform;
    instance.param = modBodies_[id].GetParam();

    // 新方式ではインスタンス単位で count は常に 1
    instance.param.count = 1;

    customizeData_->partInstances.push_back(instance);
  }

  // 新方式の可変長操作点配列を再構築する
  RebuildControlPointSnapshotsFromScene();

  // 旧固定操作点配列も互換用に保存する
  SaveControlPointsToCustomizeData();

  // 旧方式配列も互換用に再構築する
  RebuildLegacyCustomizeDataFromInstances();

  // NPCの進行状態も共有データへ保存する
  SyncNpcProgressToCustomizeData();

  // shared に積む前に整合性を揃えておく
  ModBody::NormalizeCustomizeData(*customizeData_);
}

void ModScene::RebuildControlPointSnapshotsFromScene() {
  if (customizeData_ == nullptr) {
    return;
  }

  customizeData_->controlPointSnapshots.clear();

  // まず torsoControlPoints_ から胴体の操作点を保存する
  {
    // ChestBody の partId を探す
    int chestPartId = -1;
    for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
      const PartNode *node = assembly_.FindNode(orderedPartIds_[i]);
      if (node != nullptr && node->part == ModBodyPart::ChestBody) {
        chestPartId = orderedPartIds_[i];
        break;
      }
    }

    if (chestPartId >= 0) {
      for (size_t pi = 0; pi < torsoControlPoints_.size(); ++pi) {
        const auto &point = torsoControlPoints_[pi];

        ModControlPointSnapshot snapshot;
        snapshot.ownerPartId = chestPartId;
        snapshot.ownerPartType = ModBodyPart::ChestBody;
        snapshot.role = point.role;
        snapshot.localPosition = point.localPosition;
        snapshot.radius = point.radius;
        snapshot.movable = point.movable;
        snapshot.isConnectionPoint = point.isConnectionPoint;
        snapshot.acceptsParent = point.acceptsParent;
        snapshot.acceptsChild = point.acceptsChild;

        customizeData_->controlPointSnapshots.push_back(snapshot);
      }

      auto PushTorsoAnchorSnapshot = [&](ModControlPointRole role,
                                         const Vector3 &localPosition,
                                         float radius) {
        ModControlPointSnapshot snapshot;
        snapshot.ownerPartId = chestPartId;
        snapshot.ownerPartType = ModBodyPart::ChestBody;
        snapshot.role = role;
        snapshot.localPosition = localPosition;
        snapshot.radius = radius;
        snapshot.movable = false;
        snapshot.isConnectionPoint = true;
        snapshot.acceptsParent = false;
        snapshot.acceptsChild = true;

        customizeData_->controlPointSnapshots.push_back(snapshot);
      };

      auto FindTorsoLocal = [&](ModControlPointRole role,
                                const Vector3 &fallback) -> Vector3 {
        for (const auto &point : torsoControlPoints_) {
          if (point.role == role) {
            return point.localPosition;
          }
        }
        return fallback;
      };

      auto NormalizeSafe = [&](const Vector3 &v,
                               const Vector3 &fallback) -> Vector3 {
        float len = Length(v);
        if (len < 0.0001f) {
          return fallback;
        }
        return {v.x / len, v.y / len, v.z / len};
      };

      auto MulScalar = [&](const Vector3 &v, float s) -> Vector3 {
        return {v.x * s, v.y * s, v.z * s};
      };

      auto AddV = [&](const Vector3 &a, const Vector3 &b) -> Vector3 {
        return {a.x + b.x, a.y + b.y, a.z + b.z};
      };

      auto SubV = [&](const Vector3 &a, const Vector3 &b) -> Vector3 {
        return {a.x - b.x, a.y - b.y, a.z - b.z};
      };

      Vector3 chestLocal = {0.0f, 1.27f, 0.0f};
      for (const auto &point : torsoControlPoints_) {
        if (point.role == ModControlPointRole::Chest) {
          chestLocal = point.localPosition;
          break;
        }
      }

      PushTorsoAnchorSnapshot(ModControlPointRole::NeckBase, chestLocal, 0.08f);

      // 肩・股関節アンカーは再計算せず、control point
      // の現在位置をそのまま保存する
      PushTorsoAnchorSnapshot(
          ModControlPointRole::LeftShoulder,
          GetControlPointLocalPosition(ModControlPointRole::LeftShoulder),
          0.09f);

      PushTorsoAnchorSnapshot(
          ModControlPointRole::RightShoulder,
          GetControlPointLocalPosition(ModControlPointRole::RightShoulder),
          0.09f);

      PushTorsoAnchorSnapshot(
          ModControlPointRole::LeftHip,
          GetControlPointLocalPosition(ModControlPointRole::LeftHip), 0.10f);

      PushTorsoAnchorSnapshot(
          ModControlPointRole::RightHip,
          GetControlPointLocalPosition(ModControlPointRole::RightHip), 0.10f);
    }
  }

  // 残りの部位は modBodies_ から読み取る（胴体はスキップ）
  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];

    const PartNode *node = assembly_.FindNode(id);
    if (node == nullptr) {
      continue;
    }

    // ChestBody と StomachBody は torsoControlPoints_ で処理済みなのでスキップ
    if (node->part == ModBodyPart::ChestBody ||
        node->part == ModBodyPart::StomachBody) {
      continue;
    }

    if (modBodies_.count(id) == 0) {
      continue;
    }

    const std::vector<ModControlPoint> &points =
        modBodies_.at(id).GetControlPoints();

    if (id == 13) {
      for (size_t pointIndex = 0; pointIndex < points.size(); ++pointIndex) {
      }
    }

    for (size_t pointIndex = 0; pointIndex < points.size(); ++pointIndex) {
      const ModControlPoint &point = points[pointIndex];

      ModControlPointSnapshot snapshot;
      snapshot.ownerPartId = id;
      snapshot.ownerPartType = node->part;
      snapshot.role = point.role;
      snapshot.localPosition = point.localPosition;
      snapshot.radius = point.radius;
      snapshot.movable = point.movable;
      snapshot.isConnectionPoint = point.isConnectionPoint;
      snapshot.acceptsParent = point.acceptsParent;
      snapshot.acceptsChild = point.acceptsChild;

      customizeData_->controlPointSnapshots.push_back(snapshot);

      if (node->part == ModBodyPart::Neck || node->part == ModBodyPart::Head) {
        // Logger::Log("=== MODSCENE SNAP CHECK ===");
        // Logger::Log("partType   : %d", static_cast<int>(node->part));
        // Logger::Log("ownerPartId: %d", id);
        // Logger::Log("role       : %d", static_cast<int>(snapshot.role));
        // Logger::Log("localPos   : (%.3f, %.3f, %.3f)",
        // snapshot.localPosition.x,
        //             snapshot.localPosition.y, snapshot.localPosition.z);
        // Logger::Log("radius     : %.3f", snapshot.radius);
      }
    }
  }
}

void ModScene::RebuildLegacyCustomizeDataFromInstances() {
  // 共有データが無い場合は何もしない
  if (customizeData_ == nullptr) {
    return;
  }

  // 旧方式配列を一旦初期化する
  for (size_t i = 0; i < static_cast<size_t>(ModBodyPart::Count); ++i) {
    customizeData_->partParams[i].scale = {1.0f, 1.0f, 1.0f};
    customizeData_->partParams[i].length = 1.0f;
    customizeData_->partParams[i].count = 0;
    customizeData_->partParams[i].enabled = false;
  }

  // 各部位種別について代表値を1つ保存するためのフラグを用意する
  std::array<bool, static_cast<size_t>(ModBodyPart::Count)> hasRepresentative{};
  for (size_t i = 0; i < hasRepresentative.size(); ++i) {
    hasRepresentative[i] = false;
  }

  // partInstances から代表値を拾いながら、同時に部位数を増やす
  for (size_t i = 0; i < customizeData_->partInstances.size(); ++i) {
    const ModPartInstanceData &instance = customizeData_->partInstances[i];
    const size_t index = ToIndex(instance.partType);
    if (index >= static_cast<size_t>(ModBodyPart::Count)) {
      continue;
    }

    customizeData_->partParams[index].count += 1;

    if (!hasRepresentative[index]) {
      customizeData_->partParams[index] = instance.param;
      customizeData_->partParams[index].count = 1;
      hasRepresentative[index] = true;
    }
  }

  // 元コードの流れを保つため、部位種別ごとの count を再計算する
  for (size_t i = 0; i < customizeData_->partInstances.size(); ++i) {
    const ModPartInstanceData &instance = customizeData_->partInstances[i];
    const size_t index = ToIndex(instance.partType);
    if (index >= static_cast<size_t>(ModBodyPart::Count)) {
      continue;
    }

    customizeData_->partParams[index].count += 0;
  }

  // 正確な部位数を数え直す
  std::array<int, static_cast<size_t>(ModBodyPart::Count)> counts{};
  for (size_t i = 0; i < counts.size(); ++i) {
    counts[i] = 0;
  }

  for (size_t i = 0; i < customizeData_->partInstances.size(); ++i) {
    const size_t index = ToIndex(customizeData_->partInstances[i].partType);
    if (index >= static_cast<size_t>(ModBodyPart::Count)) {
      continue;
    }
    counts[index] += 1;
  }

  // 数え直した count を最終的な値として入れる
  for (size_t i = 0; i < counts.size(); ++i) {
    customizeData_->partParams[i].count = counts[i];
  }
}

void ModScene::ResetModBodies() {
  // 全部位の改造パラメータを初期化する
  for (std::unordered_map<int, ModBody>::iterator it = modBodies_.begin();
       it != modBodies_.end(); ++it) {
    it->second.Reset();
  }
}

void ModScene::ResetSelectedPartParams() {
  // 選択中部位が無い場合は何もしない
  if (selectedPartId_ < 0) {
    return;
  }
  if (modBodies_.count(selectedPartId_) == 0) {
    return;
  }

  // 選択中部位だけ見た目パラメータを初期化する
  modBodies_[selectedPartId_].Reset();
}

void ModScene::ResetToDefaultHumanoid() {

  // 構造を初期人型へ戻す
  assembly_.InitializeDefaultHumanoid();

  // 胴体共有操作点も初期化する
  ResetTorsoControlPoints();

  // Object 一覧を構造に合わせて再同期する
  SyncObjectsWithAssembly();

  // 初期レイアウトと改造パラメータを作り直す
  SetupInitialLayout();
  ResetModBodies();

  // 選択状態と共有データも初期状態へ寄せる
  EnsureValidSelection();
  SyncCustomizeDataFromScene();
}

void ModScene::SelectPart(int partId) {
  // 選択部位を更新する
  selectedPartId_ = partId;

  // 付け替え候補は選択し直しになるのでリセットする
  reattachParentId_ = -1;
  reattachConnectorId_ = -1;

  // 部位を選び直したら操作点選択もいったん解除する
  ClearControlPointSelection();
}

void ModScene::EnsureValidSelection() {
  // 現在の選択がまだ有効ならそのまま維持する
  if (selectedPartId_ >= 0 && assembly_.FindNode(selectedPartId_) != nullptr &&
      modObjects_.count(selectedPartId_) > 0) {
    return;
  }

  // 無効ならいったん未選択に戻す
  selectedPartId_ = -1;

  // 選択可能な先頭部位を新たな選択対象にする
  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    const PartNode *node = assembly_.FindNode(id);
    if (node == nullptr) {
      continue;
    }
    if (!IsSelectablePart(node->part)) {
      continue;
    }

    selectedPartId_ = id;
    break;
  }

  // 付け替え候補はリセットする
  reattachParentId_ = -1;
  reattachConnectorId_ = -1;

  // 部位選択が無効化された場合は操作点選択も解除する
  ClearControlPointSelection();
}

void ModScene::DeleteSelectedPart() {
  // 選択中部位が無い場合は何もしない
  if (selectedPartId_ < 0) {
    return;
  }

  // セット単位削除が必要な場合を考慮して対象部位IDを補正する
  const int targetId = ResolveAssemblyOperationPartId(selectedPartId_);
  if (targetId < 0) {
    return;
  }

  // Graph から削除できなければ終了する
  if (!assembly_.RemovePart(targetId)) {
    return;
  }

  // 削除後の Object 一覧と選択状態を再同期する
  selectedPartId_ = targetId;
  SyncObjectsWithAssembly();
  EnsureValidSelection();
  ClearControlPointSelection();
}

void ModScene::ReattachSelectedPart() {
  if (selectedPartId_ < 0 || reattachParentId_ < 0) {
    return;
  }

  const int targetId = ResolveAssemblyOperationPartId(selectedPartId_);
  if (targetId < 0) {
    return;
  }

  if (!assembly_.MovePart(targetId, reattachParentId_, -1)) {
    return;
  }

  SyncObjectsWithAssembly();
  ClearControlPointSelection();
}

void ModScene::NudgeSelectedPart(const Vector3 &delta) {
  // 選択部位が無い場合は何もしない
  if (selectedPartId_ < 0) {
    return;
  }

  // 現在のローカル位置を取得する
  const PartNode *node = assembly_.FindNode(selectedPartId_);
  if (node == nullptr) {
    return;
  }

  // delta を加算してローカル位置を更新する
  assembly_.SetPartLocalTranslate(selectedPartId_,
                                  Add(node->localTransform.translate, delta));
}

int ModScene::ResolveAssemblyOperationPartId(int partId) const {
  const PartNode *node = assembly_.FindNode(partId);
  if (node == nullptr) {
    return -1;
  }

  switch (node->part) {
  case ModBodyPart::Head:
    if (node->parentId >= 0) {
      const PartNode *parent = assembly_.FindNode(node->parentId);
      if (parent != nullptr && parent->part == ModBodyPart::Neck) {
        return parent->id;
      }
    }
    break;

  case ModBodyPart::LeftForeArm:
    if (node->parentId >= 0) {
      const PartNode *parent = assembly_.FindNode(node->parentId);
      if (parent != nullptr && parent->part == ModBodyPart::LeftUpperArm) {
        return parent->id;
      }
    }
    break;

  case ModBodyPart::RightForeArm:
    if (node->parentId >= 0) {
      const PartNode *parent = assembly_.FindNode(node->parentId);
      if (parent != nullptr && parent->part == ModBodyPart::RightUpperArm) {
        return parent->id;
      }
    }
    break;

  case ModBodyPart::LeftShin:
    if (node->parentId >= 0) {
      const PartNode *parent = assembly_.FindNode(node->parentId);
      if (parent != nullptr && parent->part == ModBodyPart::LeftThigh) {
        return parent->id;
      }
    }
    break;

  case ModBodyPart::RightShin:
    if (node->parentId >= 0) {
      const PartNode *parent = assembly_.FindNode(node->parentId);
      if (parent != nullptr && parent->part == ModBodyPart::RightThigh) {
        return parent->id;
      }
    }
    break;

  default:
    break;
  }

  return partId;
}

void ModScene::UpdateControlPointEditing() {
#ifdef USE_IMGUI
  if (ImGui::GetIO().WantCaptureMouse) {
    if (!IsMouseLeftPressedNow()) {
      isDraggingControlPoint_ = false;
    }
    hoveredPartId_ = -1;
    return;
  }
#endif

  if (usingCamera_ == nullptr) {
    return;
  }

  Ray mouseRay = usingCamera_->ScreenPointToRay(system_->GetMousePosVector2());

  // 画面UI上では3D側の選択・ドラッグ開始をさせない
  if (!assemblyDrag_.isDragging && IsMouseOverAnyScreenUi()) {
    hoveredPartId_ = -1;
    return;
  }

  // Assembly ドラッグ中はそちらを最優先
  if (assemblyDrag_.isDragging) {
    UpdateAssemblyAttachCandidateFromMouseRay(mouseRay);
    ApplyAssemblyDragPreview();

    if (IsMouseLeftReleasedNow()) {
      if (DeleteDraggingAssemblyByTrashDrop()) {
        return;
      }

      if (assemblyDrag_.isPlacementValid) {
        ConfirmAssemblyDragPlacement();
      } else {
        CancelAssemblyDragPlacement();
      }
      return;
    }

    return;
  }

  // 通常時の hover 更新
  UpdateHoveredPartFromMouseRay(mouseRay);

  if (selectedControlPointIndex_ >= 0 && !isDraggingControlPoint_) {
    if (!IsMouseRayInsideSelectedControlMesh(mouseRay)) {
      ClearControlPointSelection();
      return;
    }
  }

  UpdateControlPointWheelScaling();

  // 左クリック開始時
  if (IsMouseLeftTriggeredNow()) {
    // まず操作点を優先。ただし Root は PickControlPointFromMouseRay() 側で弾く
    if (PickControlPointFromMouseRay(mouseRay)) {
      isDraggingControlPoint_ = true;
      return;
    }

    // 操作点でなければ部位ドラッグ開始
    if (TryBeginAssemblyDragFromMouseRay(mouseRay)) {
      return;
    }
  }

  // 操作点ドラッグ中
  if (isDraggingControlPoint_ && IsMouseLeftPressedNow()) {
    MoveSelectedControlPointFromMouseRay(mouseRay);
  }

  // 左ボタンを離したら操作点ドラッグ終了
  if (IsMouseLeftReleasedNow()) {
    isDraggingControlPoint_ = false;
  }
}

bool ModScene::PickControlPointFromMouseRay(const Ray &mouseRay) {
  const int visiblePartId =
      (hoveredPartId_ >= 0) ? hoveredPartId_ : selectedPartId_;

  if (visiblePartId < 0) {
    return false;
  }

  // 胴体共有点を優先して判定する
  if (IsTorsoVisiblePartId(visiblePartId)) {
    float nearestT = FLT_MAX;
    int nearestPointIndex = -1;

    for (size_t i = 0; i < torsoControlPoints_.size(); ++i) {
      if (!(torsoControlPoints_[i].movable ||
            torsoControlPoints_[i].isConnectionPoint)) {
        continue;
      }

      Sphere sphere{};
      sphere.center =
          GetTorsoControlPointWorldPosition(torsoControlPoints_[i].role);
      sphere.radius = torsoControlPoints_[i].radius;

      float t = 0.0f;
      if (!crashDecision(sphere, mouseRay, &t)) {
        continue;
      }

      if (t < nearestT) {
        nearestT = t;
        nearestPointIndex = static_cast<int>(i);
      }
    }

    if (nearestPointIndex < 0) {
      return false;
    }

    selectedControlPartId_ = -2;
    selectedControlPointIndex_ = nearestPointIndex;
    selectedPartId_ = visiblePartId;
    reattachParentId_ = -1;
    reattachConnectorId_ = -1;

    const Vector3 worldPos = GetTorsoControlPointWorldPosition(
        torsoControlPoints_[static_cast<size_t>(nearestPointIndex)].role);

    dragControlPlanePoint_ = worldPos;

    // カメラからターゲットへ向く方向 = 画面の法線
    const Vector3 cameraPos = usingCamera_->GetTransform().translate;
    dragControlPlaneNormal_ =
        NormalizeSafeV(Subtract(worldPos, cameraPos), {0.0f, 0.0f, 1.0f});

    Vector3 hitPoint{};
    if (RayPlaneIntersection(mouseRay, dragControlPlanePoint_,
                             dragControlPlaneNormal_, &hitPoint)) {
      dragControlPointOffset_ = Subtract(worldPos, hitPoint);
    } else {
      dragControlPointOffset_ = ZeroV();
    }

    return true;
  }

  const int controlOwnerId =
      ResolveControlOwnerPartId(assembly_, visiblePartId);

  if (controlOwnerId < 0) {
    return false;
  }

  if (modBodies_.count(controlOwnerId) == 0 ||
      modObjects_.count(controlOwnerId) == 0) {
    return false;
  }

  float nearestT = FLT_MAX;
  int nearestPointIndex = -1;

  Object *object = modObjects_[controlOwnerId].get();
  if (object == nullptr) {
    return false;
  }

  ModBody &body = modBodies_[controlOwnerId];
  const std::vector<ModControlPoint> &points = body.GetControlPoints();

  for (size_t pointIndex = 0; pointIndex < points.size(); ++pointIndex) {
    if (!(points[pointIndex].movable || points[pointIndex].isConnectionPoint)) {
      continue;
    }

    Sphere sphere{};
    sphere.center = body.GetControlPointWorldPosition(object, pointIndex);
    sphere.radius = points[pointIndex].radius;

    float t = 0.0f;
    if (!crashDecision(sphere, mouseRay, &t)) {
      continue;
    }

    if (t < nearestT) {
      nearestT = t;
      nearestPointIndex = static_cast<int>(pointIndex);
    }
  }

  if (nearestPointIndex >= 0 &&
      IsAssemblyDragPriorityControlPoint(controlOwnerId, nearestPointIndex)) {
    return false;
  }

  selectedControlPartId_ = controlOwnerId;
  selectedControlPointIndex_ = nearestPointIndex;

  selectedPartId_ = visiblePartId;
  reattachParentId_ = -1;
  reattachConnectorId_ = -1;

  const Vector3 worldPos =
      modBodies_[controlOwnerId].GetControlPointWorldPosition(
          object, static_cast<size_t>(nearestPointIndex));

  dragControlPlanePoint_ = worldPos;

  const Vector3 cameraPos = usingCamera_->GetTransform().translate;
  dragControlPlaneNormal_ =
      NormalizeSafeV(Subtract(worldPos, cameraPos), {0.0f, 0.0f, 1.0f});

  Vector3 hitPoint{};
  if (RayPlaneIntersection(mouseRay, dragControlPlanePoint_,
                           dragControlPlaneNormal_, &hitPoint)) {
    dragControlPointOffset_ = Subtract(worldPos, hitPoint);
  } else {
    dragControlPointOffset_ = ZeroV();
  }

  return true;
}

void ModScene::MoveSelectedControlPointFromMouseRay(const Ray &mouseRay) {
  if (selectedControlPointIndex_ < 0) {
    return;
  }

  Vector3 hitPoint{};
  if (!RayPlaneIntersection(mouseRay, dragControlPlanePoint_,
                            dragControlPlaneNormal_, &hitPoint)) {
    return;
  }

  const Vector3 targetWorld = Add(hitPoint, dragControlPointOffset_);

  if (selectedControlPartId_ == -2) {
    const int chestBodyId = assembly_.GetBodyId();
    if (chestBodyId < 0 || modObjects_.count(chestBodyId) == 0) {
      return;
    }

    Object *bodyObject = modObjects_[chestBodyId].get();
    if (bodyObject == nullptr) {
      return;
    }

    const Vector3 targetLocal =
        ModObjectUtil::TransformWorldPointToLocal(bodyObject, targetWorld);

    MoveTorsoControlPoint(static_cast<size_t>(selectedControlPointIndex_),
                          targetLocal);
    return;
  }

  if (selectedControlPartId_ < 0) {
    return;
  }

  if (modBodies_.count(selectedControlPartId_) == 0 ||
      modObjects_.count(selectedControlPartId_) == 0) {
    return;
  }

  ModBody &body = modBodies_[selectedControlPartId_];
  const std::vector<ModControlPoint> &points = body.GetControlPoints();

  if (selectedControlPointIndex_ >= static_cast<int>(points.size())) {
    return;
  }

  if (!points[static_cast<size_t>(selectedControlPointIndex_)].movable) {
    return;
  }

  Object *object = modObjects_[selectedControlPartId_].get();
  if (object == nullptr) {
    return;
  }

  const Vector3 targetLocal =
      ModObjectUtil::TransformWorldPointToLocal(object, targetWorld);

  body.MoveControlPoint(static_cast<size_t>(selectedControlPointIndex_),
                        targetLocal);
}

void ModScene::ClearControlPointSelection() {
  selectedControlPartId_ = -1;
  selectedControlPointIndex_ = -1;
  isDraggingControlPoint_ = false;
  dragControlPlanePoint_ = {0.0f, 0.0f, 0.0f};
  dragControlPlaneNormal_ = {0.0f, 0.0f, 1.0f};
  dragControlPointOffset_ = {0.0f, 0.0f, 0.0f};
  hoveredPartId_ = -1;
}

void ModScene::UpdateHoveredPartFromMouseRay(const Ray &mouseRay) {
  if (isDraggingControlPoint_ && selectedControlPartId_ >= 0) {
    hoveredPartId_ = selectedPartId_;
    return;
  }

  float nearestT = FLT_MAX;
  int nearestPartId = -1;

  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int partId = orderedPartIds_[i];

    ModSceneSegmentBoxSet boxSet{};
    if (!BuildPartPickBoxes(partId, boxSet)) {
      continue;
    }

    for (int bi = 0; bi < boxSet.count; ++bi) {
      float t = 0.0f;
      if (!IntersectRaySegmentBox(mouseRay, boxSet.segments[bi], &t)) {
        continue;
      }

      if (t < nearestT) {
        nearestT = t;
        nearestPartId = partId;
      }
    }
  }

  hoveredPartId_ = nearestPartId;
}

void ModScene::EnsureControlPointGizmoCount(size_t requiredCount) {
  while (controlPointGizmos_.size() < requiredCount) {
    std::unique_ptr<Object> gizmo = std::make_unique<Object>();
    gizmo->IntObject(system_);
    gizmo->CreateDefaultData();
    gizmo->modelHandle_ = config::default_Sphere_MeshBufferHandle_;

    // カメラ正面向きで見やすくする
    gizmo->isBillboard_ = true;

    if (!gizmo->objectParts_.empty()) {
      gizmo->objectParts_[0].materialConfig->textureHandle =
          controlPointGizmoTextureHandle_;
      gizmo->objectParts_[0].materialConfig->useModelTexture = false;
      gizmo->objectParts_[0].materialConfig->enableLighting = false;
      gizmo->objectParts_[0].materialConfig->textureColor =
          MakeColor(1.0f, 1.0f, 1.0f, 1.0f);
    }

    controlPointGizmos_.push_back(std::move(gizmo));
  }
}

void ModScene::UpdateControlPointGizmos() {
  activeControlPointGizmoCount_ = 0;

  int visiblePartId = -1;
  if (isDraggingControlPoint_ && selectedPartId_ >= 0) {
    visiblePartId = selectedPartId_;
  } else if (hoveredPartId_ >= 0) {
    visiblePartId = hoveredPartId_;
  } else if (selectedPartId_ >= 0) {
    visiblePartId = selectedPartId_;
  }

  if (IsTorsoVisiblePartId(visiblePartId)) {
    const int chestId = assembly_.GetBodyId();
    if (chestId >= 0) {
      visiblePartId = chestId;
    }
  }

  if (visiblePartId < 0) {
    return;
  }

  // torso 共有点表示
  if (IsTorsoVisiblePartId(visiblePartId)) {
    size_t visibleCount = 0;
    for (size_t i = 0; i < torsoControlPoints_.size(); ++i) {
      if (torsoControlPoints_[i].movable ||
          torsoControlPoints_[i].isConnectionPoint) {
        ++visibleCount;
      }
    }

    EnsureControlPointGizmoCount(visibleCount);

    const Vector3 cameraPos = usingCamera_->GetTransform().translate;
    size_t gizmoIndex = 0;

    for (size_t i = 0; i < torsoControlPoints_.size(); ++i) {
      if (!(torsoControlPoints_[i].movable ||
            torsoControlPoints_[i].isConnectionPoint)) {
        continue;
      }

      Object *gizmo = controlPointGizmos_[gizmoIndex].get();
      if (gizmo == nullptr || gizmo->objectParts_.empty()) {
        continue;
      }

      const bool isSelected =
          (selectedControlPartId_ == -2 &&
           selectedControlPointIndex_ == static_cast<int>(i));

      const Vector3 worldPos =
          GetTorsoControlPointWorldPosition(torsoControlPoints_[i].role);
      const float influenceRadius = torsoControlPoints_[i].radius;
      const float drawRadius = GetControlPointGizmoDrawRadius(influenceRadius);

      const Vector3 toCamera =
          NormalizeSafeV(Subtract(cameraPos, worldPos), {0.0f, 0.0f, -1.0f});
      const Vector3 drawPos =
          Add(worldPos, Multiply(drawRadius * 0.75f, toCamera));

      gizmo->mainPosition.transform.translate = drawPos;
      gizmo->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
      gizmo->mainPosition.transform.scale = {
          drawRadius * 2.0f, drawRadius * 2.0f, drawRadius * 2.0f};

      gizmo->objectParts_[0].materialConfig->useModelTexture = false;
      gizmo->objectParts_[0].materialConfig->enableLighting = false;

      if (isSelected) {
        gizmo->objectParts_[0].materialConfig->textureColor =
            MakeColor(1.0f, 0.45f, 0.2f, 1.0f);
      } else if (torsoControlPoints_[i].isConnectionPoint) {
        gizmo->objectParts_[0].materialConfig->textureColor =
            MakeColor(1.0f, 0.9f, 0.2f, 1.0f);
      } else {
        gizmo->objectParts_[0].materialConfig->textureColor =
            MakeColor(0.35f, 0.9f, 1.0f, 1.0f);
      }

      gizmo->Update(usingCamera_);
      ++gizmoIndex;
    }

    activeControlPointGizmoCount_ = gizmoIndex;
    return;
  }

  const int controlOwnerId =
      ResolveControlOwnerPartId(assembly_, visiblePartId);

  if (controlOwnerId < 0) {
    return;
  }

  if (modBodies_.count(controlOwnerId) == 0 ||
      modObjects_.count(controlOwnerId) == 0) {
    return;
  }

  Object *partObject = modObjects_[controlOwnerId].get();
  if (partObject == nullptr) {
    return;
  }

  ModBody &body = modBodies_[controlOwnerId];
  const std::vector<ModControlPoint> &points = body.GetControlPoints();

  size_t visibleCount = 0;
  for (size_t i = 0; i < points.size(); ++i) {
    if (points[i].movable || points[i].isConnectionPoint) {
      ++visibleCount;
    }
  }

  EnsureControlPointGizmoCount(visibleCount);

  const Vector3 cameraPos = usingCamera_->GetTransform().translate;

  size_t gizmoIndex = 0;
  for (size_t i = 0; i < points.size(); ++i) {
    if (!(points[i].movable || points[i].isConnectionPoint)) {
      continue;
    }

    Object *gizmo = controlPointGizmos_[gizmoIndex].get();
    if (gizmo == nullptr || gizmo->objectParts_.empty()) {
      continue;
    }

    const bool isSelected = (selectedControlPartId_ == controlOwnerId &&
                             selectedControlPointIndex_ == static_cast<int>(i));

    const Vector3 worldPos = body.GetControlPointWorldPosition(partObject, i);
    const float influenceRadius = points[i].radius;
    const float drawRadius = GetControlPointGizmoDrawRadius(influenceRadius);

    const Vector3 toCamera =
        NormalizeSafeV(Subtract(cameraPos, worldPos), {0.0f, 0.0f, -1.0f});
    const Vector3 drawPos =
        Add(worldPos, Multiply(drawRadius * 0.75f, toCamera));

    gizmo->mainPosition.transform.translate = drawPos;
    gizmo->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
    gizmo->mainPosition.transform.scale = {drawRadius * 2.0f, drawRadius * 2.0f,
                                           drawRadius * 2.0f};

    gizmo->objectParts_[0].materialConfig->useModelTexture = false;
    gizmo->objectParts_[0].materialConfig->enableLighting = false;

    if (isSelected) {
      gizmo->objectParts_[0].materialConfig->textureColor =
          MakeColor(1.0f, 0.45f, 0.2f, 1.0f);
    } else if (points[i].isConnectionPoint) {
      gizmo->objectParts_[0].materialConfig->textureColor =
          MakeColor(1.0f, 0.9f, 0.2f, 1.0f);
    } else {
      gizmo->objectParts_[0].materialConfig->textureColor =
          MakeColor(0.35f, 0.9f, 1.0f, 1.0f);
    }

    gizmo->Update(usingCamera_);
    ++gizmoIndex;
  }

  activeControlPointGizmoCount_ = gizmoIndex;
}

void ModScene::DrawControlPointGizmos() {
  for (size_t i = 0; i < activeControlPointGizmoCount_; ++i) {
    Object *gizmo = controlPointGizmos_[i].get();
    if (gizmo != nullptr) {
      gizmo->Draw();
    }
  }
}

void ModScene::ResetTorsoControlPoints() {
  torsoControlPoints_.clear();

  // 新モデル寸法に合わせる
  torsoChestToBellyLength_ = 1.2796f;
  torsoBellyToWaistLength_ = 1.6880f;

  TorsoControlPoint chest{};
  chest.role = ModControlPointRole::Chest;
  chest.localPosition = {0.0f, 1.2796f, 0.0f};
  chest.radius = 0.12f;
  chest.movable = true;
  chest.isConnectionPoint = true;
  chest.acceptsParent = false;
  chest.acceptsChild = true;
  torsoControlPoints_.push_back(chest);

  TorsoControlPoint belly{};
  belly.role = ModControlPointRole::Belly;
  belly.localPosition = {0.0f, 0.0f, 0.0f};
  belly.radius = 0.10f;
  belly.movable = true;
  belly.isConnectionPoint = true;
  belly.acceptsParent = true;
  belly.acceptsChild = true;
  torsoControlPoints_.push_back(belly);

  TorsoControlPoint waist{};
  waist.role = ModControlPointRole::Waist;
  waist.localPosition = {0.0f, -1.6880f, 0.0f};
  waist.radius = 0.12f;
  waist.movable = true;
  waist.isConnectionPoint = true;
  waist.acceptsParent = true;
  waist.acceptsChild = true;
  torsoControlPoints_.push_back(waist);
}

int ModScene::FindTorsoControlPointIndex(ModControlPointRole role) const {
  for (size_t i = 0; i < torsoControlPoints_.size(); ++i) {
    if (torsoControlPoints_[i].role == role) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

Vector3 ModScene::ResolveDynamicAttachBase(const PartNode &parentNode,
                                           const PartNode &childNode) const {
  const Vector3 defaultAttach = assembly_.GetDefaultAttachLocal(
      parentNode.part, childNode.part, childNode.side);

  // ------------------------------------------------------------
  // torso 親
  // ------------------------------------------------------------
  if (parentNode.part == ModBodyPart::ChestBody ||
      parentNode.part == ModBodyPart::StomachBody) {
    const int chestIndex =
        FindTorsoControlPointIndex(ModControlPointRole::Chest);
    const int bellyIndex =
        FindTorsoControlPointIndex(ModControlPointRole::Belly);
    const int waistIndex =
        FindTorsoControlPointIndex(ModControlPointRole::Waist);

    const Vector3 defaultChest = {0.0f, 1.2796f, 0.0f};
    const Vector3 defaultBelly = {0.0f, 0.0f, 0.0f};
    const Vector3 defaultWaist = {0.0f, -1.6880f, 0.0f};

    const Vector3 currentChest =
        (chestIndex >= 0)
            ? torsoControlPoints_[static_cast<size_t>(chestIndex)].localPosition
            : defaultChest;

    const Vector3 currentBelly =
        (bellyIndex >= 0)
            ? torsoControlPoints_[static_cast<size_t>(bellyIndex)].localPosition
            : defaultBelly;

    const Vector3 currentWaist =
        (waistIndex >= 0)
            ? torsoControlPoints_[static_cast<size_t>(waistIndex)].localPosition
            : defaultWaist;

    const float defaultChestRadius = 0.12f;
    const float defaultBellyRadius = 0.10f;
    const float defaultWaistRadius = 0.12f;

    const float currentChestRadius = GetTorsoControlPointRadius(
        ModControlPointRole::Chest, defaultChestRadius);

    const float currentBellyRadius = GetTorsoControlPointRadius(
        ModControlPointRole::Belly, defaultBellyRadius);

    const float currentWaistRadius = GetTorsoControlPointRadius(
        ModControlPointRole::Waist, defaultWaistRadius);

    const float defaultUpperTorsoRadius =
        (std::max)(defaultChestRadius, defaultBellyRadius);
    const float defaultLowerTorsoRadius =
        (std::max)(defaultBellyRadius, defaultWaistRadius);

    const float currentUpperTorsoRadius =
        (std::max)(currentChestRadius, currentBellyRadius);
    const float currentLowerTorsoRadius =
        (std::max)(currentBellyRadius, currentWaistRadius);

    const float upperTorsoRadiusRatio =
        currentUpperTorsoRadius / (std::max)(defaultUpperTorsoRadius, 0.0001f);

    const float lowerTorsoRadiusRatio =
        currentLowerTorsoRadius / (std::max)(defaultLowerTorsoRadius, 0.0001f);

    const float chestRadiusRatio =
        currentChestRadius / (std::max)(defaultChestRadius, 0.0001f);

    const float waistRadiusRatio =
        currentWaistRadius / (std::max)(defaultWaistRadius, 0.0001f);

    // 新モデル基準の接続配置
    // 腕  : Chest と同じ高さ、少し外
    // 脚  : Waist より少し下
    // 首  : Chest より少し上
    const float baseShoulderX = 1.34f;
    const float baseHipX = 0.58f;
    const float neckLift = 0.08f;
    const float legDrop = 0.10f;

    switch (childNode.part) {
    case ModBodyPart::Neck:
    case ModBodyPart::Head: {
      Vector3 attach = currentChest;
      attach.y +=
          neckLift + (currentUpperTorsoRadius - defaultUpperTorsoRadius);
      return attach;
    }

    case ModBodyPart::LeftUpperArm: {
      Vector3 attach = currentChest;
      attach.x -= baseShoulderX * upperTorsoRadiusRatio;
      attach.y = currentChest.y;
      attach.z = 0.0f;
      return attach;
    }

    case ModBodyPart::RightUpperArm: {
      Vector3 attach = currentChest;
      attach.x += baseShoulderX * upperTorsoRadiusRatio;
      attach.y = currentChest.y;
      attach.z = 0.0f;
      return attach;
    }

    case ModBodyPart::LeftThigh: {
      Vector3 attach = currentWaist;
      attach.x -= baseHipX * lowerTorsoRadiusRatio;
      attach.y -= legDrop * waistRadiusRatio;
      attach.z = 0.0f;
      return attach;
    }

    case ModBodyPart::RightThigh: {
      Vector3 attach = currentWaist;
      attach.x += baseHipX * lowerTorsoRadiusRatio;
      attach.y -= legDrop * waistRadiusRatio;
      attach.z = 0.0f;
      return attach;
    }

    default:
      return defaultAttach;
    }
  }

  // ------------------------------------------------------------
  // UpperArm 親 -> ForeArm 子
  // Bend 点を基準にする
  // ------------------------------------------------------------
  if (parentNode.part == ModBodyPart::LeftUpperArm ||
      parentNode.part == ModBodyPart::RightUpperArm) {
    if (childNode.part == ModBodyPart::LeftForeArm ||
        childNode.part == ModBodyPart::RightForeArm) {
      auto it = modBodies_.find(parentNode.id);
      if (it != modBodies_.end()) {
        const int bendIndex =
            it->second.FindControlPointIndex(ModControlPointRole::Bend);
        const int endIndex =
            it->second.FindControlPointIndex(ModControlPointRole::End);

        if (bendIndex >= 0) {
          const std::vector<ModControlPoint> &points =
              it->second.GetControlPoints();

          const Vector3 bendPos =
              points[static_cast<size_t>(bendIndex)].localPosition;
          const float defaultBendRadius = 0.09f;
          const float currentBendRadius =
              points[static_cast<size_t>(bendIndex)].radius;
          const float extraBendRadius =
              (std::max)(0.0f, currentBendRadius - defaultBendRadius);

          Vector3 outward = {0.0f, -1.0f, 0.0f};
          if (endIndex >= 0) {
            const Vector3 bendToEnd = Subtract(
                points[static_cast<size_t>(endIndex)].localPosition, bendPos);
            if (Length(bendToEnd) > 0.0001f) {
              outward = NormalizeSafeV(bendToEnd, outward);
            }
          }

          return Add(bendPos, Multiply(extraBendRadius, outward));
        }
      }
      return defaultAttach;
    }
  }

  // ------------------------------------------------------------
  // Thigh 親 -> Shin 子
  // Bend 点を基準にする
  // ------------------------------------------------------------
  if (parentNode.part == ModBodyPart::LeftThigh ||
      parentNode.part == ModBodyPart::RightThigh) {
    if (childNode.part == ModBodyPart::LeftShin ||
        childNode.part == ModBodyPart::RightShin) {
      auto it = modBodies_.find(parentNode.id);
      if (it != modBodies_.end()) {
        const int bendIndex =
            it->second.FindControlPointIndex(ModControlPointRole::Bend);
        const int endIndex =
            it->second.FindControlPointIndex(ModControlPointRole::End);

        if (bendIndex >= 0) {
          const std::vector<ModControlPoint> &points =
              it->second.GetControlPoints();

          const Vector3 bendPos =
              points[static_cast<size_t>(bendIndex)].localPosition;
          const float defaultBendRadius = 0.10f;
          const float currentBendRadius =
              points[static_cast<size_t>(bendIndex)].radius;
          const float extraBendRadius =
              (std::max)(0.0f, currentBendRadius - defaultBendRadius);

          Vector3 outward = {0.0f, -1.0f, 0.0f};
          if (endIndex >= 0) {
            const Vector3 bendToEnd = Subtract(
                points[static_cast<size_t>(endIndex)].localPosition, bendPos);
            if (Length(bendToEnd) > 0.0001f) {
              outward = NormalizeSafeV(bendToEnd, outward);
            }
          }

          return Add(bendPos, Multiply(extraBendRadius, outward));
        }
      }
      return defaultAttach;
    }
  }

  // ------------------------------------------------------------
  // Neck 親 -> Head 子
  // Head の LowerNeck を基準にしつつ、UpperNeck 側の変化も追従させる
  // ------------------------------------------------------------
  if (parentNode.part == ModBodyPart::Neck &&
      childNode.part == ModBodyPart::Head) {
    auto it = modBodies_.find(parentNode.id);
    if (it != modBodies_.end()) {
      const int bendIndex =
          it->second.FindControlPointIndex(ModControlPointRole::Bend);
      const int endIndex =
          it->second.FindControlPointIndex(ModControlPointRole::End);

      if (bendIndex >= 0) {
        const std::vector<ModControlPoint> &points =
            it->second.GetControlPoints();

        const Vector3 bendPos =
            points[static_cast<size_t>(bendIndex)].localPosition;

        const float defaultBendRadius = 0.08f;
        const float currentBendRadius =
            points[static_cast<size_t>(bendIndex)].radius;
        const float extraBendRadius =
            (std::max)(0.0f, currentBendRadius - defaultBendRadius);

        Vector3 outward = {0.0f, 1.0f, 0.0f};
        if (endIndex >= 0) {
          const Vector3 bendToEnd = Subtract(
              points[static_cast<size_t>(endIndex)].localPosition, bendPos);
          if (Length(bendToEnd) > 0.0001f) {
            outward = NormalizeSafeV(bendToEnd, outward);
          }
        }

        return Add(bendPos, Multiply(extraBendRadius, outward));
      }
    }
    return defaultAttach;
  }

  // ------------------------------------------------------------
  // Head 親
  // ------------------------------------------------------------
  if (parentNode.part == ModBodyPart::Head) {
    const float defaultHeadRadius = 0.11f;
    const float currentHeadRadius = GetPartControlPointRadius(
        parentNode.id, ModControlPointRole::End, defaultHeadRadius);

    const float headRadiusRatio =
        currentHeadRadius / (std::max)(defaultHeadRadius, 0.0001f);

    return ScaleVectorComponents(
        defaultAttach, {headRadiusRatio, headRadiusRatio, headRadiusRatio});
  }

  // ------------------------------------------------------------
  // その他
  // ------------------------------------------------------------
  if (modBodies_.count(parentNode.id) > 0) {
    const Vector3 parentScale =
        modBodies_.at(parentNode.id).GetVisualScaleRatio();
    return ScaleVectorComponents(defaultAttach, parentScale);
  }

  return defaultAttach;
}

float ModScene::GetTorsoControlPointRadius(ModControlPointRole role,
                                           float defaultRadius) const {
  const int index = FindTorsoControlPointIndex(role);
  if (index < 0) {
    return defaultRadius;
  }

  return torsoControlPoints_[static_cast<size_t>(index)].radius;
}

float ModScene::GetPartControlPointRadius(int partId, ModControlPointRole role,
                                          float defaultRadius) const {
  auto it = modBodies_.find(partId);
  if (it == modBodies_.end()) {
    return defaultRadius;
  }

  const int pointIndex = it->second.FindControlPointIndex(role);
  if (pointIndex < 0) {
    return defaultRadius;
  }

  const std::vector<ModControlPoint> &points = it->second.GetControlPoints();
  if (static_cast<size_t>(pointIndex) >= points.size()) {
    return defaultRadius;
  }

  return points[static_cast<size_t>(pointIndex)].radius;
}

float ModScene::GetPartVisualSegmentRadius(int partId,
                                           ModControlPointRole startRole,
                                           ModControlPointRole endRole) const {
  auto it = modBodies_.find(partId);
  if (it == modBodies_.end()) {
    return 0.0f;
  }

  return it->second.GetVisualSegmentRadius(startRole, endRole);
}

float ModScene::GetTorsoVisualSegmentRadius(ModControlPointRole startRole,
                                            ModControlPointRole endRole) const {
  const int startIndex = FindTorsoControlPointIndex(startRole);
  const int endIndex = FindTorsoControlPointIndex(endRole);

  if (startIndex < 0 || endIndex < 0) {
    return 0.0f;
  }

  const float startRadius =
      torsoControlPoints_[static_cast<size_t>(startIndex)].radius;
  const float endRadius =
      torsoControlPoints_[static_cast<size_t>(endIndex)].radius;

  const float safeStartRadius = (std::max)(startRadius, 0.01f);
  const float safeEndRadius = (std::max)(endRadius, 0.01f);

  const float segmentRadius = (std::max)(safeStartRadius, safeEndRadius);

  float defaultSegmentRadius = 0.10f;
  if (startRole == ModControlPointRole::Chest &&
      endRole == ModControlPointRole::Belly) {
    defaultSegmentRadius = (0.12f + 0.10f) * 0.5f;
  } else if (startRole == ModControlPointRole::Belly &&
             endRole == ModControlPointRole::Waist) {
    defaultSegmentRadius = (0.10f + 0.12f) * 0.5f;
  }

  const float thicknessScale =
      segmentRadius / (std::max)(defaultSegmentRadius, 0.0001f);

  // torso は個別 ModBody の param.scale
  // ではなく共有点半径で主に太さが決まっているので、 まずは見た目側と同じ
  // segmentRadius ベースで合わせる
  return defaultSegmentRadius * thicknessScale;
}

Vector3 ModScene::ScaleVectorComponents(const Vector3 &value,
                                        const Vector3 &scale) const {
  return {
      value.x * scale.x,
      value.y * scale.y,
      value.z * scale.z,
  };
}

Vector3
ModScene::ResolveAttachedLocalTranslate(const PartNode &childNode) const {
  if (childNode.parentId < 0) {
    return childNode.localTransform.translate;
  }

  const PartNode *parentNode = assembly_.FindNode(childNode.parentId);
  if (parentNode == nullptr) {
    return childNode.localTransform.translate;
  }

  const Vector3 defaultAttach = assembly_.GetDefaultAttachLocal(
      parentNode->part, childNode.part, childNode.side);

  const Vector3 dynamicBase = ResolveDynamicAttachBase(*parentNode, childNode);

  // 保存してある localTransform.translate はデフォルト接続基準からの編集差分
  const Vector3 offsetFromDefault =
      Subtract(childNode.localTransform.translate, defaultAttach);

  // 子自身の太さ・長さ増加ぶんだけ、親から外へ押し出す
  const Vector3 childSelfOffset = ResolveChildSelfAttachOffset(childNode);

  return Add(Add(dynamicBase, offsetFromDefault), childSelfOffset);
}

void ModScene::UpdateControlPointWheelScaling() {
  if (usingCamera_ == nullptr) {
    return;
  }

  // 中ボタン押し中はデバッグカメラ操作を優先する
  if (system_->GetMouseIsPush(2)) {
    return;
  }

  const int wheelDelta = system_->GetMouseScrollOrigin();
  if (wheelDelta == 0) {
    return;
  }

  const float scaleFactor = GetWheelScaleFactorFromDelta(wheelDelta);
  if (scaleFactor == 1.0f) {
    return;
  }

  // すでに選択中の操作点があるなら最優先でそれを拡縮する
  if (selectedControlPointIndex_ >= 0) {
    if (selectedControlPartId_ == -2) {
      ScaleTorsoControlPoint(static_cast<size_t>(selectedControlPointIndex_),
                             scaleFactor);
      return;
    }

    if (selectedControlPartId_ >= 0 &&
        modBodies_.count(selectedControlPartId_) > 0) {
      modBodies_[selectedControlPartId_].ScaleControlPoint(
          static_cast<size_t>(selectedControlPointIndex_), scaleFactor);
      return;
    }
  }

  // 未選択なら、hover中または選択中の部位を基準に
  // マウス Ray に最も近い操作点を見つけてその場で拡縮する
  const int visiblePartId =
      (hoveredPartId_ >= 0) ? hoveredPartId_ : selectedPartId_;

  if (visiblePartId < 0) {
    return;
  }

  Ray mouseRay = usingCamera_->ScreenPointToRay(system_->GetMousePosVector2());

  // 胴体共有点
  if (IsTorsoVisiblePartId(visiblePartId)) {
    float nearestT = FLT_MAX;
    int nearestPointIndex = -1;

    for (size_t i = 0; i < torsoControlPoints_.size(); ++i) {
      if (!(torsoControlPoints_[i].movable ||
            torsoControlPoints_[i].isConnectionPoint)) {
        continue;
      }

      Sphere sphere{};
      sphere.center =
          GetTorsoControlPointWorldPosition(torsoControlPoints_[i].role);
      sphere.radius = torsoControlPoints_[i].radius;

      float t = 0.0f;
      if (!crashDecision(sphere, mouseRay, &t)) {
        continue;
      }

      if (t < nearestT) {
        nearestT = t;
        nearestPointIndex = static_cast<int>(i);
      }
    }

    if (nearestPointIndex >= 0) {
      selectedControlPartId_ = -2;
      selectedControlPointIndex_ = nearestPointIndex;
      selectedPartId_ = visiblePartId;
      ScaleTorsoControlPoint(static_cast<size_t>(nearestPointIndex),
                             scaleFactor);
    }

    return;
  }

  const int controlOwnerId =
      ResolveControlOwnerPartId(assembly_, visiblePartId);

  if (controlOwnerId < 0) {
    return;
  }

  if (modBodies_.count(controlOwnerId) == 0 ||
      modObjects_.count(controlOwnerId) == 0) {
    return;
  }

  Object *object = modObjects_[controlOwnerId].get();
  if (object == nullptr) {
    return;
  }

  ModBody &body = modBodies_[controlOwnerId];
  const std::vector<ModControlPoint> &points = body.GetControlPoints();

  float nearestT = FLT_MAX;
  int nearestPointIndex = -1;

  for (size_t i = 0; i < points.size(); ++i) {
    if (!(points[i].movable || points[i].isConnectionPoint)) {
      continue;
    }

    Sphere sphere{};
    sphere.center = body.GetControlPointWorldPosition(object, i);
    sphere.radius = points[i].radius;

    float t = 0.0f;
    if (!crashDecision(sphere, mouseRay, &t)) {
      continue;
    }

    if (t < nearestT) {
      nearestT = t;
      nearestPointIndex = static_cast<int>(i);
    }
  }

  if (nearestPointIndex >= 0) {
    selectedControlPartId_ = controlOwnerId;
    selectedControlPointIndex_ = nearestPointIndex;
    selectedPartId_ = visiblePartId;

    body.ScaleControlPoint(static_cast<size_t>(nearestPointIndex), scaleFactor);
  }
}

bool ModScene::MoveTorsoControlPoint(size_t index,
                                     const Vector3 &newLocalPosition) {
  if (index >= torsoControlPoints_.size()) {
    return false;
  }

  if (!torsoControlPoints_[index].movable) {
    return false;
  }

  const int chestIndex = FindTorsoControlPointIndex(ModControlPointRole::Chest);
  const int bellyIndex = FindTorsoControlPointIndex(ModControlPointRole::Belly);
  const int waistIndex = FindTorsoControlPointIndex(ModControlPointRole::Waist);

  if (chestIndex < 0 || bellyIndex < 0 || waistIndex < 0) {
    return false;
  }

  Vector3 chestPos =
      torsoControlPoints_[static_cast<size_t>(chestIndex)].localPosition;
  Vector3 bellyPos =
      torsoControlPoints_[static_cast<size_t>(bellyIndex)].localPosition;
  Vector3 waistPos =
      torsoControlPoints_[static_cast<size_t>(waistIndex)].localPosition;

  const float baseChestBellyLength = 1.2796f;
  const float baseBellyWaistLength = 1.6880f;

  float chestBellyMin = 0.0f;
  float chestBellyMax = 0.0f;
  float bellyWaistMin = 0.0f;
  float bellyWaistMax = 0.0f;

  MakeEditRangeFromBase(baseChestBellyLength, &chestBellyMin, &chestBellyMax);
  MakeEditRangeFromBase(baseBellyWaistLength, &bellyWaistMin, &bellyWaistMax);

  if (index == static_cast<size_t>(chestIndex)) {
    Vector3 candidate = newLocalPosition;

    candidate = ClampDistance(bellyPos, candidate, chestBellyMin, chestBellyMax,
                              {0.0f, 1.0f, 0.0f});

    if (candidate.y <= bellyPos.y + 0.05f) {
      candidate.y = bellyPos.y + 0.05f;
      candidate = ClampDistance(bellyPos, candidate, chestBellyMin,
                                chestBellyMax, {0.0f, 1.0f, 0.0f});
    }

    torsoControlPoints_[index].localPosition = candidate;
    torsoChestToBellyLength_ = Length(Subtract(candidate, bellyPos));
    return true;
  }

  if (index == static_cast<size_t>(bellyIndex)) {
    Vector3 candidate = newLocalPosition;

    candidate = ClampDistance(chestPos, candidate, chestBellyMin, chestBellyMax,
                              {0.0f, -1.0f, 0.0f});

    candidate = ClampDistance(waistPos, candidate, bellyWaistMin, bellyWaistMax,
                              {0.0f, 1.0f, 0.0f});

    if (candidate.y >= chestPos.y - 0.05f) {
      candidate.y = chestPos.y - 0.05f;
    }
    if (candidate.y <= waistPos.y + 0.05f) {
      candidate.y = waistPos.y + 0.05f;
    }

    candidate = ClampDistance(chestPos, candidate, chestBellyMin, chestBellyMax,
                              {0.0f, -1.0f, 0.0f});
    candidate = ClampDistance(waistPos, candidate, bellyWaistMin, bellyWaistMax,
                              {0.0f, 1.0f, 0.0f});

    torsoControlPoints_[index].localPosition = candidate;
    torsoChestToBellyLength_ = Length(Subtract(chestPos, candidate));
    torsoBellyToWaistLength_ = Length(Subtract(candidate, waistPos));
    return true;
  }

  if (index == static_cast<size_t>(waistIndex)) {
    Vector3 candidate = newLocalPosition;

    candidate = ClampDistance(bellyPos, candidate, bellyWaistMin, bellyWaistMax,
                              {0.0f, -1.0f, 0.0f});

    if (candidate.y >= bellyPos.y - 0.05f) {
      candidate.y = bellyPos.y - 0.05f;
      candidate = ClampDistance(bellyPos, candidate, bellyWaistMin,
                                bellyWaistMax, {0.0f, -1.0f, 0.0f});
    }

    torsoControlPoints_[index].localPosition = candidate;
    torsoBellyToWaistLength_ = Length(Subtract(bellyPos, candidate));
    return true;
  }

  return false;
}

bool ModScene::ScaleTorsoControlPoint(size_t index, float scaleFactor) {
  if (index >= torsoControlPoints_.size()) {
    return false;
  }

  if (scaleFactor <= 0.0f) {
    return false;
  }

  const ModControlPointRole role = torsoControlPoints_[index].role;

  float defaultRadius = 0.10f;
  switch (role) {
  case ModControlPointRole::Chest:
    defaultRadius = 0.12f;
    break;
  case ModControlPointRole::Belly:
    defaultRadius = 0.10f;
    break;
  case ModControlPointRole::Waist:
    defaultRadius = 0.12f;
    break;
  default:
    defaultRadius = 0.10f;
    break;
  }

  float minRadius = 0.0f;
  float maxRadius = 0.0f;
  MakeEditRangeFromBase(defaultRadius, &minRadius, &maxRadius);

  float newRadius = torsoControlPoints_[index].radius * scaleFactor;
  newRadius = ClampFloatLocal(newRadius, minRadius, maxRadius);

  torsoControlPoints_[index].radius = newRadius;
  return true;
}

bool ModScene::BuildPartPickBoxes(int partId,
                                  ModSceneSegmentBoxSet &outBoxes) const {
  outBoxes.count = 0;

  const PartNode *node = assembly_.FindNode(partId);
  if (node == nullptr) {
    return false;
  }

  if (node->part == ModBodyPart::Head) {
    return BuildHeadPickBox(partId, outBoxes);
  }

  auto pushBox = [&](const Vector3 &start, const Vector3 &end, float halfWidth,
                     float halfDepth) {
    if (outBoxes.count >= 2) {
      return;
    }

    ModSceneSegmentBox &box = outBoxes.segments[outBoxes.count];

    const Vector3 segment = Subtract(end, start);
    const float segmentLength = Length(segment);

    Vector3 axisY = {0.0f, 1.0f, 0.0f};
    if (segmentLength > 0.0001f) {
      axisY = NormalizeSafeV(segment, axisY);
    }

    Vector3 fallbackForward = {0.0f, 0.0f, 1.0f};
    Vector3 axisX = CrossLocal(fallbackForward, axisY);

    if (Length(axisX) <= 0.0001f) {
      fallbackForward = {1.0f, 0.0f, 0.0f};
      axisX = CrossLocal(fallbackForward, axisY);
    }

    axisX = NormalizeSafeV(axisX, {1.0f, 0.0f, 0.0f});
    Vector3 axisZ =
        NormalizeSafeV(CrossLocal(axisY, axisX), {0.0f, 0.0f, 1.0f});

    box.center = Multiply(0.5f, Add(start, end));
    box.axisX = axisX;
    box.axisY = axisY;
    box.axisZ = axisZ;
    box.halfWidth = (std::max)(halfWidth, 0.01f);
    box.halfDepth = (std::max)(halfDepth, 0.01f);
    box.halfLength = (std::max)(segmentLength * 0.5f, 0.01f);

    ++outBoxes.count;
  };

  if (node->part == ModBodyPart::ChestBody ||
      node->part == ModBodyPart::StomachBody) {
    const int chestIndex =
        FindTorsoControlPointIndex(ModControlPointRole::Chest);
    const int bellyIndex =
        FindTorsoControlPointIndex(ModControlPointRole::Belly);
    const int waistIndex =
        FindTorsoControlPointIndex(ModControlPointRole::Waist);

    if (chestIndex < 0 || bellyIndex < 0 || waistIndex < 0) {
      return false;
    }

    const Vector3 chest =
        GetTorsoControlPointWorldPosition(ModControlPointRole::Chest);
    const Vector3 belly =
        GetTorsoControlPointWorldPosition(ModControlPointRole::Belly);
    const Vector3 waist =
        GetTorsoControlPointWorldPosition(ModControlPointRole::Waist);

    const float chestHalfWidth =
        GetModelLocalVisualRadius(ModBodyPart::ChestBody);
    const float stomachHalfWidth =
        GetModelLocalVisualRadius(ModBodyPart::StomachBody);

    pushBox(chest, belly, chestHalfWidth, chestHalfWidth);
    pushBox(belly, waist, stomachHalfWidth, stomachHalfWidth);

    return outBoxes.count > 0;
  }

  const int ownerId = ResolveControlOwnerPartId(assembly_, partId);
  if (ownerId < 0) {
    return false;
  }

  if (modBodies_.count(ownerId) == 0 || modObjects_.count(ownerId) == 0) {
    return false;
  }

  const ModBody &body = modBodies_.at(ownerId);
  const Object *object = modObjects_.at(ownerId).get();
  if (object == nullptr) {
    return false;
  }

  const int rootIndex = body.FindControlPointIndex(ModControlPointRole::Root);
  const int bendIndex = body.FindControlPointIndex(ModControlPointRole::Bend);
  const int endIndex = body.FindControlPointIndex(ModControlPointRole::End);

  auto worldAt = [&](int index) -> Vector3 {
    return body.GetControlPointWorldPosition(object,
                                             static_cast<size_t>(index));
  };

  auto pushSegmentBox = [&](int startIndex, int endIndex) {
    if (startIndex < 0 || endIndex < 0) {
      return;
    }

    const Vector3 startWorld = worldAt(startIndex);
    const Vector3 endWorld = worldAt(endIndex);

    const float halfWidth = GetModelLocalVisualRadius(node->part);
    const float halfDepth = halfWidth;

    pushBox(startWorld, endWorld, halfWidth, halfDepth);
  };

  switch (node->part) {
  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::RightUpperArm:
  case ModBodyPart::LeftThigh:
  case ModBodyPart::RightThigh:
  case ModBodyPart::Neck:
    pushSegmentBox(rootIndex, bendIndex);
    break;

  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
  case ModBodyPart::Head:
    pushSegmentBox(bendIndex, endIndex);
    break;

  default:
    break;
  }

  return outBoxes.count > 0;
}

bool ModScene::IsMouseRayInsideSelectedControlMesh(const Ray &mouseRay) const {
  if (selectedControlPointIndex_ < 0) {
    return false;
  }

  if (selectedControlPartId_ == -2) {
    for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
      const int partId = orderedPartIds_[i];
      if (!IsTorsoVisiblePartId(partId)) {
        continue;
      }

      ModSceneSegmentBoxSet boxSet{};
      if (!BuildPartPickBoxes(partId, boxSet)) {
        continue;
      }

      for (int bi = 0; bi < boxSet.count; ++bi) {
        float t = 0.0f;
        if (IntersectRaySegmentBox(mouseRay, boxSet.segments[bi], &t)) {
          return true;
        }
      }
    }

    return false;
  }

  if (selectedPartId_ < 0) {
    return false;
  }

  ModSceneSegmentBoxSet boxSet{};
  if (!BuildPartPickBoxes(selectedPartId_, boxSet)) {
    return false;
  }

  for (int bi = 0; bi < boxSet.count; ++bi) {
    float t = 0.0f;
    if (IntersectRaySegmentBox(mouseRay, boxSet.segments[bi], &t)) {
      return true;
    }
  }

  return false;
}

int ModScene::ResolveFadeGroupId(int partId) const {
  const PartNode *node = assembly_.FindNode(partId);
  if (node == nullptr) {
    return -1;
  }

  // 胴体は Chest / Stomach を同じグループとして扱う
  if (node->part == ModBodyPart::ChestBody ||
      node->part == ModBodyPart::StomachBody) {
    const int chestId = assembly_.GetBodyId();
    if (chestId >= 0) {
      return chestId;
    }
    return partId;
  }

  return ResolveControlOwnerPartId(assembly_, partId);
}

Vector3
ModScene::ResolveAttachOutwardDirection(const PartNode &childNode) const {
  switch (childNode.part) {
  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::LeftForeArm:
    return NormalizeSafeV({-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f});

  case ModBodyPart::RightUpperArm:
  case ModBodyPart::RightForeArm:
    return NormalizeSafeV({1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f});

  case ModBodyPart::LeftThigh:
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightThigh:
  case ModBodyPart::RightShin:
    return NormalizeSafeV({0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f});

  case ModBodyPart::Neck:
  case ModBodyPart::Head:
    return NormalizeSafeV({0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

  case ModBodyPart::ChestBody:
  case ModBodyPart::StomachBody:
  case ModBodyPart::Count:
  default:
    break;
  }

  if (childNode.localTransform.translate.x < 0.0f) {
    return {-1.0f, 0.0f, 0.0f};
  }
  if (childNode.localTransform.translate.x > 0.0f) {
    return {1.0f, 0.0f, 0.0f};
  }
  if (childNode.localTransform.translate.y < 0.0f) {
    return {0.0f, -1.0f, 0.0f};
  }
  return {0.0f, 1.0f, 0.0f};
}

float ModScene::GetChildDefaultAttachRadius(ModBodyPart part) const {
  switch (part) {
  case ModBodyPart::Neck:
    return 0.09f;

  case ModBodyPart::Head:
    return 0.08f;

  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::RightUpperArm:
  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
    return 0.09f;

  case ModBodyPart::LeftThigh:
  case ModBodyPart::RightThigh:
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
    return 0.10f;

  case ModBodyPart::ChestBody:
  case ModBodyPart::StomachBody:
  case ModBodyPart::Count:
  default:
    return 0.10f;
  }
}

float ModScene::GetChildCurrentAttachRadius(const PartNode &childNode) const {
  const float defaultRadius = GetChildDefaultAttachRadius(childNode.part);

  if (modBodies_.count(childNode.id) > 0) {
    switch (childNode.part) {
    case ModBodyPart::LeftUpperArm:
    case ModBodyPart::RightUpperArm:
    case ModBodyPart::LeftThigh:
    case ModBodyPart::RightThigh:
    case ModBodyPart::Neck: {
      const float rootRadius = GetPartControlPointRadius(
          childNode.id, ModControlPointRole::Root, defaultRadius);

      float bendDefault = defaultRadius;

      switch (childNode.part) {
      case ModBodyPart::LeftUpperArm:
      case ModBodyPart::RightUpperArm:
        bendDefault = 0.09f;
        break;

      case ModBodyPart::LeftThigh:
      case ModBodyPart::RightThigh:
        bendDefault = 0.10f;
        break;

      case ModBodyPart::Neck:
        bendDefault = 0.08f;
        break;

      default:
        bendDefault = defaultRadius;
        break;
      }

      const float bendRadius = GetPartControlPointRadius(
          childNode.id, ModControlPointRole::Bend, bendDefault);

      const float effectiveRadius = (std::max)(rootRadius, bendRadius);

      const Vector3 scale = modBodies_.at(childNode.id).GetVisualScaleRatio();
      return effectiveRadius * (std::max)(scale.x, scale.z);
    }

    default:
      break;
    }
  }

  if (childNode.part == ModBodyPart::LeftForeArm ||
      childNode.part == ModBodyPart::RightForeArm ||
      childNode.part == ModBodyPart::LeftShin ||
      childNode.part == ModBodyPart::RightShin ||
      childNode.part == ModBodyPart::Head) {
    const int ownerId = ResolveControlOwnerPartId(assembly_, childNode.id);
    if (ownerId >= 0 && modBodies_.count(ownerId) > 0) {
      const float pointRadius = GetPartControlPointRadius(
          ownerId, ModControlPointRole::Bend, defaultRadius);
      const Vector3 scale = modBodies_.at(ownerId).GetVisualScaleRatio();
      return pointRadius * (std::max)(scale.x, scale.z);
    }
  }

  if (modBodies_.count(childNode.id) > 0) {
    const Vector3 scale = modBodies_.at(childNode.id).GetVisualScaleRatio();
    return defaultRadius * (std::max)(scale.x, scale.z);
  }

  return defaultRadius;
}

float ModScene::GetChildDefaultLength(ModBodyPart part) const {
  switch (part) {
  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::RightUpperArm:
  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
    return 1.10f;

  case ModBodyPart::LeftThigh:
  case ModBodyPart::RightThigh:
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
    return 1.40f;

  case ModBodyPart::Neck:
    return 0.28f;

  case ModBodyPart::Head:
    return 0.52f;

  case ModBodyPart::ChestBody:
  case ModBodyPart::StomachBody:
  case ModBodyPart::Count:
  default:
    return 1.0f;
  }
}

float ModScene::GetChildCurrentLength(const PartNode &childNode) const {
  const float defaultLength = GetChildDefaultLength(childNode.part);

  if (modBodies_.count(childNode.id) > 0) {
    const Vector3 scale = modBodies_.at(childNode.id).GetVisualScaleRatio();
    return defaultLength * scale.y;
  }

  return defaultLength;
}

int ModScene::ResolveSelectedAssemblyRootPartId(int partId) const {
  return ModAssemblyResolver::ResolveAssemblyRootPartId(assembly_, partId);
}

void ModScene::BeginAssemblyDragFromPart(int pickedPartId) {
  const PartNode *pickedNode = assembly_.FindNode(pickedPartId);
  if (pickedNode == nullptr) {
    assemblyDrag_.Clear();
    return;
  }

  const int rootPartId = ResolveSelectedAssemblyRootPartId(pickedPartId);
  const PartNode *rootNode = assembly_.FindNode(rootPartId);
  if (rootNode == nullptr) {
    assemblyDrag_.Clear();
    return;
  }

  assemblyDrag_.Clear();
  assemblyDrag_.isDragging = true;
  assemblyDrag_.pickedPartId = pickedPartId;
  assemblyDrag_.assemblyRootPartId = rootPartId;
  assemblyDrag_.assemblyType = ModAssemblyUtil::GetAssemblyType(rootNode->part);
  assemblyDrag_.side = ModAssemblyUtil::GetAssemblySide(rootNode->part);

  assemblyDrag_.beforeParentId = rootNode->parentId;
  assemblyDrag_.beforeParentConnectorId = rootNode->parentConnectorId;
  assemblyDrag_.beforeSelfConnectorId = rootNode->selfConnectorId;
  assemblyDrag_.beforeLocalTranslate = rootNode->localTransform.translate;

  assemblyDrag_.previewLocalTranslate = rootNode->localTransform.translate;

  assemblyDrag_.dragPlaneNormal = {0.0f, 0.0f, 1.0f};
  assemblyDrag_.dragPlanePoint = GetAssemblyRootWorldPosition(rootPartId);

  assemblyDrag_.previewLocalTranslate = assemblyDrag_.beforeLocalTranslate;
  assemblyDrag_.dragRootOffset = {0.0f, 0.0f, 0.0f};
}

bool ModScene::IsAssemblyRootPartId(int partId) const {
  const PartNode *node = assembly_.FindNode(partId);
  if (node == nullptr) {
    return false;
  }

  return ModAssemblyUtil::IsAssemblyRootPart(node->part);
}

Vector3 ModScene::GetPartWorldPosition(int partId) const {
  auto it = modObjects_.find(partId);
  if (it == modObjects_.end() || it->second == nullptr) {
    return {0.0f, 0.0f, 0.0f};
  }

  return ModObjectUtil::ComputeObjectRootWorldTranslate(it->second.get());
}

Vector3 ModScene::GetAssemblyRootWorldPosition(int rootPartId) const {
  return GetPartWorldPosition(rootPartId);
}

bool ModScene::CanAttachAssemblyRootToParentPart(int childRootPartId,
                                                 int parentPartId) const {
  const PartNode *childNode = assembly_.FindNode(childRootPartId);
  const PartNode *parentNode = assembly_.FindNode(parentPartId);
  if (childNode == nullptr || parentNode == nullptr) {
    return false;
  }

  if (!ModAssemblyUtil::IsAssemblyRootPart(childNode->part)) {
    return false;
  }

  if (!ModAssemblyUtil::IsAssemblyRootPart(parentNode->part)) {
    return false;
  }

  if (childRootPartId == parentPartId) {
    return false;
  }

  const int childResolvedRoot = ModAssemblyResolver::ResolveAssemblyRootPartId(
      assembly_, childRootPartId);
  const int parentResolvedRoot =
      ModAssemblyResolver::ResolveAssemblyRootPartId(assembly_, parentPartId);

  if (childResolvedRoot == parentResolvedRoot) {
    return false;
  }

  if (!ModAssemblyUtil::CanPartParentChild(parentNode->part, childNode->part)) {
    return false;
  }

  const PartSide childSide = ModAssemblyUtil::GetAssemblySide(childNode->part);
  const PartSide parentSide =
      ModAssemblyUtil::GetAssemblySide(parentNode->part);

  if (!ModAssemblyUtil::IsSideCompatible(parentSide, childSide)) {
    return false;
  }

  return true;
}

int ModScene::FindBestParentConnectorIdForPosition(
    const PartNode &parentNode, const Vector3 &worldPosition) const {
  float bestDistSq = FLT_MAX;
  int bestConnectorId = -1;

  const Vector3 parentWorld = GetPartWorldPosition(parentNode.id);

  for (size_t i = 0; i < parentNode.connectors.size(); ++i) {
    const ConnectorNode &connector = parentNode.connectors[i];

    const Vector3 connectorWorld = Add(parentWorld, connector.localPosition);

    const Vector3 diff = Subtract(worldPosition, connectorWorld);
    const float distSq = Dot(diff, diff);

    if (distSq < bestDistSq) {
      bestDistSq = distSq;
      bestConnectorId = connector.id;
    }
  }

  return bestConnectorId;
}

bool ModScene::BuildAttachCapsuleCandidate(
    int childRootPartId, int parentPartId, const Vector3 &dragWorldPosition,
    ModAttachFaceCandidate *outCandidate) const {
  if (outCandidate == nullptr) {
    return false;
  }

  outCandidate->isValid = false;

  if (!CanAttachAssemblyRootToParentPart(childRootPartId, parentPartId)) {
    return false;
  }

  const PartNode *childNode = assembly_.FindNode(childRootPartId);
  const PartNode *parentNode = assembly_.FindNode(parentPartId);
  if (childNode == nullptr || parentNode == nullptr) {
    return false;
  }

  ModSceneSegmentBoxSet parentBoxes{};
  if (!BuildPartPickBoxes(parentPartId, parentBoxes)) {
    return false;
  }

  float bestSurfaceDistSq = FLT_MAX;
  int bestBoxIndex = -1;

  Vector3 bestAttachPoint{0.0f, 0.0f, 0.0f};
  Vector3 bestWorldNormal{0.0f, 1.0f, 0.0f};
  Vector3 bestConnectorSearchPoint{0.0f, 0.0f, 0.0f};
  ModAttachFace bestFace = ModAttachFace::PosY;

  for (int bi = 0; bi < parentBoxes.count; ++bi) {
    const ModSceneSegmentBox &box = parentBoxes.segments[bi];

    const Vector3 toPoint = Subtract(dragWorldPosition, box.center);

    const float localX = Dot(toPoint, box.axisX);
    const float localY = Dot(toPoint, box.axisY);
    const float localZ = Dot(toPoint, box.axisZ);

    const float clampedX =
        ClampFloatLocal(localX, -box.halfWidth, box.halfWidth);
    const float clampedY =
        ClampFloatLocal(localY, -box.halfLength, box.halfLength);
    const float clampedZ =
        ClampFloatLocal(localZ, -box.halfDepth, box.halfDepth);

    // 箱内部/近傍の基準点
    const Vector3 closestPointInBox =
        Add(Add(Add(box.center, Multiply(clampedX, box.axisX)),
                Multiply(clampedY, box.axisY)),
            Multiply(clampedZ, box.axisZ));

    // 各面までの距離を比較して最も近い面を選ぶ
    const float distToPosX = fabsf(localX - box.halfWidth);
    const float distToNegX = fabsf(localX + box.halfWidth);
    const float distToPosY = fabsf(localY - box.halfLength);
    const float distToNegY = fabsf(localY + box.halfLength);
    const float distToPosZ = fabsf(localZ - box.halfDepth);
    const float distToNegZ = fabsf(localZ + box.halfDepth);

    float minFaceDist = distToPosX;
    ModAttachFace face = ModAttachFace::PosX;
    Vector3 faceNormal = box.axisX;

    if (distToNegX < minFaceDist) {
      minFaceDist = distToNegX;
      face = ModAttachFace::NegX;
      faceNormal = Multiply(-1.0f, box.axisX);
    }

    if (distToPosY < minFaceDist) {
      minFaceDist = distToPosY;
      face = ModAttachFace::PosY;
      faceNormal = box.axisY;
    }

    if (distToNegY < minFaceDist) {
      minFaceDist = distToNegY;
      face = ModAttachFace::NegY;
      faceNormal = Multiply(-1.0f, box.axisY);
    }

    if (distToPosZ < minFaceDist) {
      minFaceDist = distToPosZ;
      face = ModAttachFace::PosZ;
      faceNormal = box.axisZ;
    }

    if (distToNegZ < minFaceDist) {
      minFaceDist = distToNegZ;
      face = ModAttachFace::NegZ;
      faceNormal = Multiply(-1.0f, box.axisZ);
    }

    float surfaceX = clampedX;
    float surfaceY = clampedY;
    float surfaceZ = clampedZ;

    switch (face) {
    case ModAttachFace::PosX:
      surfaceX = box.halfWidth;
      break;
    case ModAttachFace::NegX:
      surfaceX = -box.halfWidth;
      break;
    case ModAttachFace::PosY:
      surfaceY = box.halfLength;
      break;
    case ModAttachFace::NegY:
      surfaceY = -box.halfLength;
      break;
    case ModAttachFace::PosZ:
      surfaceZ = box.halfDepth;
      break;
    case ModAttachFace::NegZ:
      surfaceZ = -box.halfDepth;
      break;
    default:
      break;
    }

    const Vector3 attachPoint =
        Add(Add(Add(box.center, Multiply(surfaceX, box.axisX)),
                Multiply(surfaceY, box.axisY)),
            Multiply(surfaceZ, box.axisZ));

    const Vector3 surfaceDiff = Subtract(dragWorldPosition, attachPoint);
    const float distSq = Dot(surfaceDiff, surfaceDiff);

    if (distSq < bestSurfaceDistSq) {
      bestSurfaceDistSq = distSq;
      bestBoxIndex = bi;
      bestAttachPoint = attachPoint;
      bestWorldNormal = NormalizeSafeV(faceNormal, {0.0f, 1.0f, 0.0f});
      bestConnectorSearchPoint = closestPointInBox;
      bestFace = face;
    }
  }

  if (bestBoxIndex < 0) {
    return false;
  }

  ModAttachFaceCandidate candidate{};
  candidate.parentPartId = parentPartId;
  candidate.parentConnectorId = FindBestParentConnectorIdForPosition(
      *parentNode, bestConnectorSearchPoint);
  candidate.face = bestFace;
  candidate.worldPosition = bestAttachPoint;
  candidate.worldNormal = bestWorldNormal;

  const Vector3 diff = Subtract(dragWorldPosition, bestAttachPoint);
  candidate.distanceSq = Dot(diff, diff);
  candidate.isValid = (candidate.parentConnectorId >= 0);

  if (!candidate.isValid) {
    return false;
  }

  *outCandidate = candidate;
  return true;
}

ModAttachSearchResult
ModScene::FindBestAttachCandidate(int childRootPartId,
                                  const Vector3 &dragWorldPosition) const {
  ModAttachSearchResult result{};

  const float maxDistSq =
      assemblyAttachSearchRadius_ * assemblyAttachSearchRadius_;

  const std::vector<int> ids = assembly_.GetNodeIdsSorted();

  for (size_t i = 0; i < ids.size(); ++i) {
    const int parentId = ids[i];

    if (!IsAssemblyRootPartId(parentId)) {
      continue;
    }

    if (parentId == childRootPartId) {
      continue;
    }

    ModAttachFaceCandidate candidate{};
    if (!BuildAttachCapsuleCandidate(childRootPartId, parentId,
                                     dragWorldPosition, &candidate)) {
      continue;
    }

    if (candidate.distanceSq > maxDistSq) {
      continue;
    }

    if (!result.found ||
        candidate.distanceSq < result.bestCandidate.distanceSq) {
      result.bestCandidate = candidate;
      result.found = true;
    }
  }

  return result;
}

Vector3 ModScene::ComputeAssemblyPreviewLocalTranslate(
    int childRootPartId, const ModAttachFaceCandidate &candidate) const {
  const PartNode *childNode = assembly_.FindNode(childRootPartId);
  const PartNode *parentNode = assembly_.FindNode(candidate.parentPartId);
  if (childNode == nullptr || parentNode == nullptr) {
    return {0.0f, 0.0f, 0.0f};
  }

  const Vector3 defaultAttach = assembly_.GetDefaultAttachLocal(
      parentNode->part, childNode->part, childNode->side);

  const Vector3 baseAttach = ResolveDynamicAttachBase(*parentNode, *childNode);
  const Vector3 currentRootWorld =
      GetAssemblyRootWorldPosition(childRootPartId);
  const Vector3 desiredRootWorld = candidate.worldPosition;

  Vector3 worldDelta = Subtract(desiredRootWorld, currentRootWorld);

  // 現状エンジンは親回転を接続計算へ強く入れていないので、
  // ここでは world delta をそのまま local delta として扱う
  const Vector3 currentLocal = childNode->localTransform.translate;
  const Vector3 offsetFromDefault = Subtract(currentLocal, defaultAttach);

  return Add(Add(baseAttach, offsetFromDefault), worldDelta);
}

void ModScene::UpdateAssemblyAttachCandidateFromMouseRay(const Ray &mouseRay) {
  if (!assemblyDrag_.isDragging || assemblyDrag_.assemblyRootPartId < 0) {
    return;
  }

  Vector3 hitPoint{};
  if (!RayPlaneIntersectionZ(mouseRay, assemblyDrag_.dragPlanePoint.z,
                             &hitPoint)) {
    assemblyDrag_.hoveredParentPartId = -1;
    assemblyDrag_.hoveredParentConnectorId = -1;
    assemblyDrag_.hoveredFace = ModAttachFace::PosY;
    assemblyDrag_.snappedWorldPosition = {0.0f, 0.0f, 0.0f};
    assemblyDrag_.snappedWorldNormal = {0.0f, 1.0f, 0.0f};
    assemblyDrag_.hasSnappedCandidate = false;
    assemblyDrag_.isPlacementValid = false;
    assemblyDrag_.previewLocalTranslate = assemblyDrag_.beforeLocalTranslate;
    return;
  }

  const Vector3 desiredRootWorld = Add(hitPoint, assemblyDrag_.dragRootOffset);

  // まず自由ドラッグ位置へ追従
  assemblyDrag_.previewLocalTranslate = ComputeAssemblyFreeDragLocalTranslate(
      assemblyDrag_.assemblyRootPartId, desiredRootWorld);

  ModAttachSearchResult search = FindBestAttachCandidate(
      assemblyDrag_.assemblyRootPartId, desiredRootWorld);

  if (!search.found) {
    assemblyDrag_.hoveredParentPartId = -1;
    assemblyDrag_.hoveredParentConnectorId = -1;
    assemblyDrag_.hoveredFace = ModAttachFace::PosY;
    assemblyDrag_.snappedWorldPosition = {0.0f, 0.0f, 0.0f};
    assemblyDrag_.snappedWorldNormal = {0.0f, 1.0f, 0.0f};
    assemblyDrag_.hasSnappedCandidate = false;
    assemblyDrag_.isPlacementValid = false;
    return;
  }

  assemblyDrag_.hoveredParentPartId = search.bestCandidate.parentPartId;
  assemblyDrag_.hoveredParentConnectorId =
      search.bestCandidate.parentConnectorId;
  assemblyDrag_.hoveredFace = search.bestCandidate.face;
  assemblyDrag_.snappedWorldPosition = search.bestCandidate.worldPosition;
  assemblyDrag_.snappedWorldNormal = search.bestCandidate.worldNormal;
  assemblyDrag_.hasSnappedCandidate = true;

  const float snapDistSq =
      assemblyAttachSnapRadius_ * assemblyAttachSnapRadius_;

  const bool insideSnapRange = (search.bestCandidate.distanceSq <= snapDistSq);

  if (insideSnapRange) {
    assemblyDrag_.previewLocalTranslate = ComputeAssemblyPreviewLocalTranslate(
        assemblyDrag_.assemblyRootPartId, search.bestCandidate);

    // まずは接続候補とスナップ挙動だけ確認する
    // overlap 判定は後で親近傍許可を入れてから戻す
    assemblyDrag_.isPlacementValid = true;
  } else {
    assemblyDrag_.isPlacementValid = false;
  }
}

void ModScene::ApplyAssemblyDragPreview() {
  if (!assemblyDrag_.isDragging || assemblyDrag_.assemblyRootPartId < 0) {
    return;
  }

  assembly_.SetPartLocalTranslate(assemblyDrag_.assemblyRootPartId,
                                  assemblyDrag_.previewLocalTranslate);
}

void ModScene::UpdateAssemblyDragTest() {
#ifdef USE_IMGUI
  if (ImGui::GetIO().WantCaptureMouse) {
    return;
  }
#endif

  if (usingCamera_ == nullptr) {
    return;
  }

  if (selectedPartId_ < 0) {
    return;
  }

  if (system_->GetTriggerOn(DIK_G) && !assemblyDrag_.isDragging) {
    BeginAssemblyDragFromPart(selectedPartId_);
  }

  if (!assemblyDrag_.isDragging) {
    return;
  }

  Ray mouseRay = usingCamera_->ScreenPointToRay(system_->GetMousePosVector2());

  UpdateAssemblyAttachCandidateFromMouseRay(mouseRay);
  ApplyAssemblyDragPreview();

  // 左クリックを離したら配置確定判定
  if (IsMouseLeftReleasedNow()) {
    if (assemblyDrag_.isPlacementValid) {
      ConfirmAssemblyDragPlacement();
    } else {
      CancelAssemblyDragPlacement();
    }
    return;
  }

  // Esc は常にキャンセル
  if (system_->GetTriggerOn(DIK_ESCAPE)) {
    CancelAssemblyDragPlacement();
    return;
  }
}

bool ModScene::IsPartInDraggingAssembly(int partId) const {
  if (!assemblyDrag_.isDragging || assemblyDrag_.assemblyRootPartId < 0) {
    return false;
  }

  return ModAssemblyResolver::BelongsToAssemblyRoot(
      assembly_, assemblyDrag_.assemblyRootPartId, partId);
}

void ModScene::CancelAssemblyDragPlacement() {
  if (!assemblyDrag_.isDragging || assemblyDrag_.assemblyRootPartId < 0) {
    assemblyDrag_.Clear();
    return;
  }

  const int rootPartId = assemblyDrag_.assemblyRootPartId;
  const PartNode *rootNode = assembly_.FindNode(rootPartId);
  if (rootNode == nullptr) {
    assemblyDrag_.Clear();
    return;
  }

  if (assemblyDrag_.beforeParentId >= 0) {
    assembly_.MovePart(rootPartId, assemblyDrag_.beforeParentId,
                       assemblyDrag_.beforeParentConnectorId);
  }

  assembly_.SetPartLocalTranslate(rootPartId,
                                  assemblyDrag_.beforeLocalTranslate);

  SelectPart(assemblyDrag_.pickedPartId >= 0 ? assemblyDrag_.pickedPartId
                                             : rootPartId);

  assemblyDrag_.Clear();
}

void ModScene::ConfirmAssemblyDragPlacement() {
  if (!assemblyDrag_.isDragging || assemblyDrag_.assemblyRootPartId < 0) {
    assemblyDrag_.Clear();
    return;
  }

  if (!assemblyDrag_.isPlacementValid ||
      assemblyDrag_.hoveredParentPartId < 0 ||
      !assemblyDrag_.hasSnappedCandidate) {
    CancelAssemblyDragPlacement();
    return;
  }

  const int rootPartId = assemblyDrag_.assemblyRootPartId;
  const int newParentId = assemblyDrag_.hoveredParentPartId;
  const int newParentConnectorId = assemblyDrag_.hoveredParentConnectorId;

  const PartNode *childNode = assembly_.FindNode(rootPartId);
  const PartNode *parentNode = assembly_.FindNode(newParentId);
  if (childNode == nullptr || parentNode == nullptr) {
    CancelAssemblyDragPlacement();
    return;
  }

  ModAttachFaceCandidate snappedCandidate{};
  snappedCandidate.parentPartId = newParentId;
  snappedCandidate.parentConnectorId = newParentConnectorId;
  snappedCandidate.face = assemblyDrag_.hoveredFace;
  snappedCandidate.worldPosition = assemblyDrag_.snappedWorldPosition;
  snappedCandidate.worldNormal = assemblyDrag_.snappedWorldNormal;
  snappedCandidate.isValid = true;

  const Vector3 snappedLocalTranslate =
      ComputeAssemblyPreviewLocalTranslate(rootPartId, snappedCandidate);

  if (!assembly_.MovePart(rootPartId, newParentId, newParentConnectorId)) {
    CancelAssemblyDragPlacement();
    return;
  }

  assembly_.SetPartLocalTranslate(rootPartId, snappedLocalTranslate);

  SelectPart(rootPartId);
  assemblyDrag_.Clear();
}

void ModScene::ApplyAssemblyDragVisualFeedback() {
  if (!assemblyDrag_.isDragging || assemblyDrag_.assemblyRootPartId < 0) {
    return;
  }

  const bool valid = assemblyDrag_.isPlacementValid;

  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    if (!IsPartInDraggingAssembly(id)) {
      continue;
    }

    auto it = modObjects_.find(id);
    if (it == modObjects_.end() || it->second == nullptr) {
      continue;
    }

    Object *object = it->second.get();
    if (object == nullptr) {
      continue;
    }

    for (size_t partIndex = 0; partIndex < object->objectParts_.size();
         ++partIndex) {
      if (object->objectParts_[partIndex].materialConfig == nullptr) {
        continue;
      }

      Vector4 &color =
          object->objectParts_[partIndex].materialConfig->textureColor;

      if (valid) {
        color.x = 0.25f;
        color.y = 1.0f;
        color.z = 0.35f;
      } else {
        color.x = 1.0f;
        color.y = 0.25f;
        color.z = 0.25f;
      }
    }
  }
}

bool ModScene::IsMouseLeftPressedNow() const {
  return system_ != nullptr && system_->GetMouseIsPush(0);
}

bool ModScene::IsMouseLeftTriggeredNow() const {
  return system_ != nullptr && system_->GetMouseTriggerOn(0);
}

bool ModScene::IsMouseLeftReleasedNow() const {
  return system_ != nullptr && system_->GetMouseTriggerOff(0);
}

bool ModScene::IsAssemblyDragPriorityControlPoint(int ownerPartId,
                                                  int pointIndex) const {
  if (ownerPartId == -2) {
    return false;
  }

  auto bodyIt = modBodies_.find(ownerPartId);
  if (bodyIt == modBodies_.end()) {
    return false;
  }

  const std::vector<ModControlPoint> &points =
      bodyIt->second.GetControlPoints();
  if (pointIndex < 0 || static_cast<size_t>(pointIndex) >= points.size()) {
    return false;
  }

  const ModControlPointRole role = points[static_cast<size_t>(pointIndex)].role;

  // 根元接続点は部位ドラッグ開始を優先する
  return role == ModControlPointRole::Root;
}

bool ModScene::TryBeginAssemblyDragFromMouseRay(const Ray &mouseRay) {
  if (assemblyDrag_.isDragging) {
    return false;
  }

  const int visiblePartId =
      (hoveredPartId_ >= 0) ? hoveredPartId_ : selectedPartId_;
  if (visiblePartId < 0) {
    return false;
  }

  const int rootPartId = ResolveSelectedAssemblyRootPartId(visiblePartId);
  if (rootPartId < 0) {
    return false;
  }

  const Vector3 rootWorld = GetAssemblyRootWorldPosition(rootPartId);

  BeginAssemblyDragFromPart(visiblePartId);
  SelectPart(visiblePartId);

  Vector3 hitPoint{};
  if (RayPlaneIntersectionZ(mouseRay, assemblyDrag_.dragPlanePoint.z,
                            &hitPoint)) {
    assemblyDrag_.dragRootOffset = Subtract(rootWorld, hitPoint);
  } else {
    assemblyDrag_.dragRootOffset = {0.0f, 0.0f, 0.0f};
  }

  return true;
}

Vector3 ModScene::ComputeAssemblyFreeDragLocalTranslate(
    int childRootPartId, const Vector3 &desiredRootWorld) const {
  const PartNode *childNode = assembly_.FindNode(childRootPartId);
  if (childNode == nullptr) {
    return {0.0f, 0.0f, 0.0f};
  }

  const Vector3 startRootWorld = assemblyDrag_.dragPlanePoint;
  const Vector3 worldDelta = Subtract(desiredRootWorld, startRootWorld);

  return Add(assemblyDrag_.beforeLocalTranslate, worldDelta);
}

bool ModScene::IsAssemblyPreviewOverlapping(int movingAssemblyRootPartId,
                                            int ignoreParentPartId) const {
  const float overlapMargin = 0.01f;

  std::vector<int> movingParts;
  const std::vector<int> allIds = assembly_.GetNodeIdsSorted();

  for (size_t i = 0; i < allIds.size(); ++i) {
    if (IsDescendantPartIdRecursive(assembly_, movingAssemblyRootPartId,
                                    allIds[i])) {
      movingParts.push_back(allIds[i]);
    }
  }

  for (size_t i = 0; i < movingParts.size(); ++i) {
    ModSceneSegmentBoxSet movingSet{};
    if (!BuildPartPickBoxes(movingParts[i], movingSet)) {
      continue;
    }

    for (size_t j = 0; j < allIds.size(); ++j) {
      const int otherId = allIds[j];

      if (IsDescendantPartIdRecursive(assembly_, movingAssemblyRootPartId,
                                      otherId)) {
        continue;
      }

      if (otherId == ignoreParentPartId) {
        continue;
      }

      if (ignoreParentPartId >= 0) {
        const int otherGroupId = ResolveFadeGroupId(otherId);
        const int ignoreGroupId = ResolveFadeGroupId(ignoreParentPartId);

        if (otherGroupId >= 0 && otherGroupId == ignoreGroupId) {
          continue;
        }
      }

      ModSceneSegmentBoxSet otherSet{};
      if (!BuildPartPickBoxes(otherId, otherSet)) {
        continue;
      }

      for (int mi = 0; mi < movingSet.count; ++mi) {
        for (int oi = 0; oi < otherSet.count; ++oi) {
          if (IntersectSegmentBoxes(movingSet.segments[mi],
                                    otherSet.segments[oi], overlapMargin)) {
            return true;
          }
        }
      }
    }
  }

  return false;
}

Vector3
ModScene::ResolveChildSelfAttachOffset(const PartNode &childNode) const {
  Vector3 outward = ResolveAttachOutwardDirection(childNode);

  if (modBodies_.count(childNode.id) > 0) {
    const ModBody &body = modBodies_.at(childNode.id);

    switch (childNode.part) {
    case ModBodyPart::LeftUpperArm:
    case ModBodyPart::RightUpperArm:
    case ModBodyPart::LeftThigh:
    case ModBodyPart::RightThigh:
    case ModBodyPart::Neck: {
      const int rootIndex =
          body.FindControlPointIndex(ModControlPointRole::Root);
      const int bendIndex =
          body.FindControlPointIndex(ModControlPointRole::Bend);

      if (rootIndex >= 0 && bendIndex >= 0) {
        const std::vector<ModControlPoint> &points = body.GetControlPoints();
        const Vector3 dir =
            Subtract(points[static_cast<size_t>(bendIndex)].localPosition,
                     points[static_cast<size_t>(rootIndex)].localPosition);

        if (Length(dir) > 0.0001f) {
          outward = NormalizeSafeV(dir, outward);
        }
      }
      break;
    }

    default:
      break;
    }
  }

  if (childNode.part == ModBodyPart::LeftForeArm ||
      childNode.part == ModBodyPart::RightForeArm ||
      childNode.part == ModBodyPart::LeftShin ||
      childNode.part == ModBodyPart::RightShin ||
      childNode.part == ModBodyPart::Head) {
    const int ownerId = ResolveControlOwnerPartId(assembly_, childNode.id);
    if (ownerId >= 0 && modBodies_.count(ownerId) > 0) {
      const ModBody &ownerBody = modBodies_.at(ownerId);

      const int bendIndex =
          ownerBody.FindControlPointIndex(ModControlPointRole::Bend);
      const int endIndex =
          ownerBody.FindControlPointIndex(ModControlPointRole::End);

      if (bendIndex >= 0 && endIndex >= 0) {
        const std::vector<ModControlPoint> &points =
            ownerBody.GetControlPoints();
        const Vector3 dir =
            Subtract(points[static_cast<size_t>(endIndex)].localPosition,
                     points[static_cast<size_t>(bendIndex)].localPosition);

        if (Length(dir) > 0.0001f) {
          outward = NormalizeSafeV(dir, outward);
        }
      }
    }
  }

  const float defaultRadius = GetChildDefaultAttachRadius(childNode.part);
  const float currentRadius = GetChildCurrentAttachRadius(childNode);
  const float extraRadius = (std::max)(0.0f, currentRadius - defaultRadius);

  const float defaultLength = GetChildDefaultLength(childNode.part);
  const float currentLength = GetChildCurrentLength(childNode);
  const float extraLength = (std::max)(0.0f, currentLength - defaultLength);

  float pushDistance = extraRadius;
  pushDistance += extraLength * 0.35f;

  return Multiply(pushDistance, outward);
}

bool ModScene::IsTorsoPart(ModBodyPart part) const {
  return part == ModBodyPart::ChestBody || part == ModBodyPart::StomachBody;
}

bool ModScene::IsTorsoVisiblePartId(int partId) const {
  const PartNode *node = assembly_.FindNode(partId);
  if (node == nullptr) {
    return false;
  }
  return IsTorsoPart(node->part);
}

Vector3
ModScene::GetTorsoControlPointWorldPosition(ModControlPointRole role) const {
  const int chestBodyId = assembly_.GetBodyId();
  if (chestBodyId < 0 || modObjects_.count(chestBodyId) == 0) {
    return ZeroV();
  }

  const int pointIndex = FindTorsoControlPointIndex(role);
  if (pointIndex < 0) {
    return ZeroV();
  }

  const Object *bodyObject = modObjects_.at(chestBodyId).get();
  if (bodyObject == nullptr) {
    return ZeroV();
  }

  return ModObjectUtil::TransformLocalPointToWorld(
      bodyObject,
      torsoControlPoints_[static_cast<size_t>(pointIndex)].localPosition);
}

void ModScene::UpdateModObjects() {
  ApplyAssemblyToSceneHierarchy();

  torsoSharedPointsBuffer_.clear();
  torsoSharedPointsBuffer_.reserve(torsoControlPoints_.size());

  for (size_t i = 0; i < torsoControlPoints_.size(); ++i) {
    ModControlPoint point{};
    point.role = torsoControlPoints_[i].role;
    point.localPosition = torsoControlPoints_[i].localPosition;
    point.radius = torsoControlPoints_[i].radius;
    point.movable = torsoControlPoints_[i].movable;
    point.isConnectionPoint = torsoControlPoints_[i].isConnectionPoint;
    point.acceptsParent = torsoControlPoints_[i].acceptsParent;
    point.acceptsChild = torsoControlPoints_[i].acceptsChild;
    torsoSharedPointsBuffer_.push_back(point);
  }

  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    const PartNode *node = assembly_.FindNode(id);
    if (node == nullptr) {
      continue;
    }

    if (modBodies_.count(id) == 0) {
      continue;
    }

    ModBody &body = modBodies_[id];
    body.ClearExternalSegmentSource();

    switch (node->part) {
    case ModBodyPart::ChestBody: {
      body.SetExternalSegmentSource(&torsoSharedPointsBuffer_,
                                    ModControlPointRole::Chest,
                                    ModControlPointRole::Belly);
      break;
    }

    case ModBodyPart::StomachBody: {
      body.SetExternalSegmentSource(&torsoSharedPointsBuffer_,
                                    ModControlPointRole::Belly,
                                    ModControlPointRole::Waist);
      break;
    }

    case ModBodyPart::LeftForeArm:
    case ModBodyPart::RightForeArm:
    case ModBodyPart::LeftShin:
    case ModBodyPart::RightShin:
    case ModBodyPart::Head: {
      const int ownerId = ResolveControlOwnerPartId(assembly_, id);
      if (ownerId >= 0 && ownerId != id && modBodies_.count(ownerId) > 0) {
        body.SetExternalSegmentSource(&modBodies_[ownerId].GetControlPoints(),
                                      ModControlPointRole::Bend,
                                      ModControlPointRole::End);
      }
      break;
    }

    default:
      break;
    }
  }

  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];

    if (modObjects_.count(id) == 0 || modBodies_.count(id) == 0) {
      continue;
    }

    const PartNode *node = assembly_.FindNode(id);
    if (node != nullptr && node->part == ModBodyPart::Head) {
      const ModBodyPartParam &param = modBodies_[id].GetParam();
      // Logger::Log("HEAD APPLY");
      // Logger::Log("id = %d", id);
      // Logger::Log("enabled = %d", param.enabled ? 1 : 0);
      // Logger::Log("scale = (%.3f, %.3f, %.3f)", param.scale.x, param.scale.y,
      //             param.scale.z);
      // Logger::Log("length = %.3f", param.length);
    }

    Object *object = modObjects_[id].get();
    if (object != nullptr) {
      modBodies_[id].Apply(object);
    }
  }

  int fadedGroupId = -1;

  if (hoveredPartId_ >= 0) {
    fadedGroupId = ResolveFadeGroupId(hoveredPartId_);
  }

  if (selectedControlPointIndex_ >= 0) {
    if (selectedControlPartId_ == -2) {
      const int chestId = assembly_.GetBodyId();
      if (chestId >= 0) {
        fadedGroupId = ResolveFadeGroupId(chestId);
      }
    } else if (selectedPartId_ >= 0) {
      fadedGroupId = ResolveFadeGroupId(selectedPartId_);
    }
  }

  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    if (modObjects_.count(id) == 0) {
      continue;
    }

    Object *object = modObjects_[id].get();
    if (object == nullptr || object->objectParts_.empty()) {
      continue;
    }

    float alpha = 1.0f;

    if (fadedGroupId >= 0) {
      const int groupId = ResolveFadeGroupId(id);
      if (groupId == fadedGroupId) {
        alpha = 0.45f;
      }
    }

    for (size_t partIndex = 0; partIndex < object->objectParts_.size();
         ++partIndex) {
      if (object->objectParts_[partIndex].materialConfig != nullptr) {
        Vector4 &color =
            object->objectParts_[partIndex].materialConfig->textureColor;

        // まず通常色へ戻す
        color.x = 1.0f;
        color.y = 1.0f;
        color.z = 1.0f;
        color.w = alpha;
      }
    }

    object->Update(usingCamera_);
  }

  ApplyAssemblyDragVisualFeedback();

  // 色変更後にもう一度更新して反映
  if (assemblyDrag_.isDragging) {
    for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
      const int id = orderedPartIds_[i];
      auto it = modObjects_.find(id);
      if (it == modObjects_.end() || it->second == nullptr) {
        continue;
      }

      it->second->Update(usingCamera_);
    }
  }

  UpdateControlPointGizmos();
}

void ModScene::DrawModObjects() {
  // orderedPartIds_ の順に各部位 Object を描画する
  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    if (modObjects_.count(id) == 0) {
      continue;
    }

    Object *object = modObjects_[id].get();
    if (object != nullptr) {
      object->Draw();
    }
  }
}

#ifdef USE_IMGUI
void ModScene::DrawAssemblyGui() {
  // 部位一覧ツリーを開いているときだけ一覧を描画する
  if (ImGui::TreeNode("Assembly Parts")) {
    for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
      const int id = orderedPartIds_[i];
      const PartNode *node = assembly_.FindNode(id);
      if (node == nullptr) {
        continue;
      }

      // 各部位を選択可能なリストとして表示する
      ImGui::PushID(id);

      const bool selected = (id == selectedPartId_);
      char label[128];
      sprintf_s(label, "%s (Id=%d)", PartName(node->part), id);

      if (ImGui::Selectable(label, selected)) {
        SelectPart(id);
      }

      ImGui::PopID();
    }

    ImGui::TreePop();
  }
}

void ModScene::DrawSelectedPartGui() {
  // 選択部位が無い場合は案内だけ表示する
  if (selectedPartId_ < 0) {
    ImGui::Text("No part selected.");
    return;
  }

  // 選択部位が不正なら編集を中断する
  const PartNode *node = assembly_.FindNode(selectedPartId_);
  if (node == nullptr || modBodies_.count(selectedPartId_) == 0 ||
      modObjects_.count(selectedPartId_) == 0) {
    ImGui::Text("Selected part is invalid.");
    return;
  }

  // 編集対象の Object とパラメータを取得する
  Object *object = modObjects_[selectedPartId_].get();
  ModBodyPartParam &param = modBodies_[selectedPartId_].GetParam();

  // 選択中部位の基本情報を表示する
  ImGui::Separator();
  ImGui::Text("Selected Part");
  ImGui::Text("PartId: %d", selectedPartId_);
  ImGui::Text("Type: %s", PartName(node->part));
  ImGui::Text("ParentId: %d", node->parentId);

  // ローカル位置を直接編集できるようにする
  Vector3 local = node->localTransform.translate;
  if (ImGui::SliderFloat3("Local Translate", &local.x, -5.0f, 5.0f)) {
    assembly_.SetPartLocalTranslate(selectedPartId_, local);
  }

  // 見た目の有効状態、スケール、長さを編集できるようにする
  ImGui::Checkbox("Enabled", &param.enabled);
  ImGui::SliderFloat3("Mesh Scale", &param.scale.x, 0.2f, 5.0f);
  ImGui::SliderFloat("Length", &param.length, 0.2f, 5.0f, "%.2f");

  // 現在の mesh transform を確認用に表示する
  if (!object->objectParts_.empty()) {
    const Transform &mesh = object->objectParts_[0].transform;
    ImGui::Text("Mesh Translate : %.2f %.2f %.2f", mesh.translate.x,
                mesh.translate.y, mesh.translate.z);
    ImGui::Text("Mesh Scale     : %.2f %.2f %.2f", mesh.scale.x, mesh.scale.y,
                mesh.scale.z);
  }

  // 付け替え先の親部位とコネクタを選択するUIを表示する
  ImGui::Separator();
  ImGui::Text("Reattach");
  if (ImGui::BeginCombo("Parent Part",
                        reattachParentId_ >= 0 ? "Selected" : "None")) {
    for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
      const int candidateId = orderedPartIds_[i];
      if (candidateId == selectedPartId_) {
        continue;
      }

      const PartNode *candidate = assembly_.FindNode(candidateId);
      if (candidate == nullptr) {
        continue;
      }

      char parentLabel[128];
      sprintf_s(parentLabel, "%s (Id=%d)", PartName(candidate->part),
                candidateId);

      const bool selected = (candidateId == reattachParentId_);
      if (ImGui::Selectable(parentLabel, selected)) {
        reattachParentId_ = candidateId;
        reattachConnectorId_ = -1;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  // 親部位が決まっている場合は、親側コネクタも選べるようにする
  if (reattachParentId_ >= 0) {
    const PartNode *parentNode = assembly_.FindNode(reattachParentId_);
    if (parentNode != nullptr) {
      if (ImGui::BeginCombo("Parent Connector",
                            reattachConnectorId_ >= 0 ? "Selected" : "Auto")) {
        if (ImGui::Selectable("Auto", reattachConnectorId_ < 0)) {
          reattachConnectorId_ = -1;
        }

        for (size_t i = 0; i < parentNode->connectors.size(); ++i) {
          const ConnectorNode &connector = parentNode->connectors[i];
          char connectorLabel[256];
          sprintf_s(connectorLabel, "Id=%d Role=%s Side=%s", connector.id,
                    ConnectorRoleName(connector.role),
                    SideName(connector.side));

          const bool selected = (connector.id == reattachConnectorId_);
          if (ImGui::Selectable(connectorLabel, selected)) {
            reattachConnectorId_ = connector.id;
          }
          if (selected) {
            ImGui::SetItemDefaultFocus();
          }
        }

        ImGui::EndCombo();
      }
    }
  }

  // 選択部位の付け替えと削除を実行できるようにする
  if (ImGui::Button("Apply Reattach")) {
    ReattachSelectedPart();
  }

  ImGui::SameLine();
  if (ImGui::Button("Delete Selected Part")) {
    DeleteSelectedPart();
  }

  // 選択部位だけパラメータを初期化するボタンを表示する
  ImGui::Separator();
  ImGui::Text("Reset");

  if (ImGui::Button("Reset Selected Part Params")) {
    ResetSelectedPartParams();
  }

  ImGui::Separator();
  ImGui::Text("Control Points");

  if (IsTorsoVisiblePartId(selectedPartId_)) {
    for (size_t i = 0; i < torsoControlPoints_.size(); ++i) {
      const bool isSelectedPoint =
          (selectedControlPartId_ == -2 &&
           selectedControlPointIndex_ == static_cast<int>(i));

      ImGui::PushID(static_cast<int>(i));

      if (isSelectedPoint) {
        ImGui::Text("-> Point %d", static_cast<int>(i));
      } else {
        ImGui::Text("Point %d", static_cast<int>(i));
      }

      Vector3 localPos = torsoControlPoints_[i].localPosition;
      if (ImGui::SliderFloat3("Local Pos", &localPos.x, -5.0f, 5.0f)) {
        MoveTorsoControlPoint(i, localPos);
      }

      float radius = torsoControlPoints_[i].radius;
      if (ImGui::SliderFloat("Radius", &radius, 0.06f, 0.30f, "%.2f")) {
        torsoControlPoints_[i].radius = radius;
      }

      ImGui::Text("Influence Radius : %.2f", torsoControlPoints_[i].radius);
      ImGui::Text("Movable: %s",
                  torsoControlPoints_[i].movable ? "true" : "false");

      ImGui::Separator();
      ImGui::PopID();
    }
  } else {
    const int controlOwnerId =
        ResolveControlOwnerPartId(assembly_, selectedPartId_);

    if (controlOwnerId >= 0 && modBodies_.count(controlOwnerId) > 0) {
      const std::vector<ModControlPoint> &points =
          modBodies_[controlOwnerId].GetControlPoints();

      for (size_t i = 0; i < points.size(); ++i) {
        const bool isSelectedPoint =
            (selectedControlPartId_ == controlOwnerId &&
             selectedControlPointIndex_ == static_cast<int>(i));

        ImGui::PushID(static_cast<int>(i));

        if (isSelectedPoint) {
          ImGui::Text("-> Point %d", static_cast<int>(i));
        } else {
          ImGui::Text("Point %d", static_cast<int>(i));
        }

        Vector3 localPos = points[i].localPosition;
        if (ImGui::SliderFloat3("Local Pos", &localPos.x, -5.0f, 5.0f)) {
          modBodies_[controlOwnerId].MoveControlPoint(i, localPos);
        }

        float radius = points[i].radius;
        if (ImGui::SliderFloat("Radius", &radius, 0.05f, 0.30f, "%.2f")) {
          const float current = (std::max)(points[i].radius, 0.0001f);
          modBodies_[controlOwnerId].ScaleControlPoint(i, radius / current);
        }

        ImGui::Text("Influence Radius : %.2f", points[i].radius);
        ImGui::Text("Movable: %s", points[i].movable ? "true" : "false");

        ImGui::Separator();
        ImGui::PopID();
      }
    }
  }
}

void ModScene::DrawModGui() {
  ImGui::Begin("ModScene");

  ImGui::Text("MouseMiddleDrag : Move Camera");
  ImGui::Text("MouseRightDrag  : Rotate Camera");
  ImGui::Text("MouseLeftDrag   : Move Control Point");
  ImGui::Text("DIK_0           : Toggle DebugCamera");

  DrawAssemblyGui();
  DrawSelectedPartGui();

  ImGui::Separator();
  ImGui::Text("Global Reset");
  if (ImGui::Button("Reset All Part Params")) {
    ResetModBodies();
  }

  if (ImGui::Button("Reset To Default Humanoid")) {
    ResetToDefaultHumanoid();
  }

  ImGui::End();
}

const char *ModScene::ConnectorRoleName(ConnectorRole role) const {
  // 接続点役割を表示名へ変換する
  switch (role) {
  case ConnectorRole::Generic:
    return "Generic";
  case ConnectorRole::Neck:
    return "Neck";
  case ConnectorRole::Shoulder:
    return "Shoulder";
  case ConnectorRole::ArmJoint:
    return "ArmJoint";
  case ConnectorRole::Hip:
    return "Hip";
  case ConnectorRole::LegJoint:
    return "LegJoint";
  default:
    return "Unknown";
  }
}

const char *ModScene::SideName(PartSide side) const {
  // 左右属性を表示名へ変換する
  switch (side) {
  case PartSide::Center:
    return "Center";
  case PartSide::Left:
    return "Left";
  case PartSide::Right:
    return "Right";
  default:
    return "Unknown";
  }
}
#endif

void ModScene::SaveControlPointsToCustomizeData() {
  if (customizeData_ == nullptr) {
    return;
  }

  auto &cp = customizeData_->controlPoints;

  cp.leftShoulderPos =
      GetControlPointLocalPosition(ModControlPointRole::LeftShoulder);
  cp.leftElbowPos =
      GetControlPointLocalPosition(ModControlPointRole::LeftElbow);
  cp.leftWristPos =
      GetControlPointLocalPosition(ModControlPointRole::LeftWrist);

  cp.rightShoulderPos =
      GetControlPointLocalPosition(ModControlPointRole::RightShoulder);
  cp.rightElbowPos =
      GetControlPointLocalPosition(ModControlPointRole::RightElbow);
  cp.rightWristPos =
      GetControlPointLocalPosition(ModControlPointRole::RightWrist);

  cp.leftHipPos = GetControlPointLocalPosition(ModControlPointRole::LeftHip);
  cp.leftKneePos = GetControlPointLocalPosition(ModControlPointRole::LeftKnee);
  cp.leftAnklePos =
      GetControlPointLocalPosition(ModControlPointRole::LeftAnkle);

  cp.rightHipPos = GetControlPointLocalPosition(ModControlPointRole::RightHip);
  cp.rightKneePos =
      GetControlPointLocalPosition(ModControlPointRole::RightKnee);
  cp.rightAnklePos =
      GetControlPointLocalPosition(ModControlPointRole::RightAnkle);

  cp.chestPos = GetControlPointLocalPosition(ModControlPointRole::Chest);
  cp.bellyPos = GetControlPointLocalPosition(ModControlPointRole::Belly);
  cp.waistPos = GetControlPointLocalPosition(ModControlPointRole::Waist);

  cp.lowerNeckPos =
      GetControlPointLocalPosition(ModControlPointRole::LowerNeck);
  cp.upperNeckPos =
      GetControlPointLocalPosition(ModControlPointRole::UpperNeck);
  cp.headCenterPos =
      GetControlPointLocalPosition(ModControlPointRole::HeadCenter);
}

Vector3 ModScene::GetControlPointLocalPosition(ModControlPointRole role) const {
  if (role == ModControlPointRole::LeftShoulder) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::LeftUpperArm) {
        if (modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }

        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }

        return ModObjectUtil::ComputeObjectRootWorldTranslate(object);
      }
    }
  }

  if (role == ModControlPointRole::LeftElbow) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::LeftUpperArm) {
        if (modBodies_.count(id) == 0 || modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }
        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }
        const int bendIndex =
            modBodies_.at(id).FindControlPointIndex(ModControlPointRole::Bend);
        if (bendIndex >= 0) {
          return modBodies_.at(id).GetControlPointWorldPosition(
              object, static_cast<size_t>(bendIndex));
        }
      }
    }
  }

  if (role == ModControlPointRole::LeftWrist) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::LeftUpperArm) {
        if (modBodies_.count(id) == 0 || modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }
        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }
        const int endIndex =
            modBodies_.at(id).FindControlPointIndex(ModControlPointRole::End);
        if (endIndex >= 0) {
          return modBodies_.at(id).GetControlPointWorldPosition(
              object, static_cast<size_t>(endIndex));
        }
      }
    }
  }

  if (role == ModControlPointRole::RightShoulder) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::RightUpperArm) {
        if (modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }

        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }

        return ModObjectUtil::ComputeObjectRootWorldTranslate(object);
      }
    }
  }

  if (role == ModControlPointRole::RightElbow) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::RightUpperArm) {
        if (modBodies_.count(id) == 0 || modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }
        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }
        const int bendIndex =
            modBodies_.at(id).FindControlPointIndex(ModControlPointRole::Bend);
        if (bendIndex >= 0) {
          return modBodies_.at(id).GetControlPointWorldPosition(
              object, static_cast<size_t>(bendIndex));
        }
      }
    }
  }

  if (role == ModControlPointRole::RightWrist) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::RightUpperArm) {
        if (modBodies_.count(id) == 0 || modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }
        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }
        const int endIndex =
            modBodies_.at(id).FindControlPointIndex(ModControlPointRole::End);
        if (endIndex >= 0) {
          return modBodies_.at(id).GetControlPointWorldPosition(
              object, static_cast<size_t>(endIndex));
        }
      }
    }
  }

  if (role == ModControlPointRole::LeftHip) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::LeftThigh) {
        if (modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }

        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }

        return ModObjectUtil::ComputeObjectRootWorldTranslate(object);
      }
    }
  }

  if (role == ModControlPointRole::LeftKnee) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::LeftThigh) {
        if (modBodies_.count(id) == 0 || modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }
        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }
        const int bendIndex =
            modBodies_.at(id).FindControlPointIndex(ModControlPointRole::Bend);
        if (bendIndex >= 0) {
          return modBodies_.at(id).GetControlPointWorldPosition(
              object, static_cast<size_t>(bendIndex));
        }
      }
    }
  }

  if (role == ModControlPointRole::LeftAnkle) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::LeftThigh) {
        if (modBodies_.count(id) == 0 || modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }
        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }
        const int endIndex =
            modBodies_.at(id).FindControlPointIndex(ModControlPointRole::End);
        if (endIndex >= 0) {
          return modBodies_.at(id).GetControlPointWorldPosition(
              object, static_cast<size_t>(endIndex));
        }
      }
    }
  }

  if (role == ModControlPointRole::RightHip) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::RightThigh) {
        if (modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }

        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }

        return ModObjectUtil::ComputeObjectRootWorldTranslate(object);
      }
    }
  }

  if (role == ModControlPointRole::RightKnee) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::RightThigh) {
        if (modBodies_.count(id) == 0 || modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }
        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }
        const int bendIndex =
            modBodies_.at(id).FindControlPointIndex(ModControlPointRole::Bend);
        if (bendIndex >= 0) {
          return modBodies_.at(id).GetControlPointWorldPosition(
              object, static_cast<size_t>(bendIndex));
        }
      }
    }
  }

  if (role == ModControlPointRole::RightAnkle) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::RightThigh) {
        if (modBodies_.count(id) == 0 || modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }
        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }
        const int endIndex =
            modBodies_.at(id).FindControlPointIndex(ModControlPointRole::End);
        if (endIndex >= 0) {
          return modBodies_.at(id).GetControlPointWorldPosition(
              object, static_cast<size_t>(endIndex));
        }
      }
    }
  }

  if (role == ModControlPointRole::Chest) {
    return GetTorsoControlPointWorldPosition(ModControlPointRole::Chest);
  }

  if (role == ModControlPointRole::Belly) {
    return GetTorsoControlPointWorldPosition(ModControlPointRole::Belly);
  }

  if (role == ModControlPointRole::Waist) {
    return GetTorsoControlPointWorldPosition(ModControlPointRole::Waist);
  }

  if (role == ModControlPointRole::LowerNeck ||
      role == ModControlPointRole::UpperNeck ||
      role == ModControlPointRole::HeadCenter) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::Head) {
        if (modBodies_.count(id) == 0 || modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }
        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }

        const int pointIndex = modBodies_.at(id).FindControlPointIndex(role);
        if (pointIndex >= 0) {
          return modBodies_.at(id).GetControlPointWorldPosition(
              object, static_cast<size_t>(pointIndex));
        }
      }
    }
  }

  return {0.0f, 0.0f, 0.0f};
}

bool ModScene::ShouldBlockDebugCameraMouseControl() const {
  // 部位付け替えドラッグ中
  if (assemblyDrag_.isDragging) {
    return true;
  }

  // 操作点ドラッグ中
  if (isDraggingControlPoint_) {
    return true;
  }

  // 操作点を選択中
  if (selectedControlPointIndex_ >= 0) {
    return true;
  }

  // マウスが部位メッシュ上にある
  if (hoveredPartId_ >= 0) {
    return true;
  }

  return false;
}

float ModScene::GetModelLocalVisualRadius(ModBodyPart part) const {
  const int key = static_cast<int>(part);

  auto cacheIt = modelLocalVisualRadiusCache_.find(key);
  if (cacheIt != modelLocalVisualRadiusCache_.end()) {
    return cacheIt->second;
  }

  const std::string path = ModelPath(part);
  const std::vector<ModelData> models = LoadFileTop(path);

  if (models.empty()) {
    modelLocalVisualRadiusCache_[key] = 0.10f;
    return 0.10f;
  }

  const ModelData &model = models[0];

  if (model.vertices.empty()) {
    modelLocalVisualRadiusCache_[key] = 0.10f;
    return 0.10f;
  }

  float minX = FLT_MAX;
  float maxX = -FLT_MAX;
  float minZ = FLT_MAX;
  float maxZ = -FLT_MAX;

  for (size_t i = 0; i < model.vertices.size(); ++i) {
    const Vector4 &pos4 = model.vertices[i].position;
    const Vector3 localPos = {pos4.x, pos4.y, pos4.z};

    const Vector3 transformed =
        TransformPointByMatrixLocal(model.rootNode.localMatrix, localPos);

    if (transformed.x < minX) {
      minX = transformed.x;
    }
    if (transformed.x > maxX) {
      maxX = transformed.x;
    }
    if (transformed.z < minZ) {
      minZ = transformed.z;
    }
    if (transformed.z > maxZ) {
      maxZ = transformed.z;
    }
  }

  const float halfWidthX = (maxX - minX) * 0.5f;
  const float halfWidthZ = (maxZ - minZ) * 0.5f;

  const float radius = (std::max)(halfWidthX, halfWidthZ);
  const float safeRadius = (std::max)(radius, 0.01f);

  modelLocalVisualRadiusCache_[key] = safeRadius;
  return safeRadius;
}

float ModScene::GetAutoCapsuleRadiusScale(ModBodyPart part,
                                          ModControlPointRole startRole,
                                          ModControlPointRole endRole) const {
  const float modelRadius = GetModelLocalVisualRadius(part);
  const float defaultRadius =
      GetDefaultSegmentRadiusForPartRoles(part, startRole, endRole);

  return modelRadius / (std::max)(defaultRadius, 0.0001f);
}

float ModScene::GetAutoTorsoCapsuleRadiusScale(
    ModControlPointRole startRole, ModControlPointRole endRole) const {
  ModBodyPart part = ModBodyPart::ChestBody;

  if (startRole == ModControlPointRole::Belly &&
      endRole == ModControlPointRole::Waist) {
    part = ModBodyPart::StomachBody;
  }

  const float modelRadius = GetModelLocalVisualRadius(part);
  const float defaultRadius =
      GetDefaultSegmentRadiusForPartRoles(part, startRole, endRole);

  return modelRadius / (std::max)(defaultRadius, 0.0001f);
}

float ModScene::GetModelLocalVisualHalfHeight(ModBodyPart part) const {
  const int key = static_cast<int>(part);

  auto cacheIt = modelLocalVisualHalfHeightCache_.find(key);
  if (cacheIt != modelLocalVisualHalfHeightCache_.end()) {
    return cacheIt->second;
  }

  const std::string path = ModelPath(part);
  const std::vector<ModelData> models = LoadFileTop(path);

  if (models.empty()) {
    modelLocalVisualHalfHeightCache_[key] = 0.10f;
    return 0.10f;
  }

  const ModelData &model = models[0];
  if (model.vertices.empty()) {
    modelLocalVisualHalfHeightCache_[key] = 0.10f;
    return 0.10f;
  }

  float minY = FLT_MAX;
  float maxY = -FLT_MAX;

  for (size_t i = 0; i < model.vertices.size(); ++i) {
    const Vector4 &pos4 = model.vertices[i].position;
    const Vector3 localPos = {pos4.x, pos4.y, pos4.z};

    const Vector3 transformed =
        TransformPointByMatrixLocal(model.rootNode.localMatrix, localPos);

    if (transformed.y < minY) {
      minY = transformed.y;
    }
    if (transformed.y > maxY) {
      maxY = transformed.y;
    }
  }

  const float halfHeight = (maxY - minY) * 0.5f;
  const float safeHalfHeight = (std::max)(halfHeight, 0.01f);

  modelLocalVisualHalfHeightCache_[key] = safeHalfHeight;
  return safeHalfHeight;
}

bool ModScene::BuildHeadPickBox(int partId,
                                ModSceneSegmentBoxSet &outBoxes) const {
  outBoxes.count = 0;

  const PartNode *node = assembly_.FindNode(partId);
  if (node == nullptr || node->part != ModBodyPart::Head) {
    return false;
  }

  auto objectIt = modObjects_.find(partId);
  if (objectIt == modObjects_.end() || objectIt->second == nullptr) {
    return false;
  }

  const Object *headObject = objectIt->second.get();
  if (headObject == nullptr) {
    return false;
  }

  ModSceneSegmentBox &box = outBoxes.segments[0];

  const Matrix4x4 world =
      ModObjectUtil::ComputeMainPositionWorldMatrix(headObject);

  Vector3 axisX = {
      world.m[0][0],
      world.m[0][1],
      world.m[0][2],
  };
  Vector3 axisY = {
      world.m[1][0],
      world.m[1][1],
      world.m[1][2],
  };
  Vector3 axisZ = {
      world.m[2][0],
      world.m[2][1],
      world.m[2][2],
  };

  axisX = NormalizeSafeV(axisX, {1.0f, 0.0f, 0.0f});
  axisY = NormalizeSafeV(axisY, {0.0f, 1.0f, 0.0f});
  axisZ = NormalizeSafeV(axisZ, {0.0f, 0.0f, 1.0f});

  box.center = ModObjectUtil::ComputeObjectRootWorldTranslate(headObject);
  box.axisX = axisX;
  box.axisY = axisY;
  box.axisZ = axisZ;

  const float halfWidth = GetModelLocalVisualRadius(ModBodyPart::Head);
  const float halfHeight = GetModelLocalVisualHalfHeight(ModBodyPart::Head);
  const Vector3 scale = modBodies_.count(partId) > 0
                            ? modBodies_.at(partId).GetVisualScaleRatio()
                            : Vector3{1.0f, 1.0f, 1.0f};

  box.halfWidth = (std::max)(halfWidth * scale.x, 0.01f);
  box.halfLength = (std::max)(halfHeight * scale.y, 0.01f);
  box.halfDepth = (std::max)(halfWidth * scale.z, 0.01f);

  outBoxes.count = 1;
  return true;
}

void ModScene::UpdateNpcProgress() {
  const float deltaTime = system_->GetDeltaTime();

  for (size_t i = 0; i < npcProgress_.size(); ++i) {
    NpcModProgress &npc = npcProgress_[i];

    if (npc.hasReachedGoal) {
      continue;
    }

    if (npc.hasStartedMoving) {
      npc.moveElapsedTime += deltaTime;

      if (IsNpcReachedGoalInModScene(npc)) {
        npc.hasReachedGoal = true;

        if (!npc.goalNotified) {
          npc.goalNotified = true;
          npcGoalCountInMod_++;
          AddStartNotification(npc.name + "がゴール！");
        }
      }

      continue;
    }

    npc.elapsedTime += deltaTime;

    if (!npc.isFinished && npc.elapsedTime >= npc.totalTime) {
      npc.elapsedTime = npc.totalTime;
      npc.isFinished = true;
      npc.hasStartedMoving = true;
      npc.moveElapsedTime = 0.0f;

      AddStartNotification(npc.name + "がスタート！");
    }
  }

  CheckModFailureState();
  UpdateNotifications();
}

void ModScene::AddStartNotification(const std::string &text) {
  StartNotification notification{};
  notification.text = text;
  notification.timer = 0.0f;
  notification.duration = 1.5f;
  notification.startY = 80.0f;

  notifications_.push_back(notification);
}

void ModScene::UpdateNotifications() {
  const float deltaTime = system_->GetDeltaTime();

  std::vector<StartNotification> aliveNotifications;
  aliveNotifications.reserve(notifications_.size());

  for (size_t i = 0; i < notifications_.size(); ++i) {
    StartNotification notification = notifications_[i];
    notification.timer += deltaTime;

    if (notification.timer < notification.duration) {
      aliveNotifications.push_back(notification);
    }
  }

  notifications_ = std::move(aliveNotifications);
}

void ModScene::InitializeNpcModProgress() {
  npcProgress_.clear();
  npcProgress_.reserve(4);

  std::vector<NpcPresetType> presetPool = {
      NpcPresetType::Default, NpcPresetType::HeadBig, NpcPresetType::BigTorso,
      NpcPresetType::LongLeg};

  std::random_device rd;
  std::mt19937 rng(rd());
  std::shuffle(presetPool.begin(), presetPool.end(), rng);

  for (int i = 0; i < 4; ++i) {
    NpcModProgress npc{};
    npc.name = "NPC" + std::to_string(i + 1);

    npc.presetType = presetPool[i];
    npc.skillMultiplier = GetNpcSkillMultiplierByIndex(i);
    npc.runTimingSkill = GetNpcRunTimingSkillByIndex(i);

    npc.totalTime = CalculateNpcModTime(npc.presetType, npc.skillMultiplier);
    npc.elapsedTime = 0.0f;
    npc.isFinished = false;
    npc.hasStartedMoving = false;
    npc.moveElapsedTime = 0.0f;

    npc.hasReachedGoal = false;
    npc.goalNotified = false;

    npcProgress_.push_back(npc);
  }

  notifications_.clear();

  npcGoalCountInMod_ = 0;
  isModFailed_ = false;
  isFailureMenuOpen_ = false;
  failureNotified_ = false;
  pendingFailureOutcome_ = SceneOutcome::NONE;
  selectedRetryChoiceMod_ = RetryChoiceMod::RetryMod;
  failureMenuInputCooldown_ = 0.0f;
}

void ModScene::DrawStartNotifications() {
  if (notifications_.empty()) {
    return;
  }

  const float baseX = 900.0f;
  const float fontSize = 32.0f;
  const float fallDistance = 80.0f;

  for (size_t i = 0; i < notifications_.size(); ++i) {
    const StartNotification &notification = notifications_[i];

    float t = 0.0f;
    if (notification.duration > 0.0f) {
      t = notification.timer / notification.duration;
    }
    t = std::clamp(t, 0.0f, 1.0f);

    float x = baseX;
    float y = notification.startY + fallDistance * t;
    float alpha = 1.0f - t;

    bitmapFont_.RenderText(notification.text, {x, y}, fontSize,
                           BitmapFont::Align::Left, 5.0f,
                           {1.0f, 1.0f, 1.0f, alpha});
  }
}

void ModScene::SyncNpcProgressToCustomizeData() {
  if (customizeData_ == nullptr) {
    return;
  }

  customizeData_->npcStartProgressList.clear();
  customizeData_->npcStartProgressList.reserve(npcProgress_.size());

  for (size_t i = 0; i < npcProgress_.size(); ++i) {
    const NpcModProgress &src = npcProgress_[i];

    NpcStartProgressData dst;
    dst.name = src.name;
    dst.totalTime = src.totalTime;
    dst.elapsedTime = src.elapsedTime;
    dst.isFinished = src.isFinished;
    dst.hasStartedMoving = src.hasStartedMoving;
    dst.moveElapsedTime = src.moveElapsedTime;
    dst.presetType = src.presetType;
    dst.runTimingSkill = src.runTimingSkill;

    customizeData_->npcStartProgressList.push_back(dst);
  }
}

float ModScene::CalculateNpcModTime(NpcPresetType presetType,
                                    float skillMultiplier) const {
  const float baseTime = GetNpcBaseModTime();
  const float presetBonus = GetNpcPresetTimeBonus(presetType);

  return (baseTime + presetBonus) / skillMultiplier;
}

float ModScene::GetNpcBaseModTime() const { return 15.0f; }

float ModScene::GetNpcPresetTimeBonus(NpcPresetType presetType) const {
  switch (presetType) {
  case NpcPresetType::Default:
    return 0.0f;

  case NpcPresetType::HeadBig:
    return 1.5f;

  case NpcPresetType::LongLeg:
    return 3.0f;

  case NpcPresetType::BigTorso:
    return 2.0f;

  default:
    return 0.0f;
  }
}

float ModScene::GetNpcSkillMultiplierByIndex(int index) const {
  switch (index) {
  case 0:
    return 1.00f;

  case 1:
    return 0.95f;

  case 2:
    return 1.10f;

  default:
    return 0.90f;
  }
}

float ModScene::GetNpcRunTimingSkillByIndex(int index) const {
  switch (index) {
  case 0:
    return 1.15f;

  case 1:
    return 1.05f;

  case 2:
    return 0.95f;

  case 3:
    return 0.85f;

  default:
    return 1.0f;
  }
}

bool ModScene::IsNpcReachedGoalInModScene(const NpcModProgress &npc) const {
  // 暫定実装
  // ModScene には走行距離そのものが無いので、
  // 先行移動時間が一定以上なら「もうゴール済み」とみなす
  return npc.hasStartedMoving && npc.moveElapsedTime >= modNpcGoalLeadTime_;
}

void ModScene::OpenFailureMenuMod() {
  isModFailed_ = true;
  isFailureMenuOpen_ = true;
  failureMenuInputCooldown_ = 0.15f;
  selectedRetryChoiceMod_ = RetryChoiceMod::RetryMod;

  // 失敗通知を消す
  notifications_.clear();
}

void ModScene::DecideFailureMenuMod() {
  if (fade_.IsBusy() || isStartTransition_) {
    return;
  }

  switch (selectedRetryChoiceMod_) {
  case RetryChoiceMod::BackToPrompt:
    pendingFailureOutcome_ = SceneOutcome::RETURN_PROMPT;
    break;

  case RetryChoiceMod::RetryMod:
    pendingFailureOutcome_ = SceneOutcome::RETRY;
    break;

  default:
    pendingFailureOutcome_ = SceneOutcome::NONE;
    break;
  }

  if (pendingFailureOutcome_ != SceneOutcome::NONE) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
  }
}

void ModScene::UpdateFailureMenuInputMod() {
  if (!isFailureMenuOpen_) {
    return;
  }

  const float dt = system_->GetDeltaTime();
  if (failureMenuInputCooldown_ > 0.0f) {
    failureMenuInputCooldown_ -= dt;
    if (failureMenuInputCooldown_ < 0.0f) {
      failureMenuInputCooldown_ = 0.0f;
    }
  }

  const Vector2 mouse = system_->GetMousePosVector2();

  struct MenuRect {
    Vector2 center;
    Vector2 size;
  };

  const MenuRect promptRect{{640.0f, 330.0f}, {420.0f, 64.0f}};
  const MenuRect retryRect{{640.0f, 410.0f}, {420.0f, 64.0f}};

  auto IsInside = [](const Vector2 &p, const MenuRect &r) -> bool {
    const float left = r.center.x - r.size.x * 0.5f;
    const float right = r.center.x + r.size.x * 0.5f;
    const float top = r.center.y - r.size.y * 0.5f;
    const float bottom = r.center.y + r.size.y * 0.5f;
    return p.x >= left && p.x <= right && p.y >= top && p.y <= bottom;
  };

  if (IsInside(mouse, promptRect)) {
    selectedRetryChoiceMod_ = RetryChoiceMod::BackToPrompt;
  } else if (IsInside(mouse, retryRect)) {
    selectedRetryChoiceMod_ = RetryChoiceMod::RetryMod;
  }

  if (failureMenuInputCooldown_ <= 0.0f) {
    if (system_->GetTriggerOn(DIK_UP) || system_->GetTriggerOn(DIK_W)) {
      if (selectedRetryChoiceMod_ == RetryChoiceMod::RetryMod) {
        selectedRetryChoiceMod_ = RetryChoiceMod::BackToPrompt;
      }
      failureMenuInputCooldown_ = 0.12f;
    }

    if (system_->GetTriggerOn(DIK_DOWN) || system_->GetTriggerOn(DIK_S)) {
      if (selectedRetryChoiceMod_ == RetryChoiceMod::BackToPrompt) {
        selectedRetryChoiceMod_ = RetryChoiceMod::RetryMod;
      }
      failureMenuInputCooldown_ = 0.12f;
    }

    const bool mouseClicked = IsMouseLeftTriggeredNow();
    const bool keyConfirm =
        system_->GetTriggerOn(DIK_RETURN) || system_->GetTriggerOn(DIK_SPACE);

    if (mouseClicked || keyConfirm) {
      DecideFailureMenuMod();
    }
  }
}

void ModScene::DrawFailureMenuMod() {
  if (!isFailureMenuOpen_) {
    return;
  }

  const Vector4 normalColor = {1.0f, 1.0f, 1.0f, 1.0f};
  const Vector4 selectedColor = {1.0f, 1.0f, 0.2f, 1.0f};

  bitmapFont_.RenderText("しっぱい", {640.0f, 220.0f}, 72.0f,
                         BitmapFont::Align::Center, 5.0f,
                         {1.0f, 0.35f, 0.35f, 1.0f});

  const bool promptSelected =
      selectedRetryChoiceMod_ == RetryChoiceMod::BackToPrompt;
  const bool retrySelected =
      selectedRetryChoiceMod_ == RetryChoiceMod::RetryMod;

  bitmapFont_.RenderText("おだいにもどる", {640.0f, 330.0f},
                         promptSelected ? 44.0f : 36.0f,
                         BitmapFont::Align::Center, 5.0f,
                         promptSelected ? selectedColor : normalColor);

  bitmapFont_.RenderText("かいぞうからやりなおす", {640.0f, 410.0f},
                         retrySelected ? 44.0f : 36.0f,
                         BitmapFont::Align::Center, 5.0f,
                         retrySelected ? selectedColor : normalColor);
}

void ModScene::CheckModFailureState() {
  if (isModFailed_) {
    return;
  }

  if (npcGoalCountInMod_ >= modFailureGoalCount_) {
    OpenFailureMenuMod();
  }
}

void ModScene::SetupUiSprite(UiIconButton &button, const Vector2 &center,
                             const Vector2 &size, int textureHandle) {
  button.center = center;
  button.size = size;
  button.textureHandle = textureHandle;
  button.visible = true;

  button.sprite = std::make_unique<SimpleSprite>();
  button.sprite->IntObject(system_);
  button.sprite->CreateDefaultData();

  if (!button.sprite->objectParts_.empty()) {
    auto &part = button.sprite->objectParts_[0];

    part.materialConfig->textureHandle = textureHandle;
    part.materialConfig->useModelTexture = false;
    part.materialConfig->enableLighting = false;
    part.materialConfig->textureColor = MakeColor(1.0f, 1.0f, 1.0f, 1.0f);

    part.cropLT = {0.0f, 0.0f};
    part.cropSize = {0.0f, 0.0f};

    button.sprite->mainPosition.transform = CreateDefaultTransform();
    button.sprite->mainPosition.transform.translate = {
        center.x - size.x * 0.5f, center.y - size.y * 0.5f, 0.0f};

    button.sprite->mainPosition.transform.scale = {1.0f, 1.0f, 1.0f};

    part.anchorPoint = {0.0f, 0.0f};

    part.conerData.coner[0] = {0.0f, 0.0f};
    part.conerData.coner[1] = {0.0f, size.y};
    part.conerData.coner[2] = {size.x, size.y};
    part.conerData.coner[3] = {size.x, 0.0f};

    part.transform.translate = {0.0f, 0.0f, 0.0f};
    part.transform.rotate = {0.0f, 0.0f, 0.0f};
    part.transform.scale = {1.0f, 1.0f, 1.0f};
  }
}

void ModScene::UpdateUiSpriteTransform(UiIconButton &button) {
  if (button.sprite == nullptr || button.sprite->objectParts_.empty()) {
    return;
  }

  button.sprite->mainPosition.transform.translate = {
      button.center.x - button.size.x * 0.5f,
      button.center.y - button.size.y * 0.5f, 0.0f};

  button.sprite->mainPosition.transform.scale = {1.0f, 1.0f, 1.0f};

  auto &part = button.sprite->objectParts_[0];
  part.conerData.coner[0] = {0.0f, 0.0f};
  part.conerData.coner[1] = {0.0f, button.size.y};
  part.conerData.coner[2] = {button.size.x, button.size.y};
  part.conerData.coner[3] = {button.size.x, 0.0f};

  part.transform.translate = {0.0f, 0.0f, 0.0f};
  part.transform.rotate = {0.0f, 0.0f, 0.0f};
  part.transform.scale = {1.0f, 1.0f, 1.0f};
}

bool ModScene::IsPointInUiButton(const Vector2 &point,
                                 const UiIconButton &button) const {
  if (!button.visible) {
    return false;
  }
  return PointInRect(point, button.center, button.size);
}

void ModScene::InitializeScreenUi() {
  // フレーム画像
  uiFrameTextureHandle_ =
      system_->LoadTexture("GAME/resources/texture/frame.png");

  // ゴミ箱画像
  trashTextureHandle_ =
      system_->LoadTexture("GAME/resources/texture/trash.png");

  const float left = 110.0f;
  const float top = 40.0f;
  const float width = 220.0f;
  const float height = 44.0f;
  const float spacingY = 52.0f;

  SetupUiSprite(addButtons_[static_cast<size_t>(UiAddButtonType::AddHeadSet)],
                {left, top + spacingY * 0.0f}, {width, height},
                uiFrameTextureHandle_);
  addButtons_[static_cast<size_t>(UiAddButtonType::AddHeadSet)].label =
      "あたま";

  SetupUiSprite(addButtons_[static_cast<size_t>(UiAddButtonType::AddRightArm)],
                {left, top + spacingY * 1.0f}, {width, height},
                uiFrameTextureHandle_);
  addButtons_[static_cast<size_t>(UiAddButtonType::AddRightArm)].label =
      "みぎうで";

  SetupUiSprite(addButtons_[static_cast<size_t>(UiAddButtonType::AddLeftArm)],
                {left, top + spacingY * 2.0f}, {width, height},
                uiFrameTextureHandle_);
  addButtons_[static_cast<size_t>(UiAddButtonType::AddLeftArm)].label =
      "ひだりうで";

  SetupUiSprite(addButtons_[static_cast<size_t>(UiAddButtonType::AddRightLeg)],
                {left, top + spacingY * 3.0f}, {width, height},
                uiFrameTextureHandle_);
  addButtons_[static_cast<size_t>(UiAddButtonType::AddRightLeg)].label =
      "みぎあし";

  SetupUiSprite(addButtons_[static_cast<size_t>(UiAddButtonType::AddLeftLeg)],
                {left, top + spacingY * 4.0f}, {width, height},
                uiFrameTextureHandle_);
  addButtons_[static_cast<size_t>(UiAddButtonType::AddLeftLeg)].label =
      "ひだりあし";

  SetupUiSprite(addButtons_[static_cast<size_t>(UiAddButtonType::AddBody)],
                {left, top + spacingY * 5.0f}, {width, height},
                uiFrameTextureHandle_);
  addButtons_[static_cast<size_t>(UiAddButtonType::AddBody)].label = "からだ";

  SetupUiSprite(trashButton_, {1180.0f, 620.0f}, {84.0f, 84.0f},
                trashTextureHandle_);
  trashButton_.label = "ごみばこ";
}

bool ModScene::ExecuteAddButton(UiAddButtonType type) {
  bool changed = false;

  switch (type) {
  case UiAddButtonType::AddLeftArm:
    changed = assembly_.AddArmAssembly(PartSide::Left);
    break;

  case UiAddButtonType::AddRightArm:
    changed = assembly_.AddArmAssembly(PartSide::Right);
    break;

  case UiAddButtonType::AddLeftLeg:
    changed = assembly_.AddLegAssembly(PartSide::Left);
    break;

  case UiAddButtonType::AddRightLeg:
    changed = assembly_.AddLegAssembly(PartSide::Right);
    break;

  case UiAddButtonType::AddHeadSet:
    changed = assembly_.AddNeckPart();
    break;

  case UiAddButtonType::AddBody:
    changed = assembly_.AddBodyPart();
    break;

  default:
    break;
  }

  if (changed) {
    SyncAfterAssemblyChanged();
  }

  return changed;
}

bool ModScene::TryHandleAddButtonClick() {
  if (!IsMouseLeftTriggeredNow()) {
    return false;
  }

  const Vector2 mouse = system_->GetMousePosVector2();

  for (size_t i = 0; i < addButtons_.size(); ++i) {
    if (!IsPointInUiButton(mouse, addButtons_[i])) {
      continue;
    }

    ExecuteAddButton(static_cast<UiAddButtonType>(i));
    return true;
  }

  return false;
}

bool ModScene::IsMouseOverAnyScreenUi() const {
  const Vector2 mouse = system_->GetMousePosVector2();

  for (size_t i = 0; i < addButtons_.size(); ++i) {
    if (IsPointInUiButton(mouse, addButtons_[i])) {
      return true;
    }
  }

  if (IsPointInUiButton(mouse, trashButton_)) {
    return true;
  }

  return false;
}

bool ModScene::IsMouseOverTrashArea() const {
  const Vector2 mouse = system_->GetMousePosVector2();
  return IsPointInUiButton(mouse, trashButton_);
}

void ModScene::UpdateScreenUi() {
  for (size_t i = 0; i < addButtons_.size(); ++i) {
    UpdateUiSpriteTransform(addButtons_[i]);
  }

  UpdateUiSpriteTransform(trashButton_);

  isHoverTrash_ = assemblyDrag_.isDragging && IsMouseOverTrashArea();

  for (size_t i = 0; i < addButtons_.size(); ++i) {
    if (addButtons_[i].sprite == nullptr ||
        addButtons_[i].sprite->objectParts_.empty()) {
      continue;
    }

    Vector4 &color =
        addButtons_[i].sprite->objectParts_[0].materialConfig->textureColor;
    color = MakeColor(1.0f, 1.0f, 1.0f, 1.0f);

    if (IsPointInUiButton(system_->GetMousePosVector2(), addButtons_[i])) {
      color = MakeColor(0.8f, 1.0f, 0.8f, 1.0f);
    }
  }

  if (trashButton_.sprite != nullptr &&
      !trashButton_.sprite->objectParts_.empty()) {
    Vector4 &color =
        trashButton_.sprite->objectParts_[0].materialConfig->textureColor;
    color = isHoverTrash_ ? MakeColor(1.0f, 0.45f, 0.45f, 1.0f)
                          : MakeColor(1.0f, 1.0f, 1.0f, 1.0f);
  }
}

void ModScene::DrawScreenUi() {
  for (size_t i = 0; i < addButtons_.size(); ++i) {
    if (addButtons_[i].visible && addButtons_[i].sprite != nullptr) {
      addButtons_[i].sprite->Draw();
    }
  }

  if (trashButton_.visible && trashButton_.sprite != nullptr) {
    trashButton_.sprite->Draw();
  }

  const float fontHeight = 28.0f;
  const float textPaddingX = 14.0f;
  const float textPaddingY = 8.0f;

  for (size_t i = 0; i < addButtons_.size(); ++i) {
    const UiIconButton &button = addButtons_[i];
    if (!button.visible) {
      continue;
    }

    const float left = button.center.x - button.size.x * 0.5f;
    const float top = button.center.y - button.size.y * 0.5f;

    Vector4 textColor = {1.0f, 1.0f, 1.0f, 1.0f};
    if (IsPointInUiButton(system_->GetMousePosVector2(), button)) {
      textColor = {1.0f, 0.95f, 0.65f, 1.0f};
    }

    bitmapFont_.RenderText(
        button.label, {left + textPaddingX, top + textPaddingY}, fontHeight,
        BitmapFont::Align::Left, 5.0f, textColor);
  }

  if (trashButton_.visible) {
    const float left = trashButton_.center.x - trashButton_.size.x * 0.5f;
    const float top =
        trashButton_.center.y + trashButton_.size.y * 0.5f - 10.0f;

    bitmapFont_.RenderText("ごみばこ", {left - 8.0f, top}, 20.0f,
                           BitmapFont::Align::Left, 5.0f,
                           {1.0f, 1.0f, 1.0f, 1.0f});
  }
}

bool ModScene::DeleteDraggingAssemblyByTrashDrop() {
  if (!assemblyDrag_.isDragging) {
    return false;
  }

  if (!IsMouseOverTrashArea()) {
    return false;
  }

  const int rootPartId = assemblyDrag_.assemblyRootPartId;
  if (rootPartId < 0) {
    assemblyDrag_.Clear();
    return false;
  }

  const int deleteTargetId = ResolveAssemblyOperationPartId(rootPartId);
  assemblyDrag_.Clear();

  if (deleteTargetId < 0) {
    return false;
  }

  if (!assembly_.RemovePart(deleteTargetId)) {
    return false;
  }

  selectedPartId_ = deleteTargetId;
  SyncAfterAssemblyChanged();
  return true;
}

void ModScene::SyncAfterAssemblyChanged() {
  SyncObjectsWithAssembly();
  LoadCustomizeData();
  EnsureValidSelection();
  ClearControlPointSelection();
}
