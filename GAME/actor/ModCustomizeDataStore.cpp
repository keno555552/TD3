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

  switch (type) {
  case NpcPresetType::Default:
    break;

  case NpcPresetType::HeadBig:
    for (auto& inst : data->partInstances) {
      if (inst.partType == ModBodyPart::Head) {
        inst.param.scale.x *= 2.0f;
        inst.param.scale.y *= 2.0f;
        inst.param.scale.z *= 2.0f;
      } else if (inst.partType == ModBodyPart::Neck) {
        inst.param.scale.x *= 1.15f;
        inst.param.scale.z *= 1.15f;
      }
    }
    break;

  case NpcPresetType::LongLeg:
    for (auto& inst : data->partInstances) {
      if (inst.partType == ModBodyPart::LeftThigh || inst.partType == ModBodyPart::RightThigh ||
          inst.partType == ModBodyPart::LeftShin || inst.partType == ModBodyPart::RightShin) {
        inst.param.scale.y *= 2.0f;
      }
    }
    break;

  case NpcPresetType::BigTorso:
    for (auto& inst : data->partInstances) {
      if (inst.partType == ModBodyPart::ChestBody || inst.partType == ModBodyPart::StomachBody) {
        inst.param.scale.x *= 2.0f;
        inst.param.scale.y *= 2.0f;
        inst.param.scale.z *= 2.0f;
      }
    }
    break;

  case NpcPresetType::Gorilla:
    for (auto& inst : data->partInstances) {
      if (inst.partType == ModBodyPart::LeftUpperArm || inst.partType == ModBodyPart::RightUpperArm ||
          inst.partType == ModBodyPart::LeftForeArm || inst.partType == ModBodyPart::RightForeArm) {
        inst.param.scale.x *= 1.5f;
        inst.param.scale.z *= 1.5f;
        inst.param.scale.y *= 1.3f;
      } else if (inst.partType == ModBodyPart::ChestBody) {
        inst.param.scale.x *= 1.6f;
        inst.param.scale.z *= 1.4f;
      } else if (inst.partType == ModBodyPart::LeftThigh || inst.partType == ModBodyPart::RightThigh ||
                 inst.partType == ModBodyPart::LeftShin || inst.partType == ModBodyPart::RightShin) {
        inst.param.scale.y *= 0.6f;
      }
    }
    break;

  case NpcPresetType::Slender:
    for (auto& inst : data->partInstances) {
      inst.param.scale.x *= 0.6f;
      inst.param.scale.z *= 0.6f;
      if (inst.partType == ModBodyPart::LeftThigh || inst.partType == ModBodyPart::RightThigh ||
          inst.partType == ModBodyPart::LeftShin || inst.partType == ModBodyPart::RightShin ||
          inst.partType == ModBodyPart::ChestBody || inst.partType == ModBodyPart::StomachBody) {
        inst.param.scale.y *= 1.3f;
      }
    }
    break;

  case NpcPresetType::Chubby:
    for (auto& inst : data->partInstances) {
      inst.param.scale.x *= 1.8f;
      inst.param.scale.z *= 1.8f;
      if (inst.partType == ModBodyPart::LeftThigh || inst.partType == ModBodyPart::RightThigh ||
          inst.partType == ModBodyPart::LeftShin || inst.partType == ModBodyPart::RightShin) {
        inst.param.scale.y *= 0.8f;
      } else if (inst.partType == ModBodyPart::StomachBody) {
        inst.param.scale.x *= 1.3f;
        inst.param.scale.z *= 1.5f;
      }
    }
    break;

  case NpcPresetType::Giant:
    for (auto& inst : data->partInstances) {
      inst.param.scale.x *= 2.0f;
      inst.param.scale.y *= 2.0f;
      inst.param.scale.z *= 2.0f;
    }
    break;

  case NpcPresetType::Mini:
    for (auto& inst : data->partInstances) {
      inst.param.scale.x *= 0.5f;
      inst.param.scale.y *= 0.5f;
      inst.param.scale.z *= 0.5f;
    }
    break;

  case NpcPresetType::LongArm:
    for (auto& inst : data->partInstances) {
      if (inst.partType == ModBodyPart::LeftUpperArm || inst.partType == ModBodyPart::RightUpperArm ||
          inst.partType == ModBodyPart::LeftForeArm || inst.partType == ModBodyPart::RightForeArm) {
        inst.param.scale.y *= 2.0f;
      }
    }
    break;

  case NpcPresetType::WideShoulder:
    for (auto& snap : data->controlPointSnapshots) {
      ModBodyPart partType = ModBodyPart::Count;
      for (const auto& inst : data->partInstances) {
        if (inst.partId == snap.ownerPartId) {
          partType = inst.partType;
          break;
        }
      }
      if (partType == ModBodyPart::LeftUpperArm) {
        snap.localPosition.x -= 1.5f;
        snap.localPosition.y += 0.5f;
      } else if (partType == ModBodyPart::RightUpperArm) {
        snap.localPosition.x += 1.5f;
        snap.localPosition.y += 0.5f;
      }
    }
    break;

  case NpcPresetType::WideHip:
    for (auto& snap : data->controlPointSnapshots) {
      ModBodyPart partType = ModBodyPart::Count;
      for (const auto& inst : data->partInstances) {
        if (inst.partId == snap.ownerPartId) {
          partType = inst.partType;
          break;
        }
      }
      if (partType == ModBodyPart::LeftThigh) {
        snap.localPosition.x -= 1.5f;
      } else if (partType == ModBodyPart::RightThigh) {
        snap.localPosition.x += 1.5f;
      }
    }
    break;

  case NpcPresetType::LowHead:
    for (auto& snap : data->controlPointSnapshots) {
      ModBodyPart partType = ModBodyPart::Count;
      for (const auto& inst : data->partInstances) {
        if (inst.partId == snap.ownerPartId) {
          partType = inst.partType;
          break;
        }
      }
      if (partType == ModBodyPart::Neck || partType == ModBodyPart::Head) {
        snap.localPosition.y -= 1.0f;
        snap.localPosition.z += 0.8f; // Move forward slightly for a hunched look
      }
    }
    break;
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