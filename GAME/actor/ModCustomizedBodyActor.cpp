#include "GAME/actor/ModCustomizedBodyActor.h"
#include "GAME/actor/ModBodyCustomizeDataUtil.h"
#include "GAME/actor/ModObjectUtil.h"

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
  
  SaveBasePose();
}

void ModCustomizedBodyActor::UpdateAndDraw(Camera *camera) {
  if (camera == nullptr) {
    return;
  }

  if (autoGroundEnabled_) {
    SnapToGround();
  } else {
    ApplyAssemblyToSceneHierarchy();
    ApplyModBodies();
  }

  // 自動アニメーションの更新
  if (isJoyAnimating_ || isFrustrationAnimating_) {
    animTimer_ += 1.0f / 60.0f; // 約60FPSを想定したタイマー加算
    
    if (isJoyAnimating_) {
      float jumpY = std::sin(animTimer_ * 6.0f) * 0.2f;
      if (jumpY < 0.0f) jumpY = 0.0f;
      float joyWeight = jumpY / 0.2f;
      ApplyJoyPose(joyWeight);
      if (autoGroundEnabled_) SetGroundOffsetY(jumpY);
    } else if (isFrustrationAnimating_) {
      float sway = (std::sin(animTimer_ * 3.0f) + 1.0f) * 0.5f; // 0.0 ~ 1.0
      ApplyFrustrationPose(sway);
    }
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

Vector3 ModCustomizedBodyActor::GetHeadWorldPosition() const {
  Vector3 world = actorTransform_.translate;
  world.y += 0.6f; // fallback
  TryGetPartWorldPosition(ModBodyPart::Head, world);
  return world;
}

bool ModCustomizedBodyActor::TryGetPartWorldPosition(ModBodyPart part, Vector3 &outWorld) const {
  for (int partId : orderedPartIds_) {
    auto bodyIt = modBodies_.find(partId);
    if (bodyIt != modBodies_.end() && bodyIt->second.GetPart() == part) {
      auto objIt = modObjects_.find(partId);
      if (objIt != modObjects_.end() && objIt->second) {
        outWorld = ModObjectUtil::TransformLocalPointToWorld(objIt->second.get(), {0.0f, 0.0f, 0.0f});
        return true;
      }
    }
  }
  return false;
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

  return TryGetFootEndContactWorldY(body, const_cast<Object*>(object), outContactY);
}

bool ModCustomizedBodyActor::TryGetFootEndContactWorldY(const ModBody& body,
                                                        Object* object,
                                                        float& outContactY) const {
  if (object == nullptr || object->objectParts_.empty()) {
    return false;
  }

  // 接地に関わるパーツ以外は無視
  const ModBodyPart part = body.GetPart();
  if (part != ModBodyPart::LeftShin && part != ModBodyPart::RightShin &&
      part != ModBodyPart::LeftThigh && part != ModBodyPart::RightThigh) {
    return false;
  }

  const std::vector<ModControlPoint>& points = body.GetControlPoints();
  int sampleIndex = body.FindControlPointIndex(ModControlPointRole::End);
  if (sampleIndex < 0) {
    sampleIndex = body.FindControlPointIndex(ModControlPointRole::Root);
  }
  if (sampleIndex < 0 || static_cast<size_t>(sampleIndex) >= points.size()) {
    return false;
  }

  // 1. デフォルトの操作点 (End) から計算した最下点
  Vector3 visualLocalPoint = points[static_cast<size_t>(sampleIndex)].localPosition;
  if (!body.HasOwnControlPoints()) {
    visualLocalPoint.x *= object->objectParts_[0].transform.scale.x;
    visualLocalPoint.y *= object->objectParts_[0].transform.scale.y;
    visualLocalPoint.z *= object->objectParts_[0].transform.scale.z;
  }
  const Vector3 sampleWorld = ModObjectUtil::TransformLocalPointToWorld(object, visualLocalPoint);

  float contactRadius = points[static_cast<size_t>(sampleIndex)].radius;
  const float visualRadius = body.GetVisualSegmentRadius(
      ModControlPointRole::Bend, ModControlPointRole::End);
  if (visualRadius > 0.0001f) {
    contactRadius = visualRadius;
  }
  contactRadius *= actorTransform_.scale.y;

  outContactY = sampleWorld.y - contactRadius;

  // 2. 実際のメッシュから自動計算されたカプセルの最下点（こちらの方が精度が高く、分厚い靴なども考慮される）
  if (body.GetAutoCalculatedCapsule().radius > 0.0001f) {
    const ModCapsule& cap = body.GetAutoCalculatedCapsule();
    
    // カプセルの両端をスケールしてからワールド空間へ変換
    Vector3 scaledStart = cap.start;
    Vector3 scaledEnd = cap.end;
    float scaledRadius = cap.radius;
    
    if (!body.HasOwnControlPoints()) {
      scaledStart.x *= object->objectParts_[0].transform.scale.x;
      scaledStart.y *= object->objectParts_[0].transform.scale.y;
      scaledStart.z *= object->objectParts_[0].transform.scale.z;
      
      scaledEnd.x *= object->objectParts_[0].transform.scale.x;
      scaledEnd.y *= object->objectParts_[0].transform.scale.y;
      scaledEnd.z *= object->objectParts_[0].transform.scale.z;
      
      scaledRadius *= (std::max)((std::max)(object->objectParts_[0].transform.scale.x, object->objectParts_[0].transform.scale.y), object->objectParts_[0].transform.scale.z);
    }
    
    Vector3 worldStart = ModObjectUtil::TransformLocalPointToWorld(object, scaledStart);
    Vector3 worldEnd = ModObjectUtil::TransformLocalPointToWorld(object, scaledEnd);
    
    // ワールド空間での半径を算出
    Vector3 radiusLocal = {0.0f, scaledRadius, 0.0f};
    Vector3 originWorld = ModObjectUtil::TransformLocalPointToWorld(object, {0.0f, 0.0f, 0.0f});
    Vector3 radiusWorld = ModObjectUtil::TransformLocalPointToWorld(object, radiusLocal);
    float worldRadius = Length(Subtract(radiusWorld, originWorld));

    float meshLowestWorldY = std::min(worldStart.y, worldEnd.y) - worldRadius;
    
    // メッシュの方が下に出っ張っている場合、そちらを採用
    if (meshLowestWorldY < outContactY) {
      outContactY = meshLowestWorldY;
    }
  }

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
    int partId = orderedPartIds_[i];
    if (assembly_.FindNode(partId)->parentId >= 0) {
      continue;
    }

    if (modObjects_.count(partId) == 0) {
      continue;
    }

    Object *object = modObjects_[partId].get();
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
  // 破棄時にダングリングポインタが発生しないように、事前に参照をクリアする
  for (auto &pair : modObjects_) {
    if (pair.second != nullptr) {
      pair.second->followObject_ = nullptr;
      pair.second->mainPosition.parentPart = nullptr;
    }
  }

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
        
        ModControlPoint pt = torsoPoints[i];
        pt.localPosition = Add(pt.localPosition, actorTransform_.translate);
        torsoSharedPoints_.push_back(pt);
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

void ModCustomizedBodyActor::SaveBasePose() {
  baseControlPointsCache_.clear();
  for (auto& pair : modBodies_) {
    baseControlPointsCache_[pair.first] = pair.second.GetControlPoints();
  }
}

void ModCustomizedBodyActor::RestoreBasePose() {
  for (auto& pair : modBodies_) {
    auto it = baseControlPointsCache_.find(pair.first);
    if (it != baseControlPointsCache_.end()) {
      pair.second.SetControlPoints(it->second);
    }
  }
}

void ModCustomizedBodyActor::ApplyJoyPose(float blendWeight) {
  RestoreBasePose();
  if (blendWeight <= 0.0f) return;
  
  for (auto& pair : modBodies_) {
    int partId = pair.first;
    ModBody& body = pair.second;
    ModBodyPart part = body.GetPart();
    
    // バンザイ（大の字の手）
    if (part == ModBodyPart::LeftUpperArm || part == ModBodyPart::LeftForeArm || 
        part == ModBodyPart::RightUpperArm || part == ModBodyPart::RightForeArm) {
      int endIdx = body.FindControlPointIndex(ModControlPointRole::End);
      int bendIdx = body.FindControlPointIndex(ModControlPointRole::Bend);
      
      Object* armObj = modObjects_.count(partId) ? modObjects_.at(partId).get() : nullptr;
      
      bool hasForeArm = false;
      for (const auto& childPair : modObjects_) {
        const PartNode* childNode = assembly_.FindNode(childPair.first);
        if (childNode && childNode->parentId == partId && 
            (childNode->part == ModBodyPart::LeftForeArm || childNode->part == ModBodyPart::RightForeArm)) {
          hasForeArm = true;
          break;
        }
      }

      Vector3 actorDir = (part == ModBodyPart::LeftUpperArm || part == ModBodyPart::LeftForeArm) 
                       ? Vector3{-1.5f, 1.5f, 0.0f} : Vector3{1.5f, 1.5f, 0.0f};
      Matrix4x4 actorRotMatrix = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, actorTransform_.rotate, {0.0f, 0.0f, 0.0f});
      Vector3 worldDir = ModObjectUtil::TransformVector(actorRotMatrix, actorDir);
      
      Vector3 localOffset = worldDir;
      if (armObj) {
          Vector3 localDir = ModObjectUtil::TransformWorldVectorToLocal(armObj, worldDir);
          float len = Length(localDir);
          if (len > 0.0001f) {
              localDir = {localDir.x / len, localDir.y / len, localDir.z / len};
              float originalLen = Length(worldDir);
              localOffset = {localDir.x * originalLen, localDir.y * originalLen, localDir.z * originalLen};
          }
      }

      if (endIdx >= 0) {
        Vector3 basePos = baseControlPointsCache_[partId][endIdx].localPosition;
        Vector3 newPos = {basePos.x + localOffset.x * blendWeight, basePos.y + localOffset.y * blendWeight, basePos.z + localOffset.z * blendWeight};
        body.MoveControlPoint(endIdx, newPos);
      }
      
      if (bendIdx >= 0) {
        Vector3 basePos = baseControlPointsCache_[partId][bendIdx].localPosition;
        if (!hasForeArm) {
            // 前腕がない場合は肘を末端として大きく動かす
            Vector3 newPos = {basePos.x + localOffset.x * blendWeight, basePos.y + localOffset.y * blendWeight, basePos.z + localOffset.z * blendWeight};
            body.MoveControlPoint(bendIdx, newPos);
        } else {
            // 通常の肘は半分だけ動かす
            Vector3 newPos = {basePos.x + localOffset.x * blendWeight * 0.5f, basePos.y + localOffset.y * blendWeight * 0.5f, basePos.z + localOffset.z * blendWeight * 0.5f};
            body.MoveControlPoint(bendIdx, newPos);
        }
      }
    }
    
    // 足を開く（大の字の足）
    if (part == ModBodyPart::LeftThigh || part == ModBodyPart::LeftShin || 
        part == ModBodyPart::RightThigh || part == ModBodyPart::RightShin) {
      int endIdx = body.FindControlPointIndex(ModControlPointRole::End);
      int bendIdx = body.FindControlPointIndex(ModControlPointRole::Bend);
      
      Object* legObj = modObjects_.count(partId) ? modObjects_.at(partId).get() : nullptr;
      
      bool hasShin = false;
      for (const auto& childPair : modObjects_) {
        const PartNode* childNode = assembly_.FindNode(childPair.first);
        if (childNode && childNode->parentId == partId && 
            (childNode->part == ModBodyPart::LeftShin || childNode->part == ModBodyPart::RightShin)) {
          hasShin = true;
          break;
        }
      }

      Vector3 actorDir = (part == ModBodyPart::LeftThigh || part == ModBodyPart::LeftShin) 
                       ? Vector3{-0.8f, 0.0f, 0.0f} : Vector3{0.8f, 0.0f, 0.0f};
      Matrix4x4 actorRotMatrix = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, actorTransform_.rotate, {0.0f, 0.0f, 0.0f});
      Vector3 worldDir = ModObjectUtil::TransformVector(actorRotMatrix, actorDir);
      
      Vector3 localOffset = worldDir;
      if (legObj) {
          Vector3 localDir = ModObjectUtil::TransformWorldVectorToLocal(legObj, worldDir);
          float len = Length(localDir);
          if (len > 0.0001f) {
              localDir = {localDir.x / len, localDir.y / len, localDir.z / len};
              float originalLen = Length(worldDir);
              localOffset = {localDir.x * originalLen, localDir.y * originalLen, localDir.z * originalLen};
          }
      }

      if (endIdx >= 0) {
        Vector3 basePos = baseControlPointsCache_[partId][endIdx].localPosition;
        Vector3 newPos = {basePos.x + localOffset.x * blendWeight, basePos.y + localOffset.y * blendWeight, basePos.z + localOffset.z * blendWeight};
        body.MoveControlPoint(endIdx, newPos);
      }
      
      if (bendIdx >= 0) {
        Vector3 basePos = baseControlPointsCache_[partId][bendIdx].localPosition;
        if (!hasShin) {
            Vector3 newPos = {basePos.x + localOffset.x * blendWeight, basePos.y + localOffset.y * blendWeight, basePos.z + localOffset.z * blendWeight};
            body.MoveControlPoint(bendIdx, newPos);
        } else {
            Vector3 newPos = {basePos.x + localOffset.x * blendWeight * 0.5f, basePos.y + localOffset.y * blendWeight * 0.5f, basePos.z + localOffset.z * blendWeight * 0.5f};
            body.MoveControlPoint(bendIdx, newPos);
        }
      }
    }
  }
}

void ModCustomizedBodyActor::ApplyFrustrationPose(float blendWeight) {
  RestoreBasePose();
  if (blendWeight <= 0.0f) return;
  
  Object* headObj = nullptr;
  for (auto& pair : modObjects_) {
    if (modBodies_.count(pair.first) && modBodies_[pair.first].GetPart() == ModBodyPart::Head) {
      headObj = pair.second.get();
      break;
    }
  }

  Vector3 leftHandTargetWorld = GetHeadWorldPosition();
  Vector3 rightHandTargetWorld = GetHeadWorldPosition();
  if (headObj) {
    // 物理的な頭のメッシュは、この後「首を前に倒す処理」によってローカル空間で {0.0, -0.5, 0.6} 移動する。
    Vector3 deformOffset = { 0.0f, -0.5f * blendWeight, 0.6f * blendWeight };

    // 手の基本位置に変形量を足す
    Vector3 leftTargetLocal = { 0.35f + deformOffset.x, 0.15f + deformOffset.y, 0.35f + deformOffset.z };
    Vector3 rightTargetLocal = { -0.35f + deformOffset.x, 0.15f + deformOffset.y, 0.35f + deformOffset.z };

    leftHandTargetWorld = ModObjectUtil::TransformLocalPointToWorld(headObj, leftTargetLocal);
    rightHandTargetWorld = ModObjectUtil::TransformLocalPointToWorld(headObj, rightTargetLocal);
  }

  // すべての頭を、自身の首の向き（Bend -> End）を軸にして振る
  float shakeAngle = std::sin(animTimer_ * 20.0f) * 0.5f * blendWeight;

  for (auto& pair : modObjects_) {
    int partId = pair.first;
    if (modBodies_.count(partId) && modBodies_[partId].GetPart() == ModBodyPart::Head) {
      Object* currentHeadObj = pair.second.get();
      if (currentHeadObj) {
        
        // 頭の回転軸となる「実際の首の向き」を計算
        Vector3 neckDir = {0.0f, 1.0f, 0.0f}; // デフォルトは上
        const int ownerId = ResolveControlOwnerPartId(assembly_, partId);
        if (ownerId >= 0 && modBodies_.count(ownerId)) {
          const ModBody& ownerBody = modBodies_[ownerId];
          int bendIdx = ownerBody.FindControlPointIndex(ModControlPointRole::Bend);
          int endIdx = ownerBody.FindControlPointIndex(ModControlPointRole::End);
          if (bendIdx >= 0 && endIdx >= 0) {
            Vector3 bendPos = ownerBody.GetControlPoints()[bendIdx].localPosition;
            Vector3 endPos = ownerBody.GetControlPoints()[endIdx].localPosition;
            Vector3 dir = Subtract(endPos, bendPos);
            if (Length(dir) > 0.0001f) {
              neckDir = NormalizeSafeLocal(dir, {0.0f, 1.0f, 0.0f});
            }
          }
        }
        
        // 実際の首の向きを軸とした回転行列を生成
        float c = std::cos(shakeAngle);
        float s = std::sin(shakeAngle);
        float t = 1.0f - c;
        Matrix4x4 shakeRot = Identity();
        shakeRot.m[0][0] = t * neckDir.x * neckDir.x + c;
        shakeRot.m[0][1] = t * neckDir.x * neckDir.y + s * neckDir.z;
        shakeRot.m[0][2] = t * neckDir.x * neckDir.z - s * neckDir.y;
        shakeRot.m[1][0] = t * neckDir.x * neckDir.y - s * neckDir.z;
        shakeRot.m[1][1] = t * neckDir.y * neckDir.y + c;
        shakeRot.m[1][2] = t * neckDir.y * neckDir.z + s * neckDir.x;
        shakeRot.m[2][0] = t * neckDir.x * neckDir.z + s * neckDir.y;
        shakeRot.m[2][1] = t * neckDir.y * neckDir.z - s * neckDir.x;
        shakeRot.m[2][2] = t * neckDir.z * neckDir.z + c;

        // 現在のローカル回転行列と合成し、新しいオイラー角を抽出する
        Matrix4x4 origRot = MakeRotateMatrix4x4(currentHeadObj->mainPosition.transform.rotate);
        Matrix4x4 finalRot = shakeRot * origRot;
        currentHeadObj->mainPosition.transform.rotate = ExtractRotate(finalRot);
      }
    }
  }
  
  for (auto& pair : modBodies_) {
    int partId = pair.first;
    ModBody& body = pair.second;
    ModBodyPart part = body.GetPart();
    
    // 頭を抱える（手は頭へ、肘は外側へ張り出す）
    if (part == ModBodyPart::LeftUpperArm || part == ModBodyPart::LeftForeArm || 
        part == ModBodyPart::RightUpperArm || part == ModBodyPart::RightForeArm) {
      int endIdx = body.FindControlPointIndex(ModControlPointRole::End);
      int bendIdx = body.FindControlPointIndex(ModControlPointRole::Bend);
      
      Object* armObj = modObjects_.count(partId) ? modObjects_.at(partId).get() : nullptr;
      
      bool hasForeArm = false;
      for (const auto& childPair : modObjects_) {
        const PartNode* childNode = assembly_.FindNode(childPair.first);
        if (childNode && childNode->parentId == partId && 
            (childNode->part == ModBodyPart::LeftForeArm || childNode->part == ModBodyPart::RightForeArm)) {
          hasForeArm = true;
          break;
        }
      }

      // ワールド空間での肘の張り出し方向（キャラクターの向きを考慮）
      Vector3 actorBendDir = (part == ModBodyPart::LeftUpperArm || part == ModBodyPart::LeftForeArm) 
                       ? Vector3{-0.8f, 0.7f, 0.4f} : Vector3{0.8f, 0.7f, 0.4f};
      Matrix4x4 actorRotMatrix = MakeAffineMatrix({1.0f, 1.0f, 1.0f}, actorTransform_.rotate, {0.0f, 0.0f, 0.0f});
      Vector3 worldBendDir = ModObjectUtil::TransformVector(actorRotMatrix, actorBendDir);
      
      Vector3 localBendOffset = worldBendDir;
      if (armObj) {
          Vector3 localDir = ModObjectUtil::TransformWorldVectorToLocal(armObj, worldBendDir);
          float len = Length(localDir);
          if (len > 0.0001f) {
              localDir = {localDir.x / len, localDir.y / len, localDir.z / len};
              float originalLen = Length(worldBendDir);
              localBendOffset = {localDir.x * originalLen, localDir.y * originalLen, localDir.z * originalLen};
          }
      }

      if (endIdx >= 0 && armObj && headObj) {
        Vector3 basePos = baseControlPointsCache_[partId][endIdx].localPosition;
        float armLength = Length(basePos);
        
        Vector3 targetWorld = (part == ModBodyPart::LeftUpperArm || part == ModBodyPart::LeftForeArm) ? leftHandTargetWorld : rightHandTargetWorld;
        Vector3 targetLocal = ModObjectUtil::TransformWorldPointToLocal(armObj, targetWorld);
        
        // Tレックス対応：腕が元の長さより伸びないように制限する
        float targetLength = Length(targetLocal);
        if (targetLength > armLength && targetLength > 0.0001f) {
            targetLocal.x = targetLocal.x * (armLength / targetLength);
            targetLocal.y = targetLocal.y * (armLength / targetLength);
            targetLocal.z = targetLocal.z * (armLength / targetLength);
        }
        
        Vector3 newPos = {
            basePos.x + (targetLocal.x - basePos.x) * blendWeight,
            basePos.y + (targetLocal.y - basePos.y) * blendWeight,
            basePos.z + (targetLocal.z - basePos.z) * blendWeight
        };
        body.MoveControlPoint(endIdx, newPos);
      }
      
      if (bendIdx >= 0 && armObj) {
        Vector3 basePos = baseControlPointsCache_[partId][bendIdx].localPosition;
        float bendLength = Length(basePos);
        
        if (!hasForeArm && headObj) {
            // 前腕がない場合、肘（Bend）が手の役割を果たすので、頭へ向かわせる
            Vector3 targetWorld = (part == ModBodyPart::LeftUpperArm || part == ModBodyPart::LeftForeArm) ? leftHandTargetWorld : rightHandTargetWorld;
            Vector3 targetLocal = ModObjectUtil::TransformWorldPointToLocal(armObj, targetWorld);
            float targetLength = Length(targetLocal);
            if (targetLength > bendLength && targetLength > 0.0001f) {
                targetLocal.x = targetLocal.x * (bendLength / targetLength);
                targetLocal.y = targetLocal.y * (bendLength / targetLength);
                targetLocal.z = targetLocal.z * (bendLength / targetLength);
            }
            Vector3 newPos = {
                basePos.x + (targetLocal.x - basePos.x) * blendWeight,
                basePos.y + (targetLocal.y - basePos.y) * blendWeight,
                basePos.z + (targetLocal.z - basePos.z) * blendWeight
            };
            body.MoveControlPoint(bendIdx, newPos);
        } else {
            // 通常の肘の曲げ
            float scale = bendLength / 1.2f; // 通常の腕の長さに合わせたスケール
            Vector3 newPos = {
                basePos.x + localBendOffset.x * scale * blendWeight,
                basePos.y + localBendOffset.y * scale * blendWeight,
                basePos.z + localBendOffset.z * scale * blendWeight
            };
            body.MoveControlPoint(bendIdx, newPos);
        }
      }
    }
    
    // 首を前に倒す（横移動は削除し、回転のみに任せる）
    if (part == ModBodyPart::Neck || part == ModBodyPart::Head) {
      int endIdx = body.FindControlPointIndex(ModControlPointRole::End);
      int bendIdx = body.FindControlPointIndex(ModControlPointRole::Bend);
      Vector3 offset = {0.0f, -0.5f, 0.6f};

      if (endIdx >= 0) {
        Vector3 basePos = baseControlPointsCache_[partId][endIdx].localPosition;
        Vector3 newPos = {basePos.x + offset.x * blendWeight, 
                          basePos.y + offset.y * blendWeight, 
                          basePos.z + offset.z * blendWeight};
        body.MoveControlPoint(endIdx, newPos);
      }
      if (bendIdx >= 0) {
        Vector3 basePos = baseControlPointsCache_[partId][bendIdx].localPosition;
        Vector3 newPos = {basePos.x + offset.x * blendWeight * 0.5f, 
                          basePos.y + offset.y * blendWeight * 0.5f, 
                          basePos.z + offset.z * blendWeight * 0.5f};
        body.MoveControlPoint(bendIdx, newPos);
      }
    }
  }
}