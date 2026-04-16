#include "ModBody.h"
#include "ModCustomizeDataStore.h"
#include "GAME/actor/ModObjectUtil.h"
#include <cmath>

namespace {

size_t ToIndex(ModBodyPart part) { return static_cast<size_t>(part); }

Vector3 NormalizeSafe(const Vector3 &v, const Vector3 &fallback) {
  const float len = Length(v);
  if (len < 0.0001f) {
    return fallback;
  }

  const float inv = 1.0f / len;
  return {v.x * inv, v.y * inv, v.z * inv};
}

Vector3 LerpV(const Vector3 &a, const Vector3 &b, float t) {
  return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}

float AngleZFromMinusY(const Vector3 &dir) { return std::atan2(dir.x, -dir.y); }

Vector3 ZeroV() { return {0.0f, 0.0f, 0.0f}; }

bool GetVisualSegmentRoles(ModBodyPart part, ModControlPointRole &startRole,
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

float ClampFloat(float value, float minValue, float maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

bool IsLegControlPart(ModBodyPart part) {
  return part == ModBodyPart::LeftThigh || part == ModBodyPart::LeftShin ||
         part == ModBodyPart::RightThigh || part == ModBodyPart::RightShin;
}

bool IsArmControlPart(ModBodyPart part) {
  return part == ModBodyPart::LeftUpperArm ||
         part == ModBodyPart::LeftForeArm ||
         part == ModBodyPart::RightUpperArm ||
         part == ModBodyPart::RightForeArm;
}

float GetDefaultPointRadius(ModBodyPart part, ModControlPointRole role) {
  switch (role) {
  case ModControlPointRole::Chest:
    return 0.12f;
  case ModControlPointRole::Belly:
    return 0.10f;
  case ModControlPointRole::Waist:
    return 0.12f;
  case ModControlPointRole::Root:
    if (part == ModBodyPart::Neck) {
      return 0.09f;
    }
    return IsLegControlPart(part) ? 0.10f : 0.09f;
  case ModControlPointRole::Bend:
    if (part == ModBodyPart::Neck || part == ModBodyPart::Head) {
      return 0.08f;
    }
    return IsLegControlPart(part) ? 0.09f : 0.08f;
  case ModControlPointRole::End:
    if (part == ModBodyPart::Neck || part == ModBodyPart::Head) {
      return 0.11f;
    }
    return IsLegControlPart(part) ? 0.09f : 0.08f;
  case ModControlPointRole::None:
  default:
    return 0.08f;
  }
}

Vector3 GetLateralOutwardDirection(ModBodyPart part) {
  switch (part) {
  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::LeftForeArm:
  case ModBodyPart::LeftThigh:
  case ModBodyPart::LeftShin:
    return {-1.0f, 0.0f, 0.0f};

  case ModBodyPart::RightUpperArm:
  case ModBodyPart::RightForeArm:
  case ModBodyPart::RightThigh:
  case ModBodyPart::RightShin:
    return {1.0f, 0.0f, 0.0f};

  default:
    return {0.0f, 0.0f, 0.0f};
  }
}

float GetDefaultSegmentRadius(ModBodyPart part, ModControlPointRole startRole,
                              ModControlPointRole endRole) {
  const float startDefault = GetDefaultPointRadius(part, startRole);
  const float endDefault = GetDefaultPointRadius(part, endRole);
  return (startDefault + endDefault) * 0.5f;
}

Vector3 MakePartScale(const Vector3 &baseScale, const ModBodyPartParam &param) {
  Vector3 scale = baseScale;
  scale.x *= param.scale.x;
  scale.y *= param.scale.y * param.length;
  scale.z *= param.scale.z;

  if (!param.enabled) {
    scale = {0.0f, 0.0f, 0.0f};
  }

  return scale;
}

Vector3 GetAnchorOffset(ModBodyPart part, const Vector3 &newScale) {
  switch (part) {
  case ModBodyPart::Neck:
  case ModBodyPart::Head:
    return {0.0f, newScale.y * 0.5f, 0.0f};

  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightUpperArm:
  case ModBodyPart::RightForeArm:
  case ModBodyPart::LeftThigh:
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightThigh:
  case ModBodyPart::RightShin:
    return {0.0f, -newScale.y * 0.5f, 0.0f};

  case ModBodyPart::ChestBody:
  case ModBodyPart::StomachBody:
  case ModBodyPart::Count:
  default:
    return {0.0f, 0.0f, 0.0f};
  }
}

ModControlPoint MakePoint(ModControlPointRole role,
                          const Vector3 &localPosition, float radius,
                          bool movable, bool isConnectionPoint,
                          bool acceptsParent, bool acceptsChild) {
  ModControlPoint point{};
  point.role = role;
  point.localPosition = localPosition;
  point.radius = radius;
  point.movable = movable;
  point.isConnectionPoint = isConnectionPoint;
  point.acceptsParent = acceptsParent;
  point.acceptsChild = acceptsChild;
  return point;
}

int FindRoleIndexInPoints(const std::vector<ModControlPoint> &points,
                          ModControlPointRole role) {
  for (size_t i = 0; i < points.size(); ++i) {
    if (points[i].role == role) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

Vector3 ComputeMainPositionWorldTranslate(const Object *target) {
  if (target == nullptr) {
    return {0.0f, 0.0f, 0.0f};
  }

  Vector3 world = target->mainPosition.transform.translate;

  const ObjectPart *parent = target->mainPosition.parentPart;
  while (parent != nullptr) {
    world = Add(world, parent->transform.translate);
    parent = parent->parentPart;
  }

  return world;
}

void ClearLegacyPartParams(ModBodyCustomizeData &data) {
  for (size_t i = 0; i < static_cast<size_t>(ModBodyPart::Count); ++i) {
    data.partParams[i].scale = {1.0f, 1.0f, 1.0f};
    data.partParams[i].length = 1.0f;
    data.partParams[i].count = 0;
    data.partParams[i].enabled = false;
  }
}
Vector3 RotateByEulerXYZLocal(const Vector3 &point, const Vector3 &rot) {
  const float cx = cosf(rot.x);
  const float sx = sinf(rot.x);
  const float cy = cosf(rot.y);
  const float sy = sinf(rot.y);
  const float cz = cosf(rot.z);
  const float sz = sinf(rot.z);

  Vector3 v = point;

  {
    const float y = v.y * cx - v.z * sx;
    const float z = v.y * sx + v.z * cx;
    v.y = y;
    v.z = z;
  }

  {
    const float x = v.x * cy + v.z * sy;
    const float z = -v.x * sy + v.z * cy;
    v.x = x;
    v.z = z;
  }

  {
    const float x = v.x * cz - v.y * sz;
    const float y = v.x * sz + v.y * cz;
    v.x = x;
    v.y = y;
  }

  return v;
}

Vector3 InverseRotateByEulerXYZLocal(const Vector3 &point, const Vector3 &rot) {
  return RotateByEulerXYZLocal(point, {-rot.x, -rot.y, -rot.z});
}

void DebugLogVector3(const char *label, const Vector3 &v) {
  Logger::Log("%s=(%.3f, %.3f, %.3f)", label, v.x, v.y, v.z);
}

} // namespace

void ModBody::Initialize(Object *target, ModBodyPart part) {
  part_ = part;
  CacheBaseTransforms(target);
  Reset();
  ResetControlPoints();
}

void ModBody::Reset() {
  param_.scale = {1.0f, 1.0f, 1.0f};
  param_.length = 1.0f;
  param_.count = 1;
  param_.enabled = true;
}

Vector3 ModBody::GetVisualScaleRatio() const {
  Vector3 ratio = {param_.scale.x, param_.scale.y * param_.length,
                   param_.scale.z};

  if (!param_.enabled) {
    return {0.0f, 0.0f, 0.0f};
  }

  return ratio;
}

float ModBody::GetVisualSegmentRadius(ModControlPointRole startRole,
                                      ModControlPointRole endRole) const {
  const int startIndex = FindControlPointIndex(startRole);
  const int endIndex = FindControlPointIndex(endRole);

  if (startIndex < 0 || endIndex < 0) {
    return 0.0f;
  }

  const float startRadius =
      controlPoints_[static_cast<size_t>(startIndex)].radius;
  const float endRadius = controlPoints_[static_cast<size_t>(endIndex)].radius;

  const float safeStartRadius = (std::max)(startRadius, 0.01f);
  const float safeEndRadius = (std::max)(endRadius, 0.01f);

  // ApplySegmentToObjectPart() と同じ考え方
  const float segmentRadius = (std::max)(safeStartRadius, safeEndRadius);

  const float defaultSegmentRadius =
      GetDefaultSegmentRadius(part_, startRole, endRole);

  const float thicknessScale =
      segmentRadius / (std::max)(defaultSegmentRadius, 0.0001f);

  // 実際の見た目幅は base mesh の太さ × thicknessScale × param.scale
  // カプセル半径は最終的な見た目の外半径に合わせたいので
  // X/Z の大きい方を掛ける
  const float lateralScale = (std::max)(param_.scale.x, param_.scale.z);

  return defaultSegmentRadius * thicknessScale * lateralScale;
}

std::unique_ptr<ModBodyCustomizeData> ModBody::CreateDefaultCustomizeData() {
  return ModCustomizeDataStore::CreateDefaultCustomizeData();
}

std::unique_ptr<ModBodyCustomizeData> ModBody::CopySharedCustomizeData() {
  return ModCustomizeDataStore::CopySharedCustomizeData();
}

void ModBody::SetSharedCustomizeData(const ModBodyCustomizeData &data) {
  ModCustomizeDataStore::SetSharedCustomizeData(data);
}

const ModBodyCustomizeData *ModBody::GetSharedCustomizeData() {
  return ModCustomizeDataStore::GetSharedCustomizeData();
}

void ModBody::NormalizeCustomizeData(ModBodyCustomizeData &data) {
  ModCustomizeDataStore::NormalizeCustomizeData(data);
}

bool ModBody::HasOwnControlPoints() const {
  switch (part_) {
  case ModBodyPart::Neck:
  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::RightUpperArm:
  case ModBodyPart::LeftThigh:
  case ModBodyPart::RightThigh:
    return true;

  case ModBodyPart::Head:
  case ModBodyPart::ChestBody:
  case ModBodyPart::StomachBody:
  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
  case ModBodyPart::Count:
  default:
    return false;
  }
}

void ModBody::SetExternalSegmentSource(
    const std::vector<ModControlPoint> *points, ModControlPointRole startRole,
    ModControlPointRole endRole) {
  externalControlPoints_ = points;
  externalStartRole_ = startRole;
  externalEndRole_ = endRole;
}

void ModBody::ClearExternalSegmentSource() {
  externalControlPoints_ = nullptr;
  externalStartRole_ = ModControlPointRole::None;
  externalEndRole_ = ModControlPointRole::None;
}

void ModBody::CacheBaseTransforms(Object *target) {
  if (target == nullptr) {
    return;
  }

  if (target->objectParts_.empty()) {
    return;
  }

  // 初回だけ root と全 mesh の transform を保存する
  if (!isBaseCached_) {
    baseMainTransform_ = target->mainPosition.transform;
    baseMeshTransform_ = target->objectParts_[0].transform;

    basePartTransforms_.clear();
    basePartTransforms_.reserve(target->objectParts_.size());
    for (size_t i = 0; i < target->objectParts_.size(); ++i) {
      basePartTransforms_.push_back(target->objectParts_[i].transform);
    }

    isBaseCached_ = true;
  }
}

void ModBody::Apply(Object *target) {
  if (target == nullptr) {
    return;
  }

  if (target->objectParts_.empty()) {
    return;
  }

  CacheBaseTransforms(target);
  UpdateControlPointHierarchy();

  // root は接続基準なので scale を固定する
  Transform root = target->mainPosition.transform;
  root.scale = baseMainTransform_.scale;
  target->mainPosition.transform = root;

  // 無効部位は全 mesh を消す
  if (!param_.enabled) {
    for (size_t i = 0; i < target->objectParts_.size(); ++i) {
      target->objectParts_[i].transform = basePartTransforms_[i];
      target->objectParts_[i].transform.scale = ZeroV();
    }
    return;
  }

  // 通常部位、または外部点列参照部位の描画
  const std::vector<ModControlPoint> *sourcePoints = &controlPoints_;
  ModControlPointRole startRole = ModControlPointRole::None;
  ModControlPointRole endRole = ModControlPointRole::None;

  const bool useExternal = (externalControlPoints_ != nullptr);

  if (useExternal) {
    sourcePoints = externalControlPoints_;
    startRole = externalStartRole_;
    endRole = externalEndRole_;
  } else {
    if (!GetVisualSegmentRoles(part_, startRole, endRole)) {
      ApplySingleMeshFallback(target, baseMeshTransform_, basePartTransforms_,
                              part_, param_);
      return;
    }
  }

  if (sourcePoints == nullptr || sourcePoints->empty()) {
    ApplySingleMeshFallback(target, baseMeshTransform_, basePartTransforms_,
                            part_, param_);
    return;
  }

  const int startIndex = FindRoleIndexInPoints(*sourcePoints, startRole);
  const int endIndex = FindRoleIndexInPoints(*sourcePoints, endRole);

  if (startIndex < 0 || endIndex < 0) {
    ApplySingleMeshFallback(target, baseMeshTransform_, basePartTransforms_,
                            part_, param_);
    return;
  }

  Vector3 startPos =
      (*sourcePoints)[static_cast<size_t>(startIndex)].localPosition;
  Vector3 endPos = (*sourcePoints)[static_cast<size_t>(endIndex)].localPosition;

  const float startRadius =
      (*sourcePoints)[static_cast<size_t>(startIndex)].radius;
  const float endRadius = (*sourcePoints)[static_cast<size_t>(endIndex)].radius;

  // 外部点列参照部位の描画:
  // - 前腕/脛/首などは「start基準（開始点を原点化）」でも良い
  // - 体（Chest/Stomach）は3点を同列に扱いたいので原点化しない
  if (useExternal) {
    const bool isTorso =
        (part_ == ModBodyPart::ChestBody || part_ == ModBodyPart::StomachBody);

    if (!isTorso) {
      startPos = ConvertExternalPointToThisObjectLocal(target, startPos);
      endPos = ConvertExternalPointToThisObjectLocal(target, endPos);
    }
  }

  ApplySegmentToObjectPart(target, 0, startPos, endPos, startRadius, endRadius);

  // 2つ目以降の mesh は使わない
  HideUnusedMeshes(target, basePartTransforms_, 1);
}

void ModBody::ResetControlPoints() {
  BuildDefaultControlPoints();

  chain_.Clear();

  switch (part_) {
  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::RightUpperArm:
    chain_.BuildArmChain();
    break;

  case ModBodyPart::LeftThigh:
  case ModBodyPart::RightThigh:
    chain_.BuildLegChain();
    break;

  case ModBodyPart::Neck:
    chain_.BuildNeckChain();
    break;

  default:
    break;
  }

  UpdateControlPointHierarchy();
}

int ModBody::FindControlPointIndex(ModControlPointRole role) const {
  for (size_t i = 0; i < controlPoints_.size(); ++i) {
    if (controlPoints_[i].role == role) {
      return static_cast<int>(i);
    }
  }

  return -1;
}

bool ModBody::MoveControlPoint(size_t index, const Vector3 &newLocalPosition) {
  // owner 部位はまず chain を動かし、その結果を controlPoints_ へ同期する
  if (HasOwnControlPoints() && !chain_.GetNodes().empty()) {
    if (index >= controlPoints_.size()) {
      return false;
    }

    const ModControlPointRole role = controlPoints_[index].role;

    int chainIndex = chain_.FindIndex(role);
    if (chainIndex < 0) {
      // 念のための保険。基本は role で見つかる想定
      chainIndex = static_cast<int>(index);
    }

    if (chain_.MovePoint(static_cast<size_t>(chainIndex), newLocalPosition)) {
      SyncControlPointsFromChain(&controlPoints_, chain_);
      UpdateControlPointHierarchy();
      return true;
    }
  }

  // ここから下は旧ロジックのフォールバック（既存のまま）
  if (index >= controlPoints_.size()) {
    return false;
  }

  if (!controlPoints_[index].movable) {
    return false;
  }

  const ModControlPointRole role = controlPoints_[index].role;
  const Vector3 oldPosition = controlPoints_[index].localPosition;
  const Vector3 delta = Subtract(newLocalPosition, oldPosition);

  if (role == ModControlPointRole::Bend) {
    const int rootIndex = FindControlPointIndex(ModControlPointRole::Root);
    const int endIndex = FindControlPointIndex(ModControlPointRole::End);

    if (rootIndex >= 0 && endIndex >= 0) {
      const Vector3 rootPos =
          controlPoints_[static_cast<size_t>(rootIndex)].localPosition;
      const Vector3 oldBendPos = controlPoints_[index].localPosition;
      const Vector3 oldEndPos =
          controlPoints_[static_cast<size_t>(endIndex)].localPosition;

      const Vector3 bendToEndOffset = Subtract(oldEndPos, oldBendPos);

      float defaultRadius = 0.9f;
      if (IsLegControlPart(part_)) {
        defaultRadius = 1.1f;
      }

      const float minRadius = defaultRadius * 0.35f;
      const float maxRadius = defaultRadius * 1.75f;

      Vector3 rootToCandidate = Subtract(newLocalPosition, rootPos);
      Vector3 direction = NormalizeSafe(rootToCandidate, {0.0f, -1.0f, 0.0f});
      float radius = Length(rootToCandidate);
      radius = ClampFloat(radius, minRadius, maxRadius);

      const Vector3 clampedBend = Add(rootPos, Multiply(radius, direction));

      controlPoints_[index].localPosition = clampedBend;
      controlPoints_[static_cast<size_t>(endIndex)].localPosition =
          Add(clampedBend, bendToEndOffset);

      UpdateControlPointHierarchy();
      return true;
    }
  }

  controlPoints_[index].localPosition = newLocalPosition;

  switch (role) {
  case ModControlPointRole::Chest: {
    const int bellyIndex = FindControlPointIndex(ModControlPointRole::Belly);
    const int waistIndex = FindControlPointIndex(ModControlPointRole::Waist);

    if (bellyIndex >= 0) {
      controlPoints_[static_cast<size_t>(bellyIndex)].localPosition = Add(
          controlPoints_[static_cast<size_t>(bellyIndex)].localPosition, delta);
    }

    if (waistIndex >= 0) {
      controlPoints_[static_cast<size_t>(waistIndex)].localPosition = Add(
          controlPoints_[static_cast<size_t>(waistIndex)].localPosition, delta);
    }
    break;
  }

  case ModControlPointRole::Belly: {
    const int waistIndex = FindControlPointIndex(ModControlPointRole::Waist);
    if (waistIndex >= 0) {
      controlPoints_[static_cast<size_t>(waistIndex)].localPosition = Add(
          controlPoints_[static_cast<size_t>(waistIndex)].localPosition, delta);
    }
    break;
  }

  default:
    break;
  }

  UpdateControlPointHierarchy();
  return true;
}

bool ModBody::ScaleControlPoint(size_t index, float scaleFactor) {
  if (index >= controlPoints_.size()) {
    return false;
  }

  if (scaleFactor <= 0.0f) {
    return false;
  }

  ModControlPoint &point = controlPoints_[index];
  const ModControlPointRole role = point.role;

  const float defaultRadius = GetDefaultPointRadius(part_, role);
  const float minRadius = defaultRadius * 0.60f;
  const float maxRadius = defaultRadius * 2.50f;

  const float oldRadius = point.radius;

  float newRadius = point.radius * scaleFactor;
  newRadius = ClampFloat(newRadius, minRadius, maxRadius);

  const float radiusDelta = newRadius - oldRadius;
  if (fabsf(radiusDelta) < 0.0001f) {
    return true;
  }

  point.radius = newRadius;

  Logger::Log("========== ScaleControlPoint START ==========");
  Logger::Log("part=%d role=%d index=%d", static_cast<int>(part_),
              static_cast<int>(role), static_cast<int>(index));
  Logger::Log("oldRadius=%.3f newRadius=%.3f radiusDelta=%.3f scaleFactor=%.3f",
              oldRadius, newRadius, radiusDelta, scaleFactor);
  Logger::Log("param.scale=(%.3f, %.3f, %.3f) length=%.3f", param_.scale.x,
              param_.scale.y, param_.scale.z, param_.length);
  DebugLogVector3("point.localPosition", point.localPosition);

  if (HasOwnControlPoints() && !chain_.GetNodes().empty()) {
    const int chainIndex = chain_.FindIndex(role);
    if (chainIndex >= 0) {
      std::vector<ControlPointNode> &nodes = chain_.GetNodes();
      nodes[static_cast<size_t>(chainIndex)].radius = newRadius;
    }
  }

  // ------------------------------------------------------------
  // 半径変化に応じて、下流側の操作点を明示的に押し出す
  // EnforceAdjacentPointSpacing() だけだと最小距離ぶんしか動かず、
  // 見た目変化が弱いのでここで先に押し出す
  // ------------------------------------------------------------
  if (role == ModControlPointRole::Root) {
    const int bendIndex = FindControlPointIndex(ModControlPointRole::Bend);
    const int endIndex = FindControlPointIndex(ModControlPointRole::End);

    if (bendIndex >= 0) {
      const Vector3 bendPos =
          controlPoints_[static_cast<size_t>(bendIndex)].localPosition;

      Vector3 segmentDir = Subtract(bendPos, point.localPosition);
      segmentDir = NormalizeSafe(segmentDir, {0.0f, -1.0f, 0.0f});

      const Vector3 lateralDir = GetLateralOutwardDirection(part_);

      // Y方向側の逃がし
      const float axialPush = radiusDelta * 1.10f;

      // X方向側の逃がし
      // 拡大時だけ強めに横へ逃がす。縮小時は戻し過ぎないよう弱めにする。
      float lateralPush = 0.0f;
      if (radiusDelta > 0.0f) {
        lateralPush = radiusDelta * 1.00f;
      } else {
        lateralPush = radiusDelta * 0.35f;
      }

      const Vector3 totalPush = Add(Multiply(axialPush, segmentDir),
                                    Multiply(lateralPush, lateralDir));

      Logger::Log("[RootScalePush] axialPush=%.3f lateralPush=%.3f", axialPush,
                  lateralPush);
      DebugLogVector3("segmentDir", segmentDir);
      DebugLogVector3("lateralDir", lateralDir);
      DebugLogVector3("totalPush", totalPush);

      DebugLogVector3("bend.before", bendPos);
      if (endIndex >= 0) {
        DebugLogVector3(
            "end.before",
            controlPoints_[static_cast<size_t>(endIndex)].localPosition);
      }

      controlPoints_[static_cast<size_t>(bendIndex)].localPosition =
          Add(controlPoints_[static_cast<size_t>(bendIndex)].localPosition,
              totalPush);

      if (endIndex >= 0) {
        controlPoints_[static_cast<size_t>(endIndex)].localPosition =
            Add(controlPoints_[static_cast<size_t>(endIndex)].localPosition,
                totalPush);
      }

      DebugLogVector3(
          "bend.after",
          controlPoints_[static_cast<size_t>(bendIndex)].localPosition);
      if (endIndex >= 0) {
        DebugLogVector3(
            "end.after",
            controlPoints_[static_cast<size_t>(endIndex)].localPosition);
      }
    }
  }

  if (role == ModControlPointRole::Bend) {
    const int endIndex = FindControlPointIndex(ModControlPointRole::End);

    if (endIndex >= 0) {
      const Vector3 endPos =
          controlPoints_[static_cast<size_t>(endIndex)].localPosition;

      Vector3 segmentDir = Subtract(endPos, point.localPosition);
      segmentDir = NormalizeSafe(segmentDir, {0.0f, -1.0f, 0.0f});

      const Vector3 lateralDir = GetLateralOutwardDirection(part_);

      // Bend は End だけを逃がす。Root 側は動かさない。
      const float axialPush = radiusDelta * 1.00f;

      float lateralPush = 0.0f;
      if (radiusDelta > 0.0f) {
        lateralPush = radiusDelta * 0.90f;
      } else {
        lateralPush = radiusDelta * 0.30f;
      }

      const Vector3 totalPush = Add(Multiply(axialPush, segmentDir),
                                    Multiply(lateralPush, lateralDir));

      Logger::Log("[BendScalePush] axialPush=%.3f lateralPush=%.3f", axialPush,
                  lateralPush);
      DebugLogVector3("segmentDir", segmentDir);
      DebugLogVector3("lateralDir", lateralDir);
      DebugLogVector3("totalPush", totalPush);
      DebugLogVector3("end.before", endPos);

      controlPoints_[static_cast<size_t>(endIndex)].localPosition =
          Add(controlPoints_[static_cast<size_t>(endIndex)].localPosition,
              totalPush);

      DebugLogVector3(
          "end.after",
          controlPoints_[static_cast<size_t>(endIndex)].localPosition);
    }
  }

  UpdateControlPointHierarchy();

  // chain 側も最後に同期し直す
  if (HasOwnControlPoints() && !chain_.GetNodes().empty()) {
    SyncControlPointsFromChain(&controlPoints_, chain_);
  }

  for (size_t i = 0; i < controlPoints_.size(); ++i) {
    Logger::Log("point[%d] role=%d radius=%.3f pos=(%.3f, %.3f, %.3f)",
                static_cast<int>(i), static_cast<int>(controlPoints_[i].role),
                controlPoints_[i].radius, controlPoints_[i].localPosition.x,
                controlPoints_[i].localPosition.y,
                controlPoints_[i].localPosition.z);
  }
  Logger::Log("========== ScaleControlPoint END ==========");

  return true;
}

Vector3 ModBody::GetControlPointWorldPosition(const Object *target,
                                              size_t index) const {
  if (target == nullptr || index >= controlPoints_.size()) {
    return {0.0f, 0.0f, 0.0f};
  }

  return TransformControlPointLocalToWorld(target,
                                           controlPoints_[index].localPosition);
}

void ModBody::BuildDefaultControlPoints() {
  controlPoints_.clear();

  switch (part_) {
  case ModBodyPart::ChestBody: {
    controlPoints_.push_back(MakePoint(ModControlPointRole::Chest,
                                       {0.0f, 0.45f, 0.0f}, 0.12f, true, true,
                                       false, true));

    controlPoints_.push_back(MakePoint(ModControlPointRole::Belly,
                                       {0.0f, 0.0f, 0.0f}, 0.10f, true, false,
                                       false, false));

    controlPoints_.push_back(MakePoint(ModControlPointRole::Waist,
                                       {0.0f, -0.45f, 0.0f}, 0.12f, true, true,
                                       false, true));
    break;
  }

  case ModBodyPart::StomachBody: {
    break;
  }

  case ModBodyPart::Neck: {
    controlPoints_.push_back(MakePoint(ModControlPointRole::Root,
                                       {0.0f, 0.1f, 0.0f}, 0.09f, false, true,
                                       true, false));

    controlPoints_.push_back(MakePoint(ModControlPointRole::Bend,
                                       {0.0f, 0.38f, 0.0f}, 0.08f, true, true,
                                       false, true));

    controlPoints_.push_back(MakePoint(ModControlPointRole::End,
                                       {0.0f, 1.00f, 0.0f}, 0.11f, true, true,
                                       false, true));
    break;
  }

  case ModBodyPart::Head: {
    break;
  }

  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::RightUpperArm: {
    controlPoints_.push_back(MakePoint(ModControlPointRole::Root,
                                       {0.0f, 0.0f, 0.0f}, 0.09f, false, true,
                                       true, false));

    controlPoints_.push_back(MakePoint(ModControlPointRole::Bend,
                                       {0.0f, -0.55f, 0.0f}, 0.08f, true, false,
                                       false, false));

    controlPoints_.push_back(MakePoint(ModControlPointRole::End,
                                       {0.0f, -1.10f, 0.0f}, 0.08f, true, true,
                                       false, true));
    break;
  }

  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm: {
    break;
  }

  case ModBodyPart::LeftThigh:
  case ModBodyPart::RightThigh: {
    controlPoints_.push_back(MakePoint(ModControlPointRole::Root,
                                       {0.0f, 0.0f, 0.0f}, 0.10f, false, true,
                                       true, false));

    controlPoints_.push_back(MakePoint(ModControlPointRole::Bend,
                                       {0.0f, -0.70f, 0.0f}, 0.09f, true, false,
                                       false, false));

    controlPoints_.push_back(MakePoint(ModControlPointRole::End,
                                       {0.0f, -1.40f, 0.0f}, 0.09f, true, true,
                                       false, true));
    break;
  }

  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin: {
    break;
  }

  case ModBodyPart::Count:
  default:
    break;
  }
}

void ModBody::PushPointToMinimumDistance(int fixedIndex, int movableIndex,
                                         float extraMargin) {
  if (fixedIndex < 0 || movableIndex < 0) {
    return;
  }

  if (static_cast<size_t>(fixedIndex) >= controlPoints_.size() ||
      static_cast<size_t>(movableIndex) >= controlPoints_.size()) {
    return;
  }

  const Vector3 fixedPos =
      controlPoints_[static_cast<size_t>(fixedIndex)].localPosition;
  Vector3 movablePos =
      controlPoints_[static_cast<size_t>(movableIndex)].localPosition;

  const float fixedRadius =
      controlPoints_[static_cast<size_t>(fixedIndex)].radius;
  const float movableRadius =
      controlPoints_[static_cast<size_t>(movableIndex)].radius;

  const float minDistance = fixedRadius + movableRadius + extraMargin;

  Vector3 diff = Subtract(movablePos, fixedPos);
  float length = Length(diff);

  Vector3 dir = {0.0f, -1.0f, 0.0f};
  if (length > 0.0001f) {
    dir = NormalizeSafe(diff, dir);
  }

  if (length < minDistance) {
    movablePos = Add(fixedPos, Multiply(minDistance, dir));
    controlPoints_[static_cast<size_t>(movableIndex)].localPosition =
        movablePos;
  }
}

Vector3
ModBody::TransformControlPointLocalToWorld(const Object *target,
                                           const Vector3 &localPoint) const {
  if (target == nullptr) {
    return localPoint;
  }

  return ModObjectUtil::TransformLocalPointToWorld(target, localPoint);
}

Vector3 ModBody::ConvertExternalPointToThisObjectLocal(
    const Object *target, const Vector3 &externalOwnerLocalPoint) const {
  if (target == nullptr) {
    return externalOwnerLocalPoint;
  }

  // child の親空間で表現された点を、child 自身の local へ変換する
  const Vector3 childLocalOrigin = target->mainPosition.transform.translate;
  const Vector3 offsetInParent =
      Subtract(externalOwnerLocalPoint, childLocalOrigin);

  // ここが重要: ZだけでなくXYZを逆回転する
  return InverseRotateByEulerXYZLocal(offsetInParent,
                                      target->mainPosition.transform.rotate);
}

void ModBody::EnforceAdjacentPointSpacing() {
  const float extraMargin = 0.01f;

  {
    const int rootIndex = FindControlPointIndex(ModControlPointRole::Root);
    const int bendIndex = FindControlPointIndex(ModControlPointRole::Bend);
    const int endIndex = FindControlPointIndex(ModControlPointRole::End);

    if (rootIndex >= 0 && bendIndex >= 0) {
      PushPointToMinimumDistance(rootIndex, bendIndex, extraMargin);
    }

    if (bendIndex >= 0 && endIndex >= 0) {
      PushPointToMinimumDistance(bendIndex, endIndex, extraMargin);
    }
  }

  {
    const int chestIndex = FindControlPointIndex(ModControlPointRole::Chest);
    const int bellyIndex = FindControlPointIndex(ModControlPointRole::Belly);
    const int waistIndex = FindControlPointIndex(ModControlPointRole::Waist);

    if (chestIndex >= 0 && bellyIndex >= 0) {
      PushPointToMinimumDistance(chestIndex, bellyIndex, extraMargin);
    }

    if (bellyIndex >= 0 && waistIndex >= 0) {
      PushPointToMinimumDistance(bellyIndex, waistIndex, extraMargin);
    }
  }
}

void ModBody::UpdateControlPointHierarchy() {
  const int bendIndex = FindControlPointIndex(ModControlPointRole::Bend);
  const int endIndex = FindControlPointIndex(ModControlPointRole::End);

  if (bendIndex >= 0 && endIndex >= 0) {
    const Vector3 diff =
        Subtract(controlPoints_[static_cast<size_t>(endIndex)].localPosition,
                 controlPoints_[static_cast<size_t>(bendIndex)].localPosition);

    if (Length(diff) < 0.0001f) {
      const Vector3 fallback =
          (part_ == ModBodyPart::Neck || part_ == ModBodyPart::Head)
              ? Vector3{0.0f, 0.5f, 0.0f}
              : Vector3{0.0f, -0.5f, 0.0f};

      controlPoints_[static_cast<size_t>(endIndex)].localPosition =
          Add(controlPoints_[static_cast<size_t>(bendIndex)].localPosition,
              fallback);
    }
  }

  const int chestIndex = FindControlPointIndex(ModControlPointRole::Chest);
  const int bellyIndex = FindControlPointIndex(ModControlPointRole::Belly);
  const int waistIndex = FindControlPointIndex(ModControlPointRole::Waist);

  if (chestIndex >= 0 && bellyIndex >= 0) {
    const Vector3 diff =
        Subtract(controlPoints_[static_cast<size_t>(bellyIndex)].localPosition,
                 controlPoints_[static_cast<size_t>(chestIndex)].localPosition);

    if (Length(diff) < 0.0001f) {
      controlPoints_[static_cast<size_t>(bellyIndex)].localPosition =
          Add(controlPoints_[static_cast<size_t>(chestIndex)].localPosition,
              {0.0f, -0.5f, 0.0f});
    }
  }

  if (bellyIndex >= 0 && waistIndex >= 0) {
    const Vector3 diff =
        Subtract(controlPoints_[static_cast<size_t>(waistIndex)].localPosition,
                 controlPoints_[static_cast<size_t>(bellyIndex)].localPosition);

    if (Length(diff) < 0.0001f) {
      controlPoints_[static_cast<size_t>(waistIndex)].localPosition =
          Add(controlPoints_[static_cast<size_t>(bellyIndex)].localPosition,
              {0.0f, -0.5f, 0.0f});
    }
  }

  EnforceAdjacentPointSpacing();
}

void ModBody::ApplySegmentToObjectPart(Object *target, size_t partIndex,
                                       const Vector3 &startPos,
                                       const Vector3 &endPos, float startRadius,
                                       float endRadius) {
  if (target == nullptr) {
    return;
  }

  if (partIndex >= target->objectParts_.size()) {
    return;
  }

  if (partIndex >= basePartTransforms_.size()) {
    return;
  }

  Logger::Log("========== ApplySegmentToObjectPart START ==========");
  Logger::Log("part=%d partIndex=%d", static_cast<int>(part_),
              static_cast<int>(partIndex));
  DebugLogVector3("startPos", startPos);
  DebugLogVector3("endPos", endPos);
  Logger::Log("startRadius=%.3f endRadius=%.3f", startRadius, endRadius);
  Logger::Log("param.scale=(%.3f, %.3f, %.3f) length=%.3f", param_.scale.x,
              param_.scale.y, param_.scale.z, param_.length);

  Transform mesh = basePartTransforms_[partIndex];

  const Vector3 rawSegment = Subtract(endPos, startPos);
  const float rawLength = (std::max)(Length(rawSegment), 0.05f);
  const Vector3 segmentDir = NormalizeSafe(rawSegment, {0.0f, -1.0f, 0.0f});

  const float safeStartRadius = (std::max)(startRadius, 0.01f);
  const float safeEndRadius = (std::max)(endRadius, 0.01f);

  // 操作点の中心同士ではなく、
  // 両端の半径ぶんだけ外へ伸ばした区間でメッシュを作る
  // こうすると操作点球がメッシュ内に収まりやすい
  const Vector3 expandedStart =
      Subtract(startPos, Multiply(safeStartRadius, segmentDir));
  const Vector3 expandedEnd = Add(endPos, Multiply(safeEndRadius, segmentDir));

  const Vector3 visualSegment = Subtract(expandedEnd, expandedStart);
  const float visualLength = (std::max)(Length(visualSegment), rawLength);
  const Vector3 visualCenter = LerpV(expandedStart, expandedEnd, 0.5f);

  mesh.translate = visualCenter;

  mesh.rotate = basePartTransforms_[partIndex].rotate;
  mesh.rotate.z = AngleZFromMinusY(segmentDir);

  // セグメント太さは両端操作点半径の大きい方に合わせる
  const float segmentRadius = (std::max)(safeStartRadius, safeEndRadius);

  ModControlPointRole startRole = ModControlPointRole::None;
  ModControlPointRole endRole = ModControlPointRole::None;
  GetVisualSegmentRoles(part_, startRole, endRole);

  const float defaultSegmentRadius =
      GetDefaultSegmentRadius(part_, startRole, endRole);

  const float thicknessScale =
      segmentRadius / (std::max)(defaultSegmentRadius, 0.0001f);

  DebugLogVector3("expandedStart", expandedStart);
  DebugLogVector3("expandedEnd", expandedEnd);
  DebugLogVector3("visualCenter", visualCenter);
  Logger::Log("========== ApplySegmentToObjectPart END ==========");

  mesh.scale = basePartTransforms_[partIndex].scale;
  mesh.scale.x *= thicknessScale * param_.scale.x;
  mesh.scale.y = visualLength * param_.scale.y * param_.length;
  mesh.scale.z *= thicknessScale * param_.scale.z;

  target->objectParts_[partIndex].transform = mesh;
}

void ModBody::ApplySingleMeshFallback(Object *target,
                                      const Transform &baseMeshTransform,
                                      const std::vector<Transform> &baseParts,
                                      ModBodyPart part,
                                      const ModBodyPartParam &param) {
  if (target == nullptr || target->objectParts_.empty()) {
    return;
  }

  Vector3 newScale = MakePartScale(baseMeshTransform.scale, param);

  DebugLogVector3("newScale", newScale);

  Transform mesh = baseMeshTransform;
  mesh.scale = newScale;
  mesh.translate =
      Add(baseMeshTransform.translate, GetAnchorOffset(part, newScale));
  target->objectParts_[0].transform = mesh;

  HideUnusedMeshes(target, baseParts, 1);
}

void ModBody::HideUnusedMeshes(Object *target,
                               const std::vector<Transform> &baseParts,
                               size_t startIndex) {
  if (target == nullptr) {
    return;
  }

  for (size_t i = startIndex; i < target->objectParts_.size(); ++i) {
    if (i < baseParts.size()) {
      target->objectParts_[i].transform = baseParts[i];
    }
    target->objectParts_[i].transform.scale = ZeroV();
  }
}

void ModBody::SyncControlPointsFromChain(std::vector<ModControlPoint> *points,
                                         const ControlPointChain &chain) {
  if (points == nullptr) {
    return;
  }

  const std::vector<ControlPointNode> &nodes = chain.GetNodes();
  const size_t count = (std::min)(points->size(), nodes.size());

  for (size_t i = 0; i < count; ++i) {
    (*points)[i].localPosition = chain.GetWorldPosition(i);
  }
}
