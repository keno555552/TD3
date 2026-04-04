#include "ModCustomizeDataStore.h"

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

  // 制限時間初期化
  data.timeLimit_ = 30.0f;
  data.totalTimeLimit_ = 30.0f;
  data.isTimeUp_ = false;

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

  if (data.timeLimit_ < 0.0f) {
    data.timeLimit_ = 0.0f;
  }

  if (data.totalTimeLimit_ < 0.0f) {
    data.totalTimeLimit_ = 0.0f;
  }
}