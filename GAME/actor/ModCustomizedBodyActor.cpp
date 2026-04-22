#include "GAME/actor/ModCustomizedBodyActor.h"
#include "GAME/actor/ModBodyCustomizeDataUtil.h"

namespace {

Vector3 ZeroV() { return {0.0f, 0.0f, 0.0f}; }

Vector3 NormalizeSafeLocal(const Vector3 &v, const Vector3 &fallback) {
  const float len = Length(v);
  if (len < 0.0001f) {
    return fallback;
  }

  const float inv = 1.0f / len;
  return {v.x * inv, v.y * inv, v.z * inv};
}

PartSide InferSideFromPart(ModBodyPart part) {
  switch (part) {
  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::LeftForeArm:
  case ModBodyPart::LeftThigh:
  case ModBodyPart::LeftShin:
    return PartSide::Left;

  case ModBodyPart::RightUpperArm:
  case ModBodyPart::RightForeArm:
  case ModBodyPart::RightThigh:
  case ModBodyPart::RightShin:
    return PartSide::Right;

  default:
    return PartSide::Center;
  }
}

const char *PartNameLocal(ModBodyPart part) {
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

bool IsDebugTargetPart(ModBodyPart part) {
  return part == ModBodyPart::Head || part == ModBodyPart::LeftForeArm ||
         part == ModBodyPart::RightForeArm || part == ModBodyPart::LeftShin ||
         part == ModBodyPart::RightShin;
}

} // namespace

void ModCustomizedBodyActor::Initialize(kEngine *system) {
  system_ = system;
  actorTransform_ = CreateDefaultTransform();
}

void ModCustomizedBodyActor::BuildFromCustomizeData(
    const ModBodyCustomizeData &data) {
  customizeData_ = std::make_unique<ModBodyCustomizeData>(data);

  assembly_.InitializeDefaultHumanoid();

  if (!customizeData_->partInstances.empty()) {
    assembly_.Clear();

    for (size_t i = 0; i < customizeData_->partInstances.size(); ++i) {
      const ModPartInstanceData &instance = customizeData_->partInstances[i];

      PartNode node{};
      node.id = instance.partId;
      node.part = instance.partType;
      node.side = InferSideFromPart(instance.partType);
      node.parentId = instance.parentId;
      node.parentConnectorId = instance.parentConnectorId;
      node.selfConnectorId = instance.selfConnectorId;
      node.localTransform = instance.localTransform;

      assembly_.AddNode(node);
    }
  }

  SyncObjectsWithAssembly();
  RestorePartParamsFromCustomizeData();
  RestoreControlPointsFromCustomizeData();

  ApplyAssemblyToSceneHierarchy();
  ApplyModBodies();

  if (autoGroundEnabled_) {
    SnapToGround();
  }
}

void ModCustomizedBodyActor::UpdateAndDraw(Camera *camera) {
  if (camera == nullptr) {
    return;
  }

  ApplyAssemblyToSceneHierarchy();
  ApplyModBodies();

  if (autoGroundEnabled_) {
    SnapToGround();
    ApplyAssemblyToSceneHierarchy();
    ApplyModBodies();
  }

  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    if (modObjects_.count(id) == 0) {
      continue;
    }

    Object *object = modObjects_[id].get();
    if (object == nullptr) {
      continue;
    }

    object->Update(camera);
    object->Draw();
  }
}

bool ModCustomizedBodyActor::IsRootPartNode(const PartNode &node) const {
  return node.parentId < 0;
}

bool ModCustomizedBodyActor::TryGetFootEndWorldPosition(
    int partId, Vector3 &outWorld) const {
  std::unordered_map<int, ModBody>::const_iterator bodyIt =
      modBodies_.find(partId);
  std::unordered_map<int, std::unique_ptr<Object>>::const_iterator objIt =
      modObjects_.find(partId);

  if (bodyIt == modBodies_.end() || objIt == modObjects_.end()) {
    return false;
  }

  const ModBody &body = bodyIt->second;
  const Object *object = objIt->second.get();
  if (object == nullptr) {
    return false;
  }

  int endIndex = body.FindControlPointIndex(ModControlPointRole::End);
  if (endIndex >= 0) {
    outWorld = body.GetControlPointWorldPosition(object,
                                                 static_cast<size_t>(endIndex));
    return true;
  }

  int bendIndex = body.FindControlPointIndex(ModControlPointRole::Bend);
  if (bendIndex >= 0) {
    outWorld = body.GetControlPointWorldPosition(
        object, static_cast<size_t>(bendIndex));
    return true;
  }

  return false;
}

bool ModCustomizedBodyActor::TryGetFootEndContactWorldY(
    int partId, float &outContactY) const {
  std::unordered_map<int, ModBody>::const_iterator bodyIt =
      modBodies_.find(partId);
  std::unordered_map<int, std::unique_ptr<Object>>::const_iterator objIt =
      modObjects_.find(partId);

  if (bodyIt == modBodies_.end() || objIt == modObjects_.end()) {
    return false;
  }

  const ModBody &body = bodyIt->second;
  const Object *object = objIt->second.get();
  if (object == nullptr) {
    return false;
  }

  int sampleIndex = body.FindControlPointIndex(ModControlPointRole::End);
  if (sampleIndex < 0) {
    sampleIndex = body.FindControlPointIndex(ModControlPointRole::Bend);
    if (sampleIndex < 0) {
      return false;
    }
  }

  const std::vector<ModControlPoint> &points = body.GetControlPoints();
  if (static_cast<size_t>(sampleIndex) >= points.size()) {
    return false;
  }

  const Vector3 sampleWorld = body.GetControlPointWorldPosition(
      object, static_cast<size_t>(sampleIndex));

  float contactRadius = points[static_cast<size_t>(sampleIndex)].radius;

  // 見た目半径が取れる部位はそちらを優先
  const float visualRadius = body.GetVisualSegmentRadius(
      ModControlPointRole::Bend, ModControlPointRole::End);
  if (visualRadius > 0.0001f) {
    contactRadius = visualRadius;
  }

  outContactY = sampleWorld.y - contactRadius;
  return true;
}

float ModCustomizedBodyActor::ComputeLowestFootWorldY() const {
  bool found = false;
  float lowestY = 0.0f;

  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    const PartNode *node = assembly_.FindNode(id);
    if (node == nullptr) {
      continue;
    }

    float footContactY = 0.0f;
    bool ok = false;

    if (node->part == ModBodyPart::LeftShin ||
        node->part == ModBodyPart::RightShin) {
      ok = TryGetFootEndContactWorldY(id, footContactY);
    } else if (node->part == ModBodyPart::LeftThigh ||
               node->part == ModBodyPart::RightThigh) {
      // Shin が無い構成のフォールバック
      ok = TryGetFootEndContactWorldY(id, footContactY);
    }

    if (!ok) {
      continue;
    }

    if (!found) {
      lowestY = footContactY;
      found = true;
    } else if (footContactY < lowestY) {
      lowestY = footContactY;
    }
  }

  if (!found) {
    return actorTransform_.translate.y;
  }

  return lowestY;
}

void ModCustomizedBodyActor::ApplyGroundingToRootParts() {
  const float lowestFootY = ComputeLowestFootWorldY();
  const float deltaY = (groundY_ + groundOffsetY_) - lowestFootY;

  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    const PartNode *node = assembly_.FindNode(id);
    if (node == nullptr || !IsRootPartNode(*node)) {
      continue;
    }

    if (modObjects_.count(id) == 0) {
      continue;
    }

    Object *object = modObjects_[id].get();
    if (object == nullptr) {
      continue;
    }

    object->mainPosition.transform.translate.y += deltaY;
  }
}

void ModCustomizedBodyActor::SnapToGround() {
  ApplyAssemblyToSceneHierarchy();
  ApplyModBodies();
  ApplyGroundingToRootParts();
}

std::string ModCustomizedBodyActor::ModelPath(ModBodyPart part) {
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

int ModCustomizedBodyActor::ResolveControlOwnerPartId(
    const ModAssemblyGraph &assembly, int partId) {
  const PartNode *node = assembly.FindNode(partId);
  if (node == nullptr) {
    return -1;
  }

  ModBodyPart targetOwnerPart = ModBodyPart::Count;
  bool needsOwnerSearch = true;

  switch (node->part) {
  case ModBodyPart::Head:
    targetOwnerPart = ModBodyPart::Neck;
    break;
  case ModBodyPart::LeftForeArm:
    targetOwnerPart = ModBodyPart::LeftUpperArm;
    break;
  case ModBodyPart::RightForeArm:
    targetOwnerPart = ModBodyPart::RightUpperArm;
    break;
  case ModBodyPart::LeftShin:
    targetOwnerPart = ModBodyPart::LeftThigh;
    break;
  case ModBodyPart::RightShin:
    targetOwnerPart = ModBodyPart::RightThigh;
    break;
  default:
    needsOwnerSearch = false;
    break;
  }

  if (!needsOwnerSearch) {
    return partId;
  }

  const PartNode *current = node;
  while (current != nullptr && current->parentId >= 0) {
    const PartNode *parent = assembly.FindNode(current->parentId);
    if (parent == nullptr) {
      break;
    }

    if (parent->part == targetOwnerPart) {
      return parent->id;
    }

    current = parent;
  }

  // 見つからない場合は自分自身を返してフォールバック
  return partId;
}

void ModCustomizedBodyActor::ClearRuntimeObjects() {
  modObjects_.clear();
  modBodies_.clear();
  modModelHandles_.clear();
  orderedPartIds_.clear();
}

void ModCustomizedBodyActor::SyncObjectsWithAssembly() {
  orderedPartIds_ = assembly_.GetNodeIdsSorted();

  for (std::unordered_map<int, std::unique_ptr<Object>>::iterator it =
           modObjects_.begin();
       it != modObjects_.end();) {
    if (assembly_.FindNode(it->first) == nullptr) {
      modBodies_.erase(it->first);
      modModelHandles_.erase(it->first);
      it = modObjects_.erase(it);
    } else {
      ++it;
    }
  }

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
}

void ModCustomizedBodyActor::CreateObjectForNode(int partId,
                                                 const PartNode &node) {
  const std::string path = ModelPath(node.part);

  modModelHandles_[partId] = system_->SetModelObj(path);

  std::unique_ptr<Object> object = std::make_unique<Object>();
  object->IntObject(system_);
  object->CreateModelData(modModelHandles_[partId]);
  object->mainPosition.transform = CreateDefaultTransform();

  modObjects_[partId] = std::move(object);
  modBodies_[partId].Initialize(modObjects_[partId].get(), node.part);
}

void ModCustomizedBodyActor::RestorePartParamsFromCustomizeData() {
  if (customizeData_ == nullptr) {
    return;
  }

  for (size_t i = 0; i < customizeData_->partInstances.size(); ++i) {
    const ModPartInstanceData &instance = customizeData_->partInstances[i];
    if (modBodies_.count(instance.partId) == 0) {
      continue;
    }
    modBodies_[instance.partId].SetParam(instance.param);
  }
}

void ModCustomizedBodyActor::RestoreControlPointsFromCustomizeData() {
  if (customizeData_ == nullptr ||
      customizeData_->controlPointSnapshots.empty()) {
    return;
  }

  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int partId = orderedPartIds_[i];
    if (modBodies_.count(partId) == 0) {
      continue;
    }

    std::vector<const ModControlPointSnapshot *> snapshots =
        ModBodyCustomizeDataUtil::FindControlPointsByOwnerPartId(
            *customizeData_, partId);

    if (snapshots.empty()) {
      continue;
    }

    std::vector<ModControlPoint> points;
    points.reserve(snapshots.size());

    for (size_t si = 0; si < snapshots.size(); ++si) {
      const ModControlPointSnapshot *s = snapshots[si];
      if (s == nullptr) {
        continue;
      }

      ModControlPoint p{};
      p.role = s->role;
      p.localPosition = s->localPosition;
      p.radius = s->radius;
      p.movable = s->movable;
      p.isConnectionPoint = s->isConnectionPoint;
      p.acceptsParent = s->acceptsParent;
      p.acceptsChild = s->acceptsChild;
      points.push_back(p);
    }

    if (!points.empty()) {
      modBodies_[partId].SetControlPoints(points);
    }

    if (!points.empty()) {
      modBodies_[partId].SetControlPoints(points);
    }
  }
}

int ModCustomizedBodyActor::FindTorsoOwnerId() const {
  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    const PartNode *n = assembly_.FindNode(id);
    if (n != nullptr && n->part == ModBodyPart::ChestBody) {
      return id;
    }
  }
  return -1;
}

Vector3 ModCustomizedBodyActor::FindBodyPointLocal(
    int ownerPartId, ModControlPointRole role, const Vector3 &fallback) const {
  std::unordered_map<int, ModBody>::const_iterator it =
      modBodies_.find(ownerPartId);
  if (it == modBodies_.end()) {
    return fallback;
  }

  const std::vector<ModControlPoint> &points = it->second.GetControlPoints();
  for (size_t i = 0; i < points.size(); ++i) {
    if (points[i].role == role) {
      return points[i].localPosition;
    }
  }

  return fallback;
}

float ModCustomizedBodyActor::FindBodyPointRadius(int ownerPartId,
                                                  ModControlPointRole role,
                                                  float fallback) const {
  std::unordered_map<int, ModBody>::const_iterator it =
      modBodies_.find(ownerPartId);
  if (it == modBodies_.end()) {
    return fallback;
  }

  const std::vector<ModControlPoint> &points = it->second.GetControlPoints();
  for (size_t i = 0; i < points.size(); ++i) {
    if (points[i].role == role) {
      return points[i].radius;
    }
  }

  return fallback;
}

float ModCustomizedBodyActor::GetDefaultAttachRadius(
    ModBodyPart childPart) const {
  switch (childPart) {
  case ModBodyPart::Head:
    return 0.11f;

  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
    return 0.08f;

  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
    return 0.09f;

  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::RightUpperArm:
    return 0.09f;

  case ModBodyPart::LeftThigh:
  case ModBodyPart::RightThigh:
    return 0.10f;

  case ModBodyPart::Neck:
    return 0.09f;

  default:
    return 0.10f;
  }
}

float ModCustomizedBodyActor::GetCurrentAttachRadius(
    const PartNode &childNode) const {
  std::unordered_map<int, ModBody>::const_iterator it =
      modBodies_.find(childNode.id);
  if (it == modBodies_.end()) {
    return GetDefaultAttachRadius(childNode.part);
  }

  const ModBody &body = it->second;

  switch (childNode.part) {
  case ModBodyPart::Head:
    return FindBodyPointRadius(childNode.id, ModControlPointRole::End, 0.11f);

  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
    return FindBodyPointRadius(childNode.id, ModControlPointRole::Bend,
                               GetDefaultAttachRadius(childNode.part));

  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::RightUpperArm:
  case ModBodyPart::LeftThigh:
  case ModBodyPart::RightThigh:
  case ModBodyPart::Neck:
    return FindBodyPointRadius(childNode.id, ModControlPointRole::Root,
                               GetDefaultAttachRadius(childNode.part));

  default:
    break;
  }

  Vector3 ratio = body.GetVisualScaleRatio();
  return GetDefaultAttachRadius(childNode.part) * (std::max)(ratio.x, ratio.z);
}

float ModCustomizedBodyActor::GetDefaultAttachLength(
    ModBodyPart childPart) const {
  switch (childPart) {
  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::RightUpperArm:
    return 1.0814f;

  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
    return 3.2191f - 1.0814f;

  case ModBodyPart::LeftThigh:
  case ModBodyPart::RightThigh:
    return 1.5704f;

  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
    return 4.1885f - 1.5704f;

  case ModBodyPart::Neck:
    return 0.3462f;

  case ModBodyPart::Head:
    return 2.0252f - 0.3462f;

  case ModBodyPart::ChestBody:
    return 1.2796f;

  case ModBodyPart::StomachBody:
    return 1.6880f;

  default:
    return 1.0f;
  }
}

float ModCustomizedBodyActor::GetCurrentAttachLength(
    const PartNode &childNode) const {
  std::unordered_map<int, ModBody>::const_iterator it =
      modBodies_.find(childNode.id);
  if (it == modBodies_.end()) {
    return GetDefaultAttachLength(childNode.part);
  }

  const ModBody &body = it->second;
  const ModBodyPartParam &param = body.GetParam();
  return GetDefaultAttachLength(childNode.part) * param.length;
}

Vector3 ModCustomizedBodyActor::ComputeAttachPushOffset(
    const PartNode &parentNode, const PartNode &childNode) const {
  Vector3 outward = {0.0f, 1.0f, 0.0f};

  switch (childNode.part) {
  case ModBodyPart::LeftUpperArm:
    outward = {-1.0f, 0.0f, 0.0f};
    break;

  case ModBodyPart::RightUpperArm:
    outward = {1.0f, 0.0f, 0.0f};
    break;

  case ModBodyPart::LeftThigh:
    outward = {-1.0f, 0.0f, 0.0f};
    break;

  case ModBodyPart::RightThigh:
    outward = {1.0f, 0.0f, 0.0f};
    break;

  case ModBodyPart::Neck:
  case ModBodyPart::Head:
    outward = {0.0f, 1.0f, 0.0f};
    break;

  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin: {
    const int ownerId = ResolveControlOwnerPartId(assembly_, childNode.id);
    std::unordered_map<int, ModBody>::const_iterator it =
        modBodies_.find(ownerId);
    if (it != modBodies_.end()) {
      const std::vector<ModControlPoint> &points =
          it->second.GetControlPoints();

      int bendIndex = -1;
      int endIndex = -1;

      for (size_t i = 0; i < points.size(); ++i) {
        if (points[i].role == ModControlPointRole::Bend) {
          bendIndex = static_cast<int>(i);
        } else if (points[i].role == ModControlPointRole::End) {
          endIndex = static_cast<int>(i);
        }
      }

      if (bendIndex >= 0 && endIndex >= 0) {
        const Vector3 dir =
            Subtract(points[static_cast<size_t>(endIndex)].localPosition,
                     points[static_cast<size_t>(bendIndex)].localPosition);

        if (Length(dir) > 0.0001f) {
          outward = NormalizeSafeLocal(dir, outward);
        }
      }
    } else {
      outward = {0.0f, -1.0f, 0.0f};
    }
    break;
  }

  default:
    break;
  }

  const float defaultRadius = GetDefaultAttachRadius(childNode.part);
  const float currentRadius = GetCurrentAttachRadius(childNode);
  const float extraRadius = (std::max)(0.0f, currentRadius - defaultRadius);

  const float defaultLength = GetDefaultAttachLength(childNode.part);
  const float currentLength = GetCurrentAttachLength(childNode);
  const float extraLength = (std::max)(0.0f, currentLength - defaultLength);

  float pushDistance = extraRadius;
  pushDistance += extraLength * 0.35f;

  if (parentNode.part == ModBodyPart::ChestBody ||
      parentNode.part == ModBodyPart::StomachBody) {
    pushDistance += extraRadius;
  }

  return Multiply(pushDistance, outward);
}

Vector3 ModCustomizedBodyActor::ResolveDynamicAttachBase(
    const PartNode &parentNode, const PartNode &childNode,
    const Vector3 &defaultAttach) const {
  Vector3 base = defaultAttach;

  if (parentNode.part == ModBodyPart::ChestBody ||
      parentNode.part == ModBodyPart::StomachBody) {
    const int torsoOwnerId = FindTorsoOwnerId();
    if (torsoOwnerId >= 0) {
      switch (childNode.part) {
      case ModBodyPart::Neck:
      case ModBodyPart::Head:
        base = FindBodyPointLocal(torsoOwnerId, ModControlPointRole::NeckBase,
                                  defaultAttach);
        break;

      case ModBodyPart::LeftUpperArm:
        base = FindBodyPointLocal(
            torsoOwnerId, ModControlPointRole::LeftShoulder, defaultAttach);
        break;

      case ModBodyPart::RightUpperArm:
        base = FindBodyPointLocal(
            torsoOwnerId, ModControlPointRole::RightShoulder, defaultAttach);
        break;

      case ModBodyPart::LeftThigh:
        base = FindBodyPointLocal(torsoOwnerId, ModControlPointRole::LeftHip,
                                  defaultAttach);
        break;

      case ModBodyPart::RightThigh:
        base = FindBodyPointLocal(torsoOwnerId, ModControlPointRole::RightHip,
                                  defaultAttach);
        break;

      default:
        break;
      }
    }

    return Add(base, ComputeAttachPushOffset(parentNode, childNode));
  }

  if ((parentNode.part == ModBodyPart::LeftUpperArm ||
       parentNode.part == ModBodyPart::RightUpperArm) &&
      (childNode.part == ModBodyPart::LeftForeArm ||
       childNode.part == ModBodyPart::RightForeArm)) {
    std::unordered_map<int, ModBody>::const_iterator it =
        modBodies_.find(parentNode.id);
    if (it != modBodies_.end()) {
      const int bendIndex =
          it->second.FindControlPointIndex(ModControlPointRole::Bend);
      if (bendIndex >= 0) {
        const std::vector<ModControlPoint> &points =
            it->second.GetControlPoints();
        if (static_cast<size_t>(bendIndex) < points.size()) {
          base = points[static_cast<size_t>(bendIndex)].localPosition;
        }
      }
    }

    return Add(base, ComputeAttachPushOffset(parentNode, childNode));
  }

  if ((parentNode.part == ModBodyPart::LeftThigh ||
       parentNode.part == ModBodyPart::RightThigh) &&
      (childNode.part == ModBodyPart::LeftShin ||
       childNode.part == ModBodyPart::RightShin)) {
    std::unordered_map<int, ModBody>::const_iterator it =
        modBodies_.find(parentNode.id);
    if (it != modBodies_.end()) {
      const int bendIndex =
          it->second.FindControlPointIndex(ModControlPointRole::Bend);
      if (bendIndex >= 0) {
        const std::vector<ModControlPoint> &points =
            it->second.GetControlPoints();
        if (static_cast<size_t>(bendIndex) < points.size()) {
          base = points[static_cast<size_t>(bendIndex)].localPosition;
        }
      }
    }

    return Add(base, ComputeAttachPushOffset(parentNode, childNode));
  }

  if (parentNode.part == ModBodyPart::Neck &&
      childNode.part == ModBodyPart::Head) {
    std::unordered_map<int, ModBody>::const_iterator it =
        modBodies_.find(parentNode.id);
    if (it != modBodies_.end()) {
      const int bendIndex =
          it->second.FindControlPointIndex(ModControlPointRole::Bend);
      if (bendIndex >= 0) {
        const std::vector<ModControlPoint> &points =
            it->second.GetControlPoints();
        if (static_cast<size_t>(bendIndex) < points.size()) {
          base = points[static_cast<size_t>(bendIndex)].localPosition;
        }
      }
    }

    return Add(base, ComputeAttachPushOffset(parentNode, childNode));
  }

  return Add(base, ComputeAttachPushOffset(parentNode, childNode));
}

void ModCustomizedBodyActor::ApplyAssemblyToSceneHierarchy() {
  for (std::unordered_map<int, std::unique_ptr<Object>>::iterator it =
           modObjects_.begin();
       it != modObjects_.end(); ++it) {
    Object *object = it->second.get();
    if (object == nullptr) {
      continue;
    }

    object->followObject_ = nullptr;
    object->mainPosition.parentPart = nullptr;
    object->mainPosition.transform = CreateDefaultTransform();
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

    int renderParentId = node->parentId;

    if (node->part == ModBodyPart::Head ||
        node->part == ModBodyPart::LeftForeArm ||
        node->part == ModBodyPart::RightForeArm ||
        node->part == ModBodyPart::LeftShin ||
        node->part == ModBodyPart::RightShin) {
      const int ownerId = ResolveControlOwnerPartId(assembly_, id);
      if (ownerId >= 0 && ownerId != id && modObjects_.count(ownerId) > 0) {
        renderParentId = ownerId;
      }
    }

    if (renderParentId >= 0 && modObjects_.count(renderParentId) > 0) {
      Object *parentObject = modObjects_[renderParentId].get();
      if (parentObject != nullptr) {
        object->followObject_ = &parentObject->mainPosition;
        object->mainPosition.parentPart = &parentObject->mainPosition;
      }

      const PartNode *parentNode = assembly_.FindNode(renderParentId);
      if (parentNode != nullptr) {
        const Vector3 defaultAttach = assembly_.GetDefaultAttachLocal(
            parentNode->part, node->part, node->side);

        const Vector3 dynamicBase =
            ResolveDynamicAttachBase(*parentNode, *node, defaultAttach);

        const bool isExternallyDrivenPart =
            (node->part == ModBodyPart::Head ||
             node->part == ModBodyPart::LeftForeArm ||
             node->part == ModBodyPart::RightForeArm ||
             node->part == ModBodyPart::LeftShin ||
             node->part == ModBodyPart::RightShin);

        Vector3 childSelfOffset = {0.0f, 0.0f, 0.0f};

        // Head は自己オフセット禁止
        if (node->part != ModBodyPart::Head) {
          childSelfOffset = ResolveChildSelfAttachOffset(*node);
        }
        Vector3 offsetFromDefault = {0.0f, 0.0f, 0.0f};

        if (renderParentId != node->parentId || isExternallyDrivenPart) {
          localTranslate = Add(dynamicBase, childSelfOffset);
        } else {
          offsetFromDefault =
              Subtract(node->localTransform.translate, defaultAttach);

          localTranslate =
              Add(Add(dynamicBase, offsetFromDefault), childSelfOffset);
        }
      }
    }

    object->mainPosition.transform.translate = localTranslate;
    object->mainPosition.transform.rotate = localRotate;

    if (renderParentId < 0) {
      object->mainPosition.transform.translate = Add(
          object->mainPosition.transform.translate, actorTransform_.translate);
      object->mainPosition.transform.rotate =
          Add(object->mainPosition.transform.rotate, actorTransform_.rotate);
      object->mainPosition.transform.scale = actorTransform_.scale;
    } else {
      object->mainPosition.transform.scale = {1.0f, 1.0f, 1.0f};
    }
  }

  if (autoGroundEnabled_) {
    ApplyGroundingToRootParts();
  }
}

void ModCustomizedBodyActor::ApplyModBodies() {
  torsoSharedPoints_.clear();

  const int torsoOwnerId = FindTorsoOwnerId();
  if (torsoOwnerId >= 0 && modBodies_.count(torsoOwnerId) > 0) {
    const std::vector<ModControlPoint> &torsoPoints =
        modBodies_.at(torsoOwnerId).GetControlPoints();

    for (size_t i = 0; i < torsoPoints.size(); ++i) {
      if (torsoPoints[i].role == ModControlPointRole::Chest ||
          torsoPoints[i].role == ModControlPointRole::Belly ||
          torsoPoints[i].role == ModControlPointRole::Waist) {
        torsoSharedPoints_.push_back(torsoPoints[i]);
      }
    }
  }

  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    if (modBodies_.count(id) == 0) {
      continue;
    }

    const PartNode *node = assembly_.FindNode(id);
    if (node == nullptr) {
      continue;
    }

    ModBody &body = modBodies_[id];
    body.ClearExternalSegmentSource();

    switch (node->part) {
    case ModBodyPart::ChestBody:
      if (!torsoSharedPoints_.empty()) {
        body.SetExternalSegmentSource(&torsoSharedPoints_,
                                      ModControlPointRole::Chest,
                                      ModControlPointRole::Belly);
      }
      break;

    case ModBodyPart::StomachBody:
      if (!torsoSharedPoints_.empty()) {
        body.SetExternalSegmentSource(&torsoSharedPoints_,
                                      ModControlPointRole::Belly,
                                      ModControlPointRole::Waist);
      }
      break;

    case ModBodyPart::LeftForeArm:
    case ModBodyPart::RightForeArm:
    case ModBodyPart::LeftShin:
    case ModBodyPart::RightShin:
    case ModBodyPart::Head: {
      const int ownerId = ResolveControlOwnerPartId(assembly_, id);

      if (ownerId >= 0 && ownerId != id && modBodies_.count(ownerId) > 0) {
        const std::vector<ModControlPoint> &ownerPoints =
            modBodies_[ownerId].GetControlPoints();

        if (IsDebugTargetPart(node->part)) {
          int bendIndex = -1;
          int endIndex = -1;

          for (size_t pi = 0; pi < ownerPoints.size(); ++pi) {
            if (ownerPoints[pi].role == ModControlPointRole::Bend) {
              bendIndex = static_cast<int>(pi);
            } else if (ownerPoints[pi].role == ModControlPointRole::End) {
              endIndex = static_cast<int>(pi);
            }
          }
        }

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

    Object *object = modObjects_[id].get();
    if (object == nullptr) {
      continue;
    }

    modBodies_[id].Apply(object);
  }
}

Vector3 ModCustomizedBodyActor::ResolveChildSelfAttachOffset(
    const PartNode &childNode) const {
  Vector3 outward = {0.0f, 1.0f, 0.0f};

  // 自前でチェーンを持つ部位は、自分の Root -> Bend 方向を使う
  if (childNode.part == ModBodyPart::LeftUpperArm ||
      childNode.part == ModBodyPart::RightUpperArm ||
      childNode.part == ModBodyPart::LeftThigh ||
      childNode.part == ModBodyPart::RightThigh ||
      childNode.part == ModBodyPart::Neck) {
    std::unordered_map<int, ModBody>::const_iterator it =
        modBodies_.find(childNode.id);
    if (it != modBodies_.end()) {
      const ModBody &body = it->second;
      const int rootIndex =
          body.FindControlPointIndex(ModControlPointRole::Root);
      const int bendIndex =
          body.FindControlPointIndex(ModControlPointRole::Bend);

      if (rootIndex >= 0 && bendIndex >= 0) {
        const std::vector<ModControlPoint> &points = body.GetControlPoints();
        if (static_cast<size_t>(rootIndex) < points.size() &&
            static_cast<size_t>(bendIndex) < points.size()) {
          const Vector3 dir =
              Subtract(points[static_cast<size_t>(bendIndex)].localPosition,
                       points[static_cast<size_t>(rootIndex)].localPosition);

          if (Length(dir) > 0.0001f) {
            outward = NormalizeSafeLocal(dir, outward);
          }
        }
      }
    }
  }

  // 外部制御部位は owner 側の Bend -> End 方向を使う
  if (childNode.part == ModBodyPart::LeftForeArm ||
      childNode.part == ModBodyPart::RightForeArm ||
      childNode.part == ModBodyPart::LeftShin ||
      childNode.part == ModBodyPart::RightShin ||
      childNode.part == ModBodyPart::Head) {
    const int ownerId = ResolveControlOwnerPartId(assembly_, childNode.id);
    if (ownerId >= 0) {
      std::unordered_map<int, ModBody>::const_iterator ownerIt =
          modBodies_.find(ownerId);
      if (ownerIt != modBodies_.end()) {
        const ModBody &ownerBody = ownerIt->second;

        const int bendIndex =
            ownerBody.FindControlPointIndex(ModControlPointRole::Bend);
        const int endIndex =
            ownerBody.FindControlPointIndex(ModControlPointRole::End);

        if (bendIndex >= 0 && endIndex >= 0) {
          const std::vector<ModControlPoint> &points =
              ownerBody.GetControlPoints();

          if (static_cast<size_t>(bendIndex) < points.size() &&
              static_cast<size_t>(endIndex) < points.size()) {
            const Vector3 dir =
                Subtract(points[static_cast<size_t>(endIndex)].localPosition,
                         points[static_cast<size_t>(bendIndex)].localPosition);

            if (Length(dir) > 0.0001f) {
              outward = NormalizeSafeLocal(dir, outward);
            }
          }
        }
      }
    }
  }

  const float defaultRadius = GetDefaultAttachRadius(childNode.part);
  const float currentRadius = GetCurrentAttachRadius(childNode);
  const float extraRadius = (std::max)(0.0f, currentRadius - defaultRadius);

  const float defaultLength = GetDefaultAttachLength(childNode.part);
  const float currentLength = GetCurrentAttachLength(childNode);
  const float extraLength = (std::max)(0.0f, currentLength - defaultLength);

  return Multiply(extraRadius + extraLength, outward);
}