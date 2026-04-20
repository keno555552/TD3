#include "GAME/actor/ModBodyCustomizeDataUtil.h"
#include "GAME/actor/ModCustomizedBodyActor.h"

namespace {

Vector3 ZeroV() { return {0.0f, 0.0f, 0.0f}; }

} // namespace

void ModCustomizedBodyActor::Initialize(kEngine *system) { system_ = system; }

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
}

void ModCustomizedBodyActor::UpdateAndDraw(Camera *camera) {
  if (camera == nullptr) {
    return;
  }

  ApplyAssemblyToSceneHierarchy();
  ApplyModBodies();

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

Vector3 ModCustomizedBodyActor::FindSnapshotLocal(
    int ownerPartId, ModControlPointRole role, const Vector3 &fallback) const {
  if (customizeData_ == nullptr) {
    return fallback;
  }

  const ModControlPointSnapshot *s =
      ModBodyCustomizeDataUtil::FindControlPointByRole(*customizeData_,
                                                       ownerPartId, role);
  if (s == nullptr) {
    return fallback;
  }

  return s->localPosition;
}

Vector3 ModCustomizedBodyActor::ResolveDynamicAttachBase(
    const PartNode &parentNode, const PartNode &childNode,
    const Vector3 &defaultAttach) const {
  if (parentNode.part == ModBodyPart::ChestBody ||
      parentNode.part == ModBodyPart::StomachBody) {
    const int torsoOwnerId = FindTorsoOwnerId();
    if (torsoOwnerId >= 0) {
      switch (childNode.part) {
      case ModBodyPart::Neck:
      case ModBodyPart::Head:
        return FindSnapshotLocal(torsoOwnerId, ModControlPointRole::NeckBase,
                                 defaultAttach);
      case ModBodyPart::LeftUpperArm:
        return FindSnapshotLocal(
            torsoOwnerId, ModControlPointRole::LeftShoulder, defaultAttach);
      case ModBodyPart::RightUpperArm:
        return FindSnapshotLocal(
            torsoOwnerId, ModControlPointRole::RightShoulder, defaultAttach);
      case ModBodyPart::LeftThigh:
        return FindSnapshotLocal(torsoOwnerId, ModControlPointRole::LeftHip,
                                 defaultAttach);
      case ModBodyPart::RightThigh:
        return FindSnapshotLocal(torsoOwnerId, ModControlPointRole::RightHip,
                                 defaultAttach);
      default:
        break;
      }
    }
    return defaultAttach;
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
          return points[static_cast<size_t>(bendIndex)].localPosition;
        }
      }
    }
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
          return points[static_cast<size_t>(bendIndex)].localPosition;
        }
      }
    }
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
          return points[static_cast<size_t>(bendIndex)].localPosition;
        }
      }
    }
  }

  return defaultAttach;
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

    if (node->parentId >= 0 && modObjects_.count(node->parentId) > 0) {
      Object *parentObject = modObjects_[node->parentId].get();
      if (parentObject != nullptr) {
        object->followObject_ = &parentObject->mainPosition;
        object->mainPosition.parentPart = &parentObject->mainPosition;
      }

      const PartNode *parentNode = assembly_.FindNode(node->parentId);
      if (parentNode != nullptr) {
        const Vector3 defaultAttach = assembly_.GetDefaultAttachLocal(
            parentNode->part, node->part, node->side);

        const Vector3 dynamicBase =
            ResolveDynamicAttachBase(*parentNode, *node, defaultAttach);

        const Vector3 offsetFromDefault =
            Subtract(node->localTransform.translate, defaultAttach);

        localTranslate = Add(dynamicBase, offsetFromDefault);
      }
    }

    object->mainPosition.transform.translate = localTranslate;
    object->mainPosition.transform.scale = {1.0f, 1.0f, 1.0f};
  }
}

void ModCustomizedBodyActor::ApplyModBodies() {
  torsoSharedPoints_.clear();

  int torsoOwnerId = FindTorsoOwnerId();

  if (customizeData_ != nullptr && torsoOwnerId >= 0) {
    auto PushRole = [&](ModControlPointRole role) {
      const ModControlPointSnapshot *s =
          ModBodyCustomizeDataUtil::FindControlPointByRole(*customizeData_,
                                                           torsoOwnerId, role);
      if (s == nullptr) {
        return;
      }

      ModControlPoint p{};
      p.role = s->role;
      p.localPosition = s->localPosition;
      p.radius = s->radius;
      p.movable = s->movable;
      p.isConnectionPoint = s->isConnectionPoint;
      p.acceptsParent = s->acceptsParent;
      p.acceptsChild = s->acceptsChild;
      torsoSharedPoints_.push_back(p);
    };

    PushRole(ModControlPointRole::Chest);
    PushRole(ModControlPointRole::Belly);
    PushRole(ModControlPointRole::Waist);
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