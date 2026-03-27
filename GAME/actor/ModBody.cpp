#include "ModBody.h"
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
    startRole = ModControlPointRole::Root;
    endRole = ModControlPointRole::Bend;
    return true;

  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
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

  case ModBodyPart::Neck:
    startRole = ModControlPointRole::LowerNeck;
    endRole = ModControlPointRole::UpperNeck;
    return true;

  case ModBodyPart::Head:
    startRole = ModControlPointRole::UpperNeck;
    endRole = ModControlPointRole::HeadCenter;
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

ModBodyCustomizeData MakeDefaultCustomizeData() {
  ModBodyCustomizeData data{};

  for (auto &param : data.partParams) {
    param.scale = {1.0f, 1.0f, 1.0f};
    param.length = 1.0f;
    param.count = 1;
    param.enabled = true;
  }

  for (auto &offset : data.bodyJointOffsets) {
    offset = {0.0f, 0.0f, 0.0f};
  }

  data.bodyJointOffsets[ToIndex(ModBodyPart::Neck)] = {0.0f, 1.0f, 0.0f};
  data.bodyJointOffsets[ToIndex(ModBodyPart::LeftUpperArm)] = {-1.25f, 1.0f,
                                                               0.0f};
  data.bodyJointOffsets[ToIndex(ModBodyPart::RightUpperArm)] = {1.25f, 1.0f,
                                                                0.0f};
  data.bodyJointOffsets[ToIndex(ModBodyPart::LeftThigh)] = {-0.5f, -1.25f,
                                                            0.0f};
  data.bodyJointOffsets[ToIndex(ModBodyPart::RightThigh)] = {0.5f, -1.25f,
                                                             0.0f};

  return data;
}

std::unique_ptr<ModBodyCustomizeData> &SharedCustomizeDataStorage() {
  static std::unique_ptr<ModBodyCustomizeData> sharedData = nullptr;
  return sharedData;
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

void HideUnusedMeshes(Object *target, const std::vector<Transform> &baseParts,
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

void ApplySingleMeshFallback(Object *target, const Transform &baseMeshTransform,
                            const std::vector<Transform> &baseParts,
                            ModBodyPart part, const ModBodyPartParam &param) {
  if (target == nullptr || target->objectParts_.empty()) {
    return;
  }

  Vector3 newScale = MakePartScale(baseMeshTransform.scale, param);
  Transform mesh = baseMeshTransform;
  mesh.scale = newScale;
  mesh.translate = Add(baseMeshTransform.translate, GetAnchorOffset(part, newScale));
  target->objectParts_[0].transform = mesh;

  HideUnusedMeshes(target, baseParts, 1);
}

void SyncControlPointsFromChain(std::vector<ModControlPoint> *points,
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

std::unique_ptr<ModBodyCustomizeData> ModBody::CreateDefaultCustomizeData() {
  return std::make_unique<ModBodyCustomizeData>(MakeDefaultCustomizeData());
}

std::unique_ptr<ModBodyCustomizeData> ModBody::CopySharedCustomizeData() {
  const std::unique_ptr<ModBodyCustomizeData> &sharedData =
      SharedCustomizeDataStorage();

  if (sharedData == nullptr) {
    return nullptr;
  }

  return std::make_unique<ModBodyCustomizeData>(*sharedData);
}

void ModBody::SetSharedCustomizeData(const ModBodyCustomizeData &data) {
  SharedCustomizeDataStorage() = std::make_unique<ModBodyCustomizeData>(data);
}

const ModBodyCustomizeData *ModBody::GetSharedCustomizeData() {
  return SharedCustomizeDataStorage().get();
}

bool ModBody::HasOwnControlPoints() const {
  switch (part_) {
  case ModBodyPart::Head:
  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::RightUpperArm:
  case ModBodyPart::LeftThigh:
  case ModBodyPart::RightThigh:
    return true;

  case ModBodyPart::ChestBody:
  case ModBodyPart::StomachBody:
  case ModBodyPart::Neck:
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
      ApplySingleMeshFallback(target, baseMeshTransform_, basePartTransforms_, part_, param_);
      return;
    }
  }

  if (sourcePoints == nullptr || sourcePoints->empty()) {
    ApplySingleMeshFallback(target, baseMeshTransform_, basePartTransforms_, part_, param_);
    return;
  }

  const int startIndex = FindRoleIndexInPoints(*sourcePoints, startRole);
  const int endIndex = FindRoleIndexInPoints(*sourcePoints, endRole);

  if (startIndex < 0 || endIndex < 0) {
    ApplySingleMeshFallback(target, baseMeshTransform_, basePartTransforms_, part_, param_);
    return;
  }

  Vector3 startPos =
      (*sourcePoints)[static_cast<size_t>(startIndex)].localPosition;
  Vector3 endPos = (*sourcePoints)[static_cast<size_t>(endIndex)].localPosition;

  // 外部点列参照部位の描画:
  // - 前腕/脛/首などは「start基準（開始点を原点化）」でも良い
  // - 体（Chest/Stomach）は3点を同列に扱いたいので原点化しない
  if (useExternal) {
    const bool isTorso =
        (part_ == ModBodyPart::ChestBody || part_ == ModBodyPart::StomachBody);

    if (!isTorso) {
      endPos = Subtract(endPos, startPos);
      startPos = ZeroV();
    }
  }

  ApplySegmentToObjectPart(target, 0, startPos, endPos);

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

  case ModBodyPart::Head:
    chain_.BuildHeadChain();
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

  case ModControlPointRole::LowerNeck: {
    const int upperIndex =
        FindControlPointIndex(ModControlPointRole::UpperNeck);
    const int headIndex =
        FindControlPointIndex(ModControlPointRole::HeadCenter);

    if (upperIndex >= 0) {
      controlPoints_[static_cast<size_t>(upperIndex)].localPosition = Add(
          controlPoints_[static_cast<size_t>(upperIndex)].localPosition, delta);
    }

    if (headIndex >= 0) {
      controlPoints_[static_cast<size_t>(headIndex)].localPosition = Add(
          controlPoints_[static_cast<size_t>(headIndex)].localPosition, delta);
    }
    break;
  }

  case ModControlPointRole::UpperNeck: {
    const int headIndex =
        FindControlPointIndex(ModControlPointRole::HeadCenter);
    if (headIndex >= 0) {
      controlPoints_[static_cast<size_t>(headIndex)].localPosition = Add(
          controlPoints_[static_cast<size_t>(headIndex)].localPosition, delta);
    }
    break;
  }

  default:
    break;
  }

  UpdateControlPointHierarchy();
  return true;
}

Vector3 ModBody::GetControlPointWorldPosition(const Object *target,
                                              size_t index) const {
  if (target == nullptr || index >= controlPoints_.size()) {
    return {0.0f, 0.0f, 0.0f};
  }

  const Vector3 rootWorld = ComputeMainPositionWorldTranslate(target);
  return Add(rootWorld, controlPoints_[index].localPosition);
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
    break;
  }

  case ModBodyPart::Head: {
    controlPoints_.push_back(MakePoint(ModControlPointRole::LowerNeck,
                                       {0.0f, 0.0f, 0.0f}, 0.09f, false, true,
                                       true, false));

    controlPoints_.push_back(MakePoint(ModControlPointRole::UpperNeck,
                                       {0.0f, 0.28f, 0.0f}, 0.08f, true, true,
                                       false, true));

    controlPoints_.push_back(MakePoint(ModControlPointRole::HeadCenter,
                                       {0.0f, 0.80f, 0.0f}, 0.11f, true, true,
                                       false, true));
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

void ModBody::UpdateControlPointHierarchy() {
  const int bendIndex = FindControlPointIndex(ModControlPointRole::Bend);
  const int endIndex = FindControlPointIndex(ModControlPointRole::End);

  if (bendIndex >= 0 && endIndex >= 0) {
    const Vector3 diff =
        Subtract(controlPoints_[static_cast<size_t>(endIndex)].localPosition,
                 controlPoints_[static_cast<size_t>(bendIndex)].localPosition);

    if (Length(diff) < 0.0001f) {
      controlPoints_[static_cast<size_t>(endIndex)].localPosition =
          Add(controlPoints_[static_cast<size_t>(bendIndex)].localPosition,
              {0.0f, -0.5f, 0.0f});
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

  const int lowerIndex = FindControlPointIndex(ModControlPointRole::LowerNeck);
  const int upperIndex = FindControlPointIndex(ModControlPointRole::UpperNeck);
  const int headIndex = FindControlPointIndex(ModControlPointRole::HeadCenter);

  if (lowerIndex >= 0 && upperIndex >= 0) {
    const Vector3 diff =
        Subtract(controlPoints_[static_cast<size_t>(upperIndex)].localPosition,
                 controlPoints_[static_cast<size_t>(lowerIndex)].localPosition);

    if (Length(diff) < 0.0001f) {
      controlPoints_[static_cast<size_t>(upperIndex)].localPosition =
          Add(controlPoints_[static_cast<size_t>(lowerIndex)].localPosition,
              {0.0f, 0.35f, 0.0f});
    }
  }

  if (upperIndex >= 0 && headIndex >= 0) {
    const Vector3 diff =
        Subtract(controlPoints_[static_cast<size_t>(headIndex)].localPosition,
                 controlPoints_[static_cast<size_t>(upperIndex)].localPosition);

    if (Length(diff) < 0.0001f) {
      controlPoints_[static_cast<size_t>(headIndex)].localPosition =
          Add(controlPoints_[static_cast<size_t>(upperIndex)].localPosition,
              {0.0f, 0.5f, 0.0f});
    }
  }
}

void ModBody::ApplySegmentToObjectPart(Object *target, size_t partIndex,
                                       const Vector3 &startPos,
                                       const Vector3 &endPos) {
  if (target == nullptr) {
    return;
  }

  if (partIndex >= target->objectParts_.size()) {
    return;
  }

  if (partIndex >= basePartTransforms_.size()) {
    return;
  }

  Transform mesh = basePartTransforms_[partIndex];

  // 区間ベクトルと長さを求める
  const Vector3 segment = Subtract(endPos, startPos);
  const float segmentLength = (std::max)(Length(segment), 0.05f);
  const Vector3 segmentDir = NormalizeSafe(segment, {0.0f, -1.0f, 0.0f});

  // モデルは原点中心で作られているので、必ず点間の中点へ置く
  const Vector3 segmentCenter = LerpV(startPos, endPos, 0.5f);
  mesh.translate = segmentCenter;

  // -Y 向き基準の棒として回転を Z に反映する
  mesh.rotate = basePartTransforms_[partIndex].rotate;
  mesh.rotate.z = AngleZFromMinusY(segmentDir);

  // 太さは param.scale、長さは区間長に合わせる
  mesh.scale = basePartTransforms_[partIndex].scale;
  mesh.scale.x *= param_.scale.x;
  mesh.scale.y = segmentLength * param_.scale.y * param_.length;
  mesh.scale.z *= param_.scale.z;

  target->objectParts_[partIndex].transform = mesh;
}