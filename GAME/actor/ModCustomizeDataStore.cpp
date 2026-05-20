#include "GAME/actor/ModCustomizeDataStore.h"
#include "GAME/actor/ModAssemblyGraph.h"

namespace {

/// <summary>
/// デフォルトの改造データを生成する（内部用）
///
/// 【内容】
/// - 全部位のスケール・長さ・有効状態を初期化
/// - 旧方式の bodyJointOffsets にデフォルト接続位置を設定
/// - 制限時間などの進行データを初期化
///
/// 【注意】
/// bodyJointOffsets は「固定オフセット」なので、
/// 実際の操作点（controlPointSnapshots）とは一致しない可能性がある
/// → 新方式ではあくまで互換用
/// </summary>
ModBodyCustomizeData MakeDefaultCustomizeDataLocal() {
  ModBodyCustomizeData data{};
  data.dataVersion = 2;

  // 各部位パラメータの初期化
  for (auto &param : data.partParams) {
    param.scale = {1.0f, 1.0f, 1.0f};
    param.length = 1.0f;
    param.count = 1;
    param.enabled = true;
  }

  // 旧方式の接続オフセット初期化
  for (auto &offset : data.bodyJointOffsets) {
    offset = {0.0f, 0.0f, 0.0f};
  }

  // デフォルトの接続位置（旧方式）
  data.bodyJointOffsets[static_cast<size_t>(ModBodyPart::Neck)] = {0.0f, 1.0f,
                                                                   0.0f};
  data.bodyJointOffsets[static_cast<size_t>(ModBodyPart::LeftUpperArm)] = {
      -1.25f, 1.0f, 0.0f};
  data.bodyJointOffsets[static_cast<size_t>(ModBodyPart::RightUpperArm)] = {
      1.25f, 1.0f, 0.0f};
  data.bodyJointOffsets[static_cast<size_t>(ModBodyPart::LeftThigh)] = {
      -0.5f, -1.25f, 0.0f};
  data.bodyJointOffsets[static_cast<size_t>(ModBodyPart::RightThigh)] = {
      0.5f, -1.25f, 0.0f};

  // 新方式の操作点は空からスタート
  data.controlPointSnapshots.clear();

  // 新方式のデフォルト部位・操作点構成を生成する
  ModAssemblyGraph graph;
  graph.InitializeDefaultHumanoid();

  std::vector<int> ids = graph.GetNodeIdsSorted();
  for (size_t i = 0; i < ids.size(); ++i) {
    const int id = ids[i];
    const PartNode* node = graph.FindNode(id);
    if (node == nullptr) {
      continue;
    }

    ModPartInstanceData instance;
    instance.partId = node->id;
    instance.partType = node->part;
    instance.parentId = node->parentId;
    instance.parentConnectorId = node->parentConnectorId;
    instance.selfConnectorId = node->selfConnectorId;
    instance.localTransform = node->localTransform;
    instance.resolvedLocalTranslate = node->localTransform.translate;

    instance.param.scale = {1.0f, 1.0f, 1.0f};
    instance.param.length = 1.0f;
    instance.param.enabled = true;
    instance.param.count = 1;

    data.partInstances.push_back(instance);

    // デフォルトのコントロールポイントも生成する
    ModBody dummyBody;
    dummyBody.Initialize(nullptr, node->part);
    const std::vector<ModControlPoint>& cps = dummyBody.GetControlPoints();

    for (size_t cpIndex = 0; cpIndex < cps.size(); ++cpIndex) {
      const ModControlPoint& cp = cps[cpIndex];
      ModControlPointSnapshot snapshot;
      snapshot.ownerPartId = node->id;
      snapshot.ownerPartType = node->part;
      snapshot.role = cp.role;
      snapshot.localPosition = cp.localPosition;
      snapshot.radius = cp.radius;
      snapshot.movable = cp.movable;
      snapshot.isConnectionPoint = cp.isConnectionPoint;
      snapshot.acceptsParent = cp.acceptsParent;
      snapshot.acceptsChild = cp.acceptsChild;

      data.controlPointSnapshots.push_back(snapshot);
    }
  }

  // 初期生成後に新旧の整合性（旧配列へ反映）を取る
  ModCustomizeDataStore::NormalizeCustomizeData(data);

  // defaultControlPointSnapshots にも初期値をコピーしておく（ScoreCalculator用）
  data.defaultControlPointSnapshots = data.controlPointSnapshots;

  return data;
}

/// <summary>
/// 共有改造データの実体（シングルトン）
///
/// 【役割】
/// - シーン間で改造データを共有する
/// - ModScene → TravelScene などの受け渡しに使用
///
/// 【注意】
/// static unique_ptr を参照で返しているので、
/// 外部から直接差し替え可能になっている
/// </summary>
std::unique_ptr<ModBodyCustomizeData> &SharedCustomizeDataStorage() {
  static std::unique_ptr<ModBodyCustomizeData> sharedData = nullptr;
  return sharedData;
}

std::vector<std::unique_ptr<ModBodyCustomizeData>> &SharedNpcCustomizeDataStorage() {
  static std::vector<std::unique_ptr<ModBodyCustomizeData>> sharedNpcData;
  return sharedNpcData;
}

/// <summary>
/// 部位タイプが有効範囲かチェックする
/// </summary>
bool IsValidPartTypeLocal(ModBodyPart part) {
  const size_t index = static_cast<size_t>(part);
  return index < static_cast<size_t>(ModBodyPart::Count);
}

/// <summary>
/// 旧方式の partParams を初期化（全無効化）
///
/// 【目的】
/// 新方式（partInstances）から再構築する前にリセットする
/// </summary>
void ClearLegacyPartParamsLocal(ModBodyCustomizeData &data) {
  for (size_t i = 0; i < static_cast<size_t>(ModBodyPart::Count); ++i) {
    data.partParams[i].scale = {1.0f, 1.0f, 1.0f};
    data.partParams[i].length = 1.0f;
    data.partParams[i].count = 0;
    data.partParams[i].enabled = false;
  }
}

/// <summary>
/// 新方式の操作点スナップショットを旧方式へ反映する
///
/// 【目的】
/// - 旧コード（TravelSceneなど）が controlPoints を参照しても動くようにする
///
/// 【重要】
/// ここで「肩」「肘」「膝」などの位置が決まる
/// → 実質的に Root / Bend / End の変換テーブル
///
/// 【注意】
/// これは互換処理であり、
/// 本来は controlPointSnapshots を直接使うべき
/// </summary>
void ApplySnapshotToLegacyControlPointLocal(
    ModControlPointData &legacy, const ModControlPointSnapshot &snapshot) {

  switch (snapshot.role) {

  // 胴体
  case ModControlPointRole::Chest:
    legacy.chestPos = snapshot.localPosition;
    break;

  case ModControlPointRole::Belly:
    legacy.bellyPos = snapshot.localPosition;
    break;

  case ModControlPointRole::Waist:
    legacy.waistPos = snapshot.localPosition;
    break;

  // Root（根元）
  case ModControlPointRole::Root:
    switch (snapshot.ownerPartType) {
    case ModBodyPart::LeftUpperArm:
      legacy.leftShoulderPos = snapshot.localPosition;
      break;
    case ModBodyPart::RightUpperArm:
      legacy.rightShoulderPos = snapshot.localPosition;
      break;
    case ModBodyPart::LeftThigh:
      legacy.leftHipPos = snapshot.localPosition;
      break;
    case ModBodyPart::RightThigh:
      legacy.rightHipPos = snapshot.localPosition;
      break;
    case ModBodyPart::Neck:
      legacy.lowerNeckPos = snapshot.localPosition;
      break;
    default:
      break;
    }
    break;

  // Bend（関節）
  case ModControlPointRole::Bend:
    switch (snapshot.ownerPartType) {
    case ModBodyPart::LeftUpperArm:
      legacy.leftElbowPos = snapshot.localPosition;
      break;
    case ModBodyPart::RightUpperArm:
      legacy.rightElbowPos = snapshot.localPosition;
      break;
    case ModBodyPart::LeftThigh:
      legacy.leftKneePos = snapshot.localPosition;
      break;
    case ModBodyPart::RightThigh:
      legacy.rightKneePos = snapshot.localPosition;
      break;
    case ModBodyPart::Neck:
      legacy.upperNeckPos = snapshot.localPosition;
      break;
    default:
      break;
    }
    break;

  // End（末端）
  case ModControlPointRole::End:
    switch (snapshot.ownerPartType) {
    case ModBodyPart::LeftUpperArm:
      legacy.leftWristPos = snapshot.localPosition;
      break;
    case ModBodyPart::RightUpperArm:
      legacy.rightWristPos = snapshot.localPosition;
      break;
    case ModBodyPart::LeftThigh:
      legacy.leftAnklePos = snapshot.localPosition;
      break;
    case ModBodyPart::RightThigh:
      legacy.rightAnklePos = snapshot.localPosition;
      break;
    case ModBodyPart::Neck:
      legacy.headCenterPos = snapshot.localPosition;
      break;
    default:
      break;
    }
    break;

  default:
    break;
  }
}

} // namespace

std::unique_ptr<ModBodyCustomizeData>
ModCustomizeDataStore::CreateDefaultCustomizeData() {
  return std::make_unique<ModBodyCustomizeData>(MakeDefaultCustomizeDataLocal());
}

std::unique_ptr<ModBodyCustomizeData> ModCustomizeDataStore::CopySharedCustomizeData() {
  const std::unique_ptr<ModBodyCustomizeData> &sharedData =
      SharedCustomizeDataStorage();
  if (sharedData == nullptr) {
    return nullptr;
  }
  return std::make_unique<ModBodyCustomizeData>(*sharedData);
}

void ModCustomizeDataStore::SetSharedCustomizeData(const ModBodyCustomizeData &data) {
  ModBodyCustomizeData normalized = data;
  NormalizeCustomizeData(normalized);
  SharedCustomizeDataStorage() =
      std::make_unique<ModBodyCustomizeData>(normalized);
}

const ModBodyCustomizeData *ModCustomizeDataStore::GetSharedCustomizeData() {
  return SharedCustomizeDataStorage().get();
}

std::unique_ptr<ModBodyCustomizeData> ModCustomizeDataStore::CreateNpcPreset(NpcPresetType type, const ModBodyCustomizeData* baseData) {
  std::unique_ptr<ModBodyCustomizeData> data;
  if (baseData != nullptr) {
    data = std::make_unique<ModBodyCustomizeData>(*baseData);
  } else {
    data = ModBody::CreateDefaultCustomizeData();
  }

  if (data == nullptr) return nullptr;

  auto scalePart = [&](ModBodyPart targetType, float sx, float sy, float sz) {
    for (auto& inst : data->partInstances) {
      if (inst.partType == targetType) {
        inst.param.scale.x *= sx;
        inst.param.scale.y *= sy;
        inst.param.scale.z *= sz;
        
        for (auto& snap : data->controlPointSnapshots) {
          if (snap.ownerPartId == inst.partId) {
            snap.localPosition.x *= sx;
            snap.localPosition.y *= sy;
            snap.localPosition.z *= sz;
            snap.radius *= (sx + sz) * 0.5f;
          }
        }
      }
    }
  };

  switch (type) {
  case NpcPresetType::Default:
    break;

  case NpcPresetType::HeadBig:
    scalePart(ModBodyPart::Head, 2.0f, 2.0f, 2.0f);
    scalePart(ModBodyPart::Neck, 1.15f, 1.0f, 1.15f);
    break;

  case NpcPresetType::LongLeg:
  {
    for (auto& inst : data->partInstances) {
      if (inst.partType == ModBodyPart::LeftThigh || inst.partType == ModBodyPart::RightThigh ||
          inst.partType == ModBodyPart::LeftShin || inst.partType == ModBodyPart::RightShin) {
        inst.param.scale.x = 1.0f;
        inst.param.scale.y = 1.0f;
        inst.param.scale.z = 1.0f;
      }

      if (inst.partType == ModBodyPart::LeftShin) {
        inst.localTransform.translate.y = -3.14f;
        inst.resolvedLocalTranslate.y = -3.14f;
      } else if (inst.partType == ModBodyPart::RightShin) {
        inst.localTransform.translate.y = -3.14f;
        inst.resolvedLocalTranslate.y = -3.14f;
      }
    }

    for (auto& snap : data->controlPointSnapshots) {
      ModBodyPart partType = ModBodyPart::Count;
      for (const auto& inst : data->partInstances) {
        if (inst.partId == snap.ownerPartId) {
          partType = inst.partType;
          break;
        }
      }

      if (partType == ModBodyPart::LeftThigh || partType == ModBodyPart::RightThigh ||
          partType == ModBodyPart::LeftShin || partType == ModBodyPart::RightShin) {
        snap.localPosition.y *= 2.0f;
      }
    }
    break;
  }

  case NpcPresetType::BigTorso:
   // scalePart(ModBodyPart::ChestBody, 2.0f, 2.0f, 2.0f);
   // scalePart(ModBodyPart::StomachBody, 2.0f, 2.0f, 2.0f);
    //for (auto& inst : data->partInstances) {
    // if (inst.partType == ModBodyPart::ChestBody ||
    //     inst.partType == ModBodyPart::StomachBody) {
    //   inst.param.scale.x *= 2.0f;
    //   inst.param.scale.y *= 2.0f;
    //   inst.param.scale.z *= 2.0f;
    // }
    //  if (inst.partType == ModBodyPart::LeftUpperArm) {
    //    inst.localTransform.translate.x -= 0.8f;
    //    inst.resolvedLocalTranslate.x -= 0.8f;
    //  } else if (inst.partType == ModBodyPart::RightUpperArm) {
    //    inst.localTransform.translate.x += 0.8f;
    //    inst.resolvedLocalTranslate.x += 0.8f;
    //  }
    //}
    //for (auto& snap : data->controlPointSnapshots) {
    //  ModBodyPart partType = ModBodyPart::Count;
    //  for (const auto& inst : data->partInstances) {
    //    if (inst.partId == snap.ownerPartId) {
    //      partType = inst.partType;
    //      break;
    //    }
    //  }
    //  if (partType == ModBodyPart::LeftUpperArm) {
    //    snap.localPosition.x -= 0.8f;
    //  } else if (partType == ModBodyPart::RightUpperArm) {
    //    snap.localPosition.x += 0.8f;
    //  }
    //}
    //break;

    for (auto &inst : data->partInstances) {
      if (inst.partType == ModBodyPart::ChestBody ||
          inst.partType == ModBodyPart::StomachBody) {
        inst.param.scale.x = 1.0f;
        inst.param.scale.y = 1.0f;
        inst.param.scale.z = 1.0f;

        for (auto &snap : data->controlPointSnapshots) {
          if (snap.ownerPartId == inst.partId) {
            snap.radius *= 2.0f;
            snap.localPosition.x *= 2.0f;
            snap.localPosition.y *= 2.0f;
            snap.localPosition.z *= 2.0f;
          }
        }
      }

      // 腕の配置位置（元の最終座標 x = ±4.10, y = 2.30）
      if (inst.partType == ModBodyPart::LeftUpperArm) {
        inst.localTransform.translate.x = -4.10f;
        inst.localTransform.translate.y = 2.30f;
        inst.resolvedLocalTranslate.x = -4.10f;
        inst.resolvedLocalTranslate.y = 2.30f;
      } else if (inst.partType == ModBodyPart::RightUpperArm) {
        inst.localTransform.translate.x = 4.10f;
        inst.localTransform.translate.y = 2.30f;
        inst.resolvedLocalTranslate.x = 4.10f;
        inst.resolvedLocalTranslate.y = 2.30f;
      }
      // 首の配置位置（元の最終座標 y = 2.5592）
      else if (inst.partType == ModBodyPart::Neck) {
        inst.localTransform.translate.y = 2.5592f;
        inst.resolvedLocalTranslate.y = 2.5592f;
      }
      // 両脚の配置位置（元の最終座標 x = ±1.00, y = -3.36）
      else if (inst.partType == ModBodyPart::LeftThigh) {
        inst.localTransform.translate.x = -1.00f;
        inst.localTransform.translate.y = -3.36f;
        inst.resolvedLocalTranslate.x = -1.00f;
        inst.resolvedLocalTranslate.y = -3.36f;
      } else if (inst.partType == ModBodyPart::RightThigh) {
        inst.localTransform.translate.x = 1.00f;
        inst.localTransform.translate.y = -3.36f;
        inst.resolvedLocalTranslate.x = 1.00f;
        inst.resolvedLocalTranslate.y = -3.36f;
      }
    }

    // 腕の操作点は元のコード通り x 方向に ±0.8f だけずらす
    for (auto &snap : data->controlPointSnapshots) {
      ModBodyPart partType = ModBodyPart::Count;
      for (const auto &inst : data->partInstances) {
        if (inst.partId == snap.ownerPartId) {
          partType = inst.partType;
          break;
        }
      }
      if (partType == ModBodyPart::LeftUpperArm) {
        snap.localPosition.x -= 0.8f;
      } else if (partType == ModBodyPart::RightUpperArm) {
        snap.localPosition.x += 0.8f;
      }
    }
    break;

  case NpcPresetType::Gorilla:
  {
    auto getScale = [](ModBodyPart p) -> Vector3 {
      if (p == ModBodyPart::LeftUpperArm || p == ModBodyPart::RightUpperArm ||
          p == ModBodyPart::LeftForeArm || p == ModBodyPart::RightForeArm) {
        return {1.5f, 1.3f, 1.5f};
      }
      if (p == ModBodyPart::ChestBody) {
        return {1.6f, 1.0f, 1.4f};
      }
      if (p == ModBodyPart::LeftThigh || p == ModBodyPart::RightThigh ||
          p == ModBodyPart::LeftShin || p == ModBodyPart::RightShin) {
        return {1.0f, 0.6f, 1.0f};
      }
      return {1.0f, 1.0f, 1.0f};
    };

    for(auto& inst : data->partInstances) {
      inst.param.scale = {1.0f, 1.0f, 1.0f};
      ModBodyPart parentPart = ModBodyPart::Count;
      for (const auto& parentInst : data->partInstances) {
        if (parentInst.partId == inst.parentId) {
          parentPart = parentInst.partType;
          break;
        }
      }
      Vector3 pScale = getScale(parentPart);
      inst.localTransform.translate.x *= pScale.x;
      inst.localTransform.translate.y *= pScale.y;
      inst.localTransform.translate.z *= pScale.z;
      inst.resolvedLocalTranslate = inst.localTransform.translate;
    }

    for(auto& snap : data->controlPointSnapshots) {
      ModBodyPart partType = ModBodyPart::Count;
      for (const auto& inst : data->partInstances) {
        if (inst.partId == snap.ownerPartId) {
          partType = inst.partType;
          break;
        }
      }
      Vector3 mScale = getScale(partType);
      snap.localPosition.x *= mScale.x;
      snap.localPosition.y *= mScale.y;
      snap.localPosition.z *= mScale.z;
      snap.radius *= (mScale.x + mScale.z) * 0.5f;
    }
    break;
  }

  case NpcPresetType::Slender:
  {
    auto getScale = [](ModBodyPart p) -> Vector3 {
      Vector3 s = {0.6f, 1.0f, 0.6f};
      if (p == ModBodyPart::LeftThigh || p == ModBodyPart::RightThigh ||
          p == ModBodyPart::LeftShin || p == ModBodyPart::RightShin ||
          p == ModBodyPart::ChestBody || p == ModBodyPart::StomachBody) {
        s.y *= 1.3f;
      }
      return s;
    };

    for(auto& inst : data->partInstances) {
      ModBodyPart parentPart = ModBodyPart::Count;
      for (const auto& parentInst : data->partInstances) {
        if (parentInst.partId == inst.parentId) {
          parentPart = parentInst.partType;
          break;
        }
      }
      Vector3 pScale = getScale(parentPart);

      if (inst.partType == ModBodyPart::Head) {
        Vector3 hScale = getScale(inst.partType);
        inst.param.scale.x = hScale.x;
        inst.param.scale.y = hScale.y / pScale.y;
        inst.param.scale.z = hScale.z;
      } else {
        inst.param.scale = {1.0f, 1.0f, 1.0f};
      }

      inst.localTransform.translate.x *= pScale.x;
      inst.localTransform.translate.y *= pScale.y;
      inst.localTransform.translate.z *= pScale.z;
      inst.resolvedLocalTranslate = inst.localTransform.translate;
    }

    for(auto& snap : data->controlPointSnapshots) {
      ModBodyPart partType = ModBodyPart::Count;
      for (const auto& inst : data->partInstances) {
        if (inst.partId == snap.ownerPartId) {
          partType = inst.partType;
          break;
        }
      }
      Vector3 mScale = getScale(partType);
      snap.localPosition.x *= mScale.x;
      snap.localPosition.y *= mScale.y;
      snap.localPosition.z *= mScale.z;
      snap.radius *= (mScale.x + mScale.z) * 0.5f;
    }
    break;
  }

  case NpcPresetType::Chubby:
  {
    auto getScale = [](ModBodyPart p) -> Vector3 {
      Vector3 s = {1.8f, 1.0f, 1.8f};
      if (p == ModBodyPart::LeftThigh || p == ModBodyPart::RightThigh ||
          p == ModBodyPart::LeftShin || p == ModBodyPart::RightShin) {
        s.y *= 0.8f;
      } else if (p == ModBodyPart::StomachBody) {
        s.x *= 1.3f;
        s.z *= 1.5f;
      }
      return s;
    };

    for(auto& inst : data->partInstances) {
      ModBodyPart parentPart = ModBodyPart::Count;
      for (const auto& parentInst : data->partInstances) {
        if (parentInst.partId == inst.parentId) {
          parentPart = parentInst.partType;
          break;
        }
      }
      Vector3 pScale = getScale(parentPart);

      if (inst.partType == ModBodyPart::Head) {
        Vector3 hScale = getScale(inst.partType);
        inst.param.scale.x = hScale.x;
        inst.param.scale.y = hScale.y / pScale.y;
        inst.param.scale.z = hScale.z;
      } else {
        inst.param.scale = {1.0f, 1.0f, 1.0f};
      }

      inst.localTransform.translate.x *= pScale.x;
      inst.localTransform.translate.y *= pScale.y;
      inst.localTransform.translate.z *= pScale.z;
      inst.resolvedLocalTranslate = inst.localTransform.translate;
    }

    for(auto& snap : data->controlPointSnapshots) {
      ModBodyPart partType = ModBodyPart::Count;
      for (const auto& inst : data->partInstances) {
        if (inst.partId == snap.ownerPartId) {
          partType = inst.partType;
          break;
        }
      }
      Vector3 mScale = getScale(partType);
      snap.localPosition.x *= mScale.x;
      snap.localPosition.y *= mScale.y;
      snap.localPosition.z *= mScale.z;
      snap.radius *= (mScale.x + mScale.z) * 0.5f;
    }
    break;
  }

  case NpcPresetType::Giant:
  {
    auto getScale = [](ModBodyPart p) -> Vector3 {
      return {2.0f, 2.0f, 2.0f};
    };

    for(auto& inst : data->partInstances) {
      ModBodyPart parentPart = ModBodyPart::Count;
      for (const auto& parentInst : data->partInstances) {
        if (parentInst.partId == inst.parentId) {
          parentPart = parentInst.partType;
          break;
        }
      }
      Vector3 pScale = getScale(parentPart);

      if (inst.partType == ModBodyPart::Head) {
        Vector3 hScale = getScale(inst.partType);
        inst.param.scale.x = hScale.x;
        inst.param.scale.y = hScale.y / pScale.y;
        inst.param.scale.z = hScale.z;
      } else {
        inst.param.scale = {1.0f, 1.0f, 1.0f};
      }

      inst.localTransform.translate.x *= pScale.x;
      inst.localTransform.translate.y *= pScale.y;
      inst.localTransform.translate.z *= pScale.z;
      inst.resolvedLocalTranslate = inst.localTransform.translate;
    }

    for(auto& snap : data->controlPointSnapshots) {
      ModBodyPart partType = ModBodyPart::Count;
      for (const auto& inst : data->partInstances) {
        if (inst.partId == snap.ownerPartId) {
          partType = inst.partType;
          break;
        }
      }
      Vector3 mScale = getScale(partType);
      snap.localPosition.x *= mScale.x;
      snap.localPosition.y *= mScale.y;
      snap.localPosition.z *= mScale.z;
      snap.radius *= (mScale.x + mScale.z) * 0.5f;
    }
    break;
  }

  case NpcPresetType::Mini:
  {
    auto getScale = [](ModBodyPart p) -> Vector3 {
      return {0.5f, 0.5f, 0.5f};
    };

    for(auto& inst : data->partInstances) {
      ModBodyPart parentPart = ModBodyPart::Count;
      for (const auto& parentInst : data->partInstances) {
        if (parentInst.partId == inst.parentId) {
          parentPart = parentInst.partType;
          break;
        }
      }
      Vector3 pScale = getScale(parentPart);

      if (inst.partType == ModBodyPart::Head) {
        Vector3 hScale = getScale(inst.partType);
        inst.param.scale.x = hScale.x;
        inst.param.scale.y = hScale.y / pScale.y;
        inst.param.scale.z = hScale.z;
      } else {
        inst.param.scale = {1.0f, 1.0f, 1.0f};
      }

      inst.localTransform.translate.x *= pScale.x;
      inst.localTransform.translate.y *= pScale.y;
      inst.localTransform.translate.z *= pScale.z;
      inst.resolvedLocalTranslate = inst.localTransform.translate;
    }

    for(auto& snap : data->controlPointSnapshots) {
      ModBodyPart partType = ModBodyPart::Count;
      for (const auto& inst : data->partInstances) {
        if (inst.partId == snap.ownerPartId) {
          partType = inst.partType;
          break;
        }
      }
      Vector3 mScale = getScale(partType);
      snap.localPosition.x *= mScale.x;
      snap.localPosition.y *= mScale.y;
      snap.localPosition.z *= mScale.z;
      snap.radius *= (mScale.x + mScale.z) * 0.5f;
    }
    break;
  }

  case NpcPresetType::LongArm:
  {
    for (auto& inst : data->partInstances) {
      if (inst.partType == ModBodyPart::LeftUpperArm || inst.partType == ModBodyPart::RightUpperArm ||
          inst.partType == ModBodyPart::LeftForeArm || inst.partType == ModBodyPart::RightForeArm) {
        inst.param.scale.x = 1.0f;
        inst.param.scale.y = 1.0f;
        inst.param.scale.z = 1.0f;
      }

      if (inst.partType == ModBodyPart::LeftForeArm) {
        inst.localTransform.translate.y = -2.16f;
        inst.resolvedLocalTranslate.y = -2.16f;
      } else if (inst.partType == ModBodyPart::RightForeArm) {
        inst.localTransform.translate.y = -2.16f;
        inst.resolvedLocalTranslate.y = -2.16f;
      }
    }

    for (auto& snap : data->controlPointSnapshots) {
      ModBodyPart partType = ModBodyPart::Count;
      for (const auto& inst : data->partInstances) {
        if (inst.partId == snap.ownerPartId) {
          partType = inst.partType;
          break;
        }
      }

      if (partType == ModBodyPart::LeftUpperArm || partType == ModBodyPart::RightUpperArm ||
          partType == ModBodyPart::LeftForeArm || partType == ModBodyPart::RightForeArm) {
        snap.localPosition.y *= 2.0f;
      }
    }
    break;
  }

  case NpcPresetType::WideShoulder:
    // 肩幅をさらに広げ、腕の太さも追加して「肩幅おばけ」にする
    for (auto& inst : data->partInstances) {
      if (inst.partType == ModBodyPart::LeftUpperArm || inst.partType == ModBodyPart::RightUpperArm) {
        inst.param.scale.x *= 1.8f;
        inst.param.scale.z *= 1.8f;
      }
      if (inst.partType == ModBodyPart::LeftUpperArm) {
        inst.localTransform.translate.x -= 1.2f;
        inst.localTransform.translate.y += 0.1f;
        inst.resolvedLocalTranslate.x -= 1.2f;
        inst.resolvedLocalTranslate.y += 0.1f;
      } else if (inst.partType == ModBodyPart::RightUpperArm) {
        inst.localTransform.translate.x += 1.2f;
        inst.localTransform.translate.y += 0.1f;
        inst.resolvedLocalTranslate.x += 1.2f;
        inst.resolvedLocalTranslate.y += 0.1f;
      }
    }
    for (auto& snap : data->controlPointSnapshots) {
      ModBodyPart partType = ModBodyPart::Count;
      for (const auto& inst : data->partInstances) {
        if (inst.partId == snap.ownerPartId) {
          partType = inst.partType;
          break;
        }
      }
      if (partType == ModBodyPart::LeftUpperArm) {
        snap.localPosition.x -= 1.2f; // さらに外へ
        snap.localPosition.y += 0.1f; // 少し上へ
        snap.radius *= 1.8f;
      } else if (partType == ModBodyPart::RightUpperArm) {
        snap.localPosition.x += 1.2f;
        snap.localPosition.y += 0.1f;
        snap.radius *= 1.8f;
      } else if (partType == ModBodyPart::LeftForeArm || partType == ModBodyPart::RightForeArm) {
        // 前腕（ForeArm）のCPのradiusも1.8倍にスケーリングし、評価に腕全体の太さ変化を100%同期させる（描画の太さは現状維持）
        snap.radius *= 1.8f;
      }
    }
    break;

  case NpcPresetType::WideHip:
    // 腰をさらに広くし、太ももを太くして「下半身どっしり型」にする
    for (auto& inst : data->partInstances) {
      if (inst.partType == ModBodyPart::LeftThigh || inst.partType == ModBodyPart::RightThigh) {
        inst.param.scale.x *= 1.8f;
        inst.param.scale.z *= 1.8f;
      }
      if (inst.partType == ModBodyPart::LeftThigh) {
        inst.localTransform.translate.x -= 2.2f;
        inst.resolvedLocalTranslate.x -= 2.2f;
      } else if (inst.partType == ModBodyPart::RightThigh) {
        inst.localTransform.translate.x += 2.2f;
        inst.resolvedLocalTranslate.x += 2.2f;
      }
    }
    for (auto& snap : data->controlPointSnapshots) {
      ModBodyPart partType = ModBodyPart::Count;
      for (const auto& inst : data->partInstances) {
        if (inst.partId == snap.ownerPartId) {
          partType = inst.partType;
          break;
        }
      }
      if (partType == ModBodyPart::LeftThigh) {
        snap.localPosition.x -= 2.2f; // さらに外へ
        snap.radius *= 1.8f;
      } else if (partType == ModBodyPart::RightThigh) {
        snap.localPosition.x += 2.2f;
        snap.radius *= 1.8f;
      } else if (partType == ModBodyPart::LeftShin || partType == ModBodyPart::RightShin) {
        // 下腿（Shin）のCPのradiusも1.8倍にスケーリングし、評価に脚全体の太さ変化を100%同期させる（描画の太さは現状維持）
        snap.radius *= 1.8f;
      }
    }
    break;

  case NpcPresetType::MutantAsura:
  {
    // 胴体・腕の描画用スケール (inst.param.scale) は親子重複による異常変形や接続崩れを防ぐため 1.0f (変更なし) に維持し、
    // すべて CP (controlPointSnapshots) のスケーリングによってモデル描画と評価の両方を綺麗に変形・同期させます。
    for (auto& inst : data->partInstances) {
      // 胴体の scale は 1.0f に維持
      if (inst.partType == ModBodyPart::ChestBody || inst.partType == ModBodyPart::StomachBody) {
        inst.param.scale.x = 1.0f;
        inst.param.scale.y = 1.0f;
        inst.param.scale.z = 1.0f;
      }
      // 腕の scale も 1.0f に維持
      else if (inst.partType == ModBodyPart::LeftUpperArm || inst.partType == ModBodyPart::RightUpperArm ||
               inst.partType == ModBodyPart::LeftForeArm || inst.partType == ModBodyPart::RightForeArm) {
        inst.param.scale.x = 1.0f;
        inst.param.scale.y = 1.0f;
        inst.param.scale.z = 1.0f;
      }

      // --- 胴体巨大化 (1.5倍) に合わせた、各関節の接続位置 (translate) の手動オフセット調整 ---
      // 元の上腕 (UpperArm) の配置位置 (デフォルト x = ±2.05, y = 1.15 を 1.5倍にし、さらに 4本腕用に y += 0.5f)
      if (inst.partType == ModBodyPart::LeftUpperArm) {
        inst.localTransform.translate.x = -3.075f;
        inst.localTransform.translate.y = 2.225f;
        inst.resolvedLocalTranslate.x = -3.075f;
        inst.resolvedLocalTranslate.y = 2.225f;
      } else if (inst.partType == ModBodyPart::RightUpperArm) {
        inst.localTransform.translate.x = 3.075f;
        inst.localTransform.translate.y = 2.225f;
        inst.resolvedLocalTranslate.x = 3.075f;
        inst.resolvedLocalTranslate.y = 2.225f;
      }
      // 首の配置位置 (デフォルト y = 1.2796 を 1.5倍)
      else if (inst.partType == ModBodyPart::Neck) {
        inst.localTransform.translate.y = 1.9194f;
        inst.resolvedLocalTranslate.y = 1.9194f;
      }
      // 両脚の配置位置 (デフォルト x = ±0.50, y = -1.68 を 1.5倍)
      else if (inst.partType == ModBodyPart::LeftThigh) {
        inst.localTransform.translate.x = -0.75f;
        inst.localTransform.translate.y = -2.52f;
        inst.resolvedLocalTranslate.x = -0.75f;
        inst.resolvedLocalTranslate.y = -2.52f;
      } else if (inst.partType == ModBodyPart::RightThigh) {
        inst.localTransform.translate.x = 0.75f;
        inst.localTransform.translate.y = -2.52f;
        inst.resolvedLocalTranslate.x = 0.75f;
        inst.resolvedLocalTranslate.y = -2.52f;
      }
    }

    // 元の上腕のCPスナップショットの配置位置を調整 (y += 0.5f)
    for (auto& snap : data->controlPointSnapshots) {
      ModBodyPart partType = ModBodyPart::Count;
      for (const auto& inst : data->partInstances) {
        if (inst.partId == snap.ownerPartId) {
          partType = inst.partType;
          break;
        }
      }
      if (partType == ModBodyPart::LeftUpperArm || partType == ModBodyPart::RightUpperArm) {
        snap.localPosition.y += 0.5f;
      }
    }

    // --- 追加の腕 (下側の2本) の生成処理 ---
    std::vector<ModPartInstanceData> newArms;
    std::vector<ModControlPointSnapshot> newSnaps;
    int chestId = -1;
    for (const auto& inst : data->partInstances) {
      if (inst.partType == ModBodyPart::ChestBody) chestId = inst.partId;
    }

    int nextId = 100;
    for (int armType = static_cast<int>(ModBodyPart::LeftUpperArm); armType <= static_cast<int>(ModBodyPart::RightForeArm); ++armType) {
      for (const auto& inst : data->partInstances) {
        if (static_cast<int>(inst.partType) == armType) {
          ModPartInstanceData extraArm = inst;
          extraArm.partId = nextId;
          if (inst.partType == ModBodyPart::LeftUpperArm || inst.partType == ModBodyPart::RightUpperArm) {
             extraArm.parentId = chestId; // attach to chest
             // 胴体 1.5倍基準の位置 (x = ±3.075, y = 1.725) から y -= 1.0f, x ±= 0.6f に配置
             extraArm.localTransform.translate.y = 0.725f;
             extraArm.resolvedLocalTranslate.y = 0.725f;
             if (inst.partType == ModBodyPart::LeftUpperArm) {
               extraArm.localTransform.translate.x = -3.675f;
               extraArm.resolvedLocalTranslate.x = -3.675f;
             } else {
               extraArm.localTransform.translate.x = 3.675f;
               extraArm.resolvedLocalTranslate.x = 3.675f;
             }
          } else {
             extraArm.parentId = nextId - 1;
          }
          // 追加アームのスケールも 1.0f に維持 (CP変形で太さ・長さを調整するため)
          extraArm.param.scale.x = 1.0f;
          extraArm.param.scale.y = 1.0f;
          extraArm.param.scale.z = 1.0f;

          newArms.push_back(extraArm);

          for (const auto& snap : data->controlPointSnapshots) {
            if (snap.ownerPartId == inst.partId) {
              ModControlPointSnapshot extraSnap = snap;
              extraSnap.ownerPartId = extraArm.partId;
              if (inst.partType == ModBodyPart::LeftUpperArm || inst.partType == ModBodyPart::RightUpperArm) {
                 // 胴体 1.5倍基準の位置 (y = 1.725) から y -= 1.0f, x ±= 0.6f にCPもオフセット
                 extraSnap.localPosition.y -= 1.0f;
                 if (inst.partType == ModBodyPart::LeftUpperArm) {
                   extraSnap.localPosition.x -= 0.6f;
                 } else {
                   extraSnap.localPosition.x += 0.6f;
                 }
              }
              newSnaps.push_back(extraSnap);
            }
          }
          nextId++;
          break;
        }
      }
    }

    data->partInstances.insert(data->partInstances.end(), newArms.begin(), newArms.end());
    data->controlPointSnapshots.insert(data->controlPointSnapshots.end(), newSnaps.begin(), newSnaps.end());

    // --- CP評価・描画同期スケーリング (ここで全ての変形サイズを一元適用します) ---
    for (auto& snap : data->controlPointSnapshots) {
      ModBodyPart partType = ModBodyPart::Count;
      for (const auto& inst : data->partInstances) {
        if (inst.partId == snap.ownerPartId) {
          partType = inst.partType;
          break;
        }
      }

      // 胴体 (ChestBody / StomachBody) のCP: 太さ・長さ・奥行きすべて 1.5倍 にスケール
      if (partType == ModBodyPart::ChestBody || partType == ModBodyPart::StomachBody) {
        snap.radius *= 1.5f;
        snap.localPosition.x *= 1.5f;
        snap.localPosition.y *= 1.5f;
        snap.localPosition.z *= 1.5f;
      }
      // 腕 (元の腕および追加された4本すべての腕) のCP: 太さを 1.3倍、長さを 1.5倍 にスケール
      else if (partType == ModBodyPart::LeftUpperArm || partType == ModBodyPart::RightUpperArm ||
               partType == ModBodyPart::LeftForeArm || partType == ModBodyPart::RightForeArm) {
        snap.radius *= 1.3f;
        snap.localPosition.x *= 1.3f;
        snap.localPosition.y *= 1.5f;
        snap.localPosition.z *= 1.3f;
      }
    }
    // -----------------------------------------------------------------------------------------
    break;
  }

  case NpcPresetType::OctopusLegs:
  {
    // 腕・脚の描画用スケール (inst.param.scale) は親子重複による異常変形や接続崩れを防ぐため 1.0f (変更なし) に維持し、
    // すべて CP (controlPointSnapshots) のスケーリングによってモデル描画と評価の両方を綺麗に変形・同期させます。
    for (auto& inst : data->partInstances) {
      if (inst.partType == ModBodyPart::LeftUpperArm || inst.partType == ModBodyPart::RightUpperArm ||
          inst.partType == ModBodyPart::LeftForeArm || inst.partType == ModBodyPart::RightForeArm) {
        inst.param.scale.x = 1.0f;
        inst.param.scale.y = 1.0f;
        inst.param.scale.z = 1.0f;
      }
      if (inst.partType == ModBodyPart::LeftThigh || inst.partType == ModBodyPart::RightThigh ||
          inst.partType == ModBodyPart::LeftShin || inst.partType == ModBodyPart::RightShin) {
        inst.param.scale.x = 1.0f;
        inst.param.scale.y = 1.0f;
        inst.param.scale.z = 1.0f;
        inst.param.length = 1.0f; // 元のlengthも等倍に維持
      }

      // --- スケール等倍化に伴う、子パーツ（前腕・下腿）の接続位置（translate）の完全手動同期 ---
      if (inst.partType == ModBodyPart::LeftForeArm || inst.partType == ModBodyPart::RightForeArm) {
        // 上腕が0.7倍の長さになったため、前腕の接続位置(デフォルト -1.08f) を0.7倍の -0.756f にオフセット
        inst.localTransform.translate.y = -0.756f;
        inst.resolvedLocalTranslate.y = -0.756f;
      }
      if (inst.partType == ModBodyPart::LeftShin || inst.partType == ModBodyPart::RightShin) {
        // 大腿が1.2倍の長さになったため、下腿の接続位置(デフォルト -1.57f) を1.2倍の -1.884f にオフセット
        inst.localTransform.translate.y = -1.884f;
        inst.resolvedLocalTranslate.y = -1.884f;
      }
    }

    std::vector<ModPartInstanceData> newLegs;
    std::vector<ModControlPointSnapshot> newSnaps;

    int parentIdLeft = -1;
    int parentIdRight = -1;
    for (const auto& inst : data->partInstances) {
      if (inst.partType == ModBodyPart::LeftThigh) parentIdLeft = inst.parentId;
      if (inst.partType == ModBodyPart::RightThigh) parentIdRight = inst.parentId;
    }

    // Radial spread directions (dirZ, dirX multiplier)
    struct RadialDir { float dirZ; float dirX; };
    RadialDir dirs[4] = {
      { 1.0f, 0.0f },    // Pair 0: Front
      { 0.5f, 1.0f },    // Pair 1: Outer-Front
      { -0.5f, 1.0f },   // Pair 2: Outer-Back
      { -1.0f, 0.0f }    // Pair 3: Back
    };

    int nextId = 200;
    
    // Process Pair 0 (Original legs) - Apply offset ONLY to Bend and End snapshots
    for (auto& snap : data->controlPointSnapshots) {
      ModBodyPart partType = ModBodyPart::Count;
      for (const auto& inst : data->partInstances) {
        if (inst.partId == snap.ownerPartId) {
          partType = inst.partType;
          break;
        }
      }
      if (partType == ModBodyPart::LeftThigh || partType == ModBodyPart::RightThigh || 
          partType == ModBodyPart::LeftShin || partType == ModBodyPart::RightShin) {
        float dirZ = dirs[0].dirZ;
        float dirX = (partType == ModBodyPart::LeftThigh || partType == ModBodyPart::LeftShin) ? -dirs[0].dirX : dirs[0].dirX;
        
        if (partType == ModBodyPart::LeftThigh || partType == ModBodyPart::RightThigh) {
          if (snap.role == ModControlPointRole::Bend) {
            snap.localPosition.z += dirZ * 0.7f;
            snap.localPosition.x += dirX * 0.7f;
          } else if (snap.role == ModControlPointRole::End) {
            snap.localPosition.z += dirZ * 1.8f;
            snap.localPosition.x += dirX * 1.8f;
          }
        } else {
          // Shins
          if (snap.role == ModControlPointRole::Root) {
            snap.localPosition.z += dirZ * 0.7f;
            snap.localPosition.x += dirX * 0.7f;
          } else if (snap.role == ModControlPointRole::End) {
            snap.localPosition.z += dirZ * 1.8f;
            snap.localPosition.x += dirX * 1.8f;
          }
        }
      }
    }

    // Process Pair 1, 2, 3 (Extra legs)
    for (int pairIdx = 1; pairIdx < 4; ++pairIdx) {
      float dirZ = dirs[pairIdx].dirZ;
      float dirXBase = dirs[pairIdx].dirX;

      int extraLeftThighId = -1;
      int extraRightThighId = -1;

      for (int legType = static_cast<int>(ModBodyPart::LeftThigh); legType <= static_cast<int>(ModBodyPart::RightShin); ++legType) {
        for (const auto& inst : data->partInstances) {
          if (static_cast<int>(inst.partType) == legType) {
            ModPartInstanceData extraLeg = inst;
            extraLeg.partId = nextId;

            if (inst.partType == ModBodyPart::LeftThigh || inst.partType == ModBodyPart::RightThigh) {
               extraLeg.parentId = (inst.partType == ModBodyPart::LeftThigh) ? parentIdLeft : parentIdRight;
               
               if (inst.partType == ModBodyPart::LeftThigh) extraLeftThighId = nextId;
               if (inst.partType == ModBodyPart::RightThigh) extraRightThighId = nextId;
               
               // Fix the visual Y-coordinate gap for extra legs ONLY!
               extraLeg.localTransform.translate.y += 0.4f;
               extraLeg.resolvedLocalTranslate.y += 0.4f;
               
            } else if (inst.partType == ModBodyPart::LeftShin) {
               extraLeg.parentId = extraLeftThighId;
            } else if (inst.partType == ModBodyPart::RightShin) {
               extraLeg.parentId = extraRightThighId;
            }

            // 追加の脚のスケールも 1.0f に維持
            extraLeg.param.scale.x = 1.0f;
            extraLeg.param.scale.y = 1.0f;
            extraLeg.param.scale.z = 1.0f;
            extraLeg.param.length = 1.0f;

            newLegs.push_back(extraLeg);

            for (const auto& snap : data->controlPointSnapshots) {
              if (snap.ownerPartId == inst.partId) {
                ModControlPointSnapshot extraSnap = snap;
                extraSnap.ownerPartId = extraLeg.partId;
                
                if (inst.partType == ModBodyPart::LeftThigh || inst.partType == ModBodyPart::RightThigh ||
                    inst.partType == ModBodyPart::LeftShin || inst.partType == ModBodyPart::RightShin) {
                   float dirX = (inst.partType == ModBodyPart::LeftThigh || inst.partType == ModBodyPart::LeftShin) ? -dirXBase : dirXBase;
                   float baseDirX = (inst.partType == ModBodyPart::LeftThigh || inst.partType == ModBodyPart::LeftShin) ? -dirs[0].dirX : dirs[0].dirX;
                   
                   if (inst.partType == ModBodyPart::LeftThigh || inst.partType == ModBodyPart::RightThigh) {
                     if (extraSnap.role == ModControlPointRole::Bend) {
                       extraSnap.localPosition.z += (dirZ * 0.7f) - 0.7f;
                       extraSnap.localPosition.x += (dirX - baseDirX) * 0.7f;
                     } else if (extraSnap.role == ModControlPointRole::End) {
                       extraSnap.localPosition.z += (dirZ * 1.8f) - 1.8f;
                       extraSnap.localPosition.x += (dirX - baseDirX) * 1.8f;
                     }
                   } else {
                     // Shins
                     if (extraSnap.role == ModControlPointRole::Root) {
                       extraSnap.localPosition.z += (dirZ * 0.7f) - 0.7f;
                       extraSnap.localPosition.x += (dirX - baseDirX) * 0.7f;
                     } else if (extraSnap.role == ModControlPointRole::End) {
                       extraSnap.localPosition.z += (dirZ * 1.8f) - 1.8f;
                       extraSnap.localPosition.x += (dirX - baseDirX) * 1.8f;
                     }
                   }
                }
                newSnaps.push_back(extraSnap);
              }
            }
            nextId++;
            break;
          }
        }
      }
    }

    data->partInstances.insert(data->partInstances.end(), newLegs.begin(), newLegs.end());
    data->controlPointSnapshots.insert(data->controlPointSnapshots.end(), newSnaps.begin(), newSnaps.end());

    // --- CP評価・描画同期スケーリング (ここで全ての変形サイズを一元適用します) ---
    for (auto& snap : data->controlPointSnapshots) {
      ModBodyPart partType = ModBodyPart::Count;
      for (const auto& inst : data->partInstances) {
        if (inst.partId == snap.ownerPartId) {
          partType = inst.partType;
          break;
        }
      }

      // 腕 (元の腕および追加されたすべての腕) のCP: 太さ・長さともに 0.7倍 にスケール
      if (partType == ModBodyPart::LeftUpperArm || partType == ModBodyPart::RightUpperArm ||
          partType == ModBodyPart::LeftForeArm || partType == ModBodyPart::RightForeArm) {
        snap.radius *= 0.7f;
        snap.localPosition.x *= 0.7f;
        snap.localPosition.y *= 0.7f;
        snap.localPosition.z *= 0.7f;
      }
      // 脚 (元の脚および追加されたすべての脚) のCP: radius(太さ)を 0.6倍、長さ方向(y)を 1.2倍 にスケーリング
      else if (partType == ModBodyPart::LeftThigh || partType == ModBodyPart::RightThigh ||
               partType == ModBodyPart::LeftShin || partType == ModBodyPart::RightShin) {
        snap.radius *= 0.6f;
        // ※放射状広がり (x, z) への干渉を避けるため、パーツ本来の長さ方向 (localPosition.y) のみ 1.2倍 にスケールします
        snap.localPosition.y *= 1.2f;
      }
    }
    // -----------------------------------------------------------------------------------------
    break;
  }
  }

  NormalizeCustomizeData(*data);
  return data;
}

void ModCustomizeDataStore::SetSharedNpcCustomizeData(int rankIndex, const ModBodyCustomizeData &data) {
  std::vector<std::unique_ptr<ModBodyCustomizeData>> &storage = SharedNpcCustomizeDataStorage();
  if (rankIndex >= storage.size()) {
    storage.resize(rankIndex + 1);
  }
  ModBodyCustomizeData normalized = data;
  NormalizeCustomizeData(normalized);
  storage[rankIndex] = std::make_unique<ModBodyCustomizeData>(normalized);
}

const ModBodyCustomizeData *ModCustomizeDataStore::GetSharedNpcCustomizeData(int rankIndex) {
  std::vector<std::unique_ptr<ModBodyCustomizeData>> &storage = SharedNpcCustomizeDataStorage();
  if (rankIndex >= 0 && rankIndex < storage.size()) {
    return storage[rankIndex].get();
  }
  return nullptr;
}

void ModCustomizeDataStore::ClearSharedNpcCustomizeData() {
  SharedNpcCustomizeDataStorage().clear();
}

// トラベルシーンでのプレイヤー到着順位
static int s_travelFinishRank = -1;

void ModCustomizeDataStore::SetTravelFinishRank(int rank) {
  s_travelFinishRank = rank;
}

int ModCustomizeDataStore::GetTravelFinishRank() {
  return s_travelFinishRank;
}

void ModCustomizeDataStore::NormalizeCustomizeData(ModBodyCustomizeData &data) {
  data.dataVersion = 2;

  // 新方式の partInstances を基準に旧固定配列を再構築する
  ClearLegacyPartParamsLocal(data);

  std::array<bool, static_cast<size_t>(ModBodyPart::Count)> hasRepresentative{};
  std::array<int, static_cast<size_t>(ModBodyPart::Count)> counts{};

  for (size_t i = 0; i < data.partInstances.size(); ++i) {
    ModPartInstanceData &instance = data.partInstances[i];

    if (!IsValidPartTypeLocal(instance.partType)) {
      continue;
    }

    instance.param.count = 1;

    const size_t index = static_cast<size_t>(instance.partType);
    counts[index] += 1;

    if (!hasRepresentative[index]) {
      data.partParams[index] = instance.param;
      data.partParams[index].count = 1;
      hasRepresentative[index] = true;
    }
  }

  for (size_t i = 0; i < counts.size(); ++i) {
    data.partParams[i].count = counts[i];
    if (counts[i] <= 0) {
      data.partParams[i].enabled = false;
    }
  }

  // 可変長操作点から旧固定操作点へも反映しておく
  for (size_t i = 0; i < data.controlPointSnapshots.size(); ++i) {
    ApplySnapshotToLegacyControlPointLocal(data.controlPoints,
                                           data.controlPointSnapshots[i]);
  }
}