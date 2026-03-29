#include "ModCustomizeDataStore.h"

namespace {

ModBodyCustomizeData MakeDefaultCustomizeDataLocal() {
  ModBodyCustomizeData data{};
  data.dataVersion = 2;

  for (auto &param : data.partParams) {
    param.scale = {1.0f, 1.0f, 1.0f};
    param.length = 1.0f;
    param.count = 1;
    param.enabled = true;
  }

  for (auto &offset : data.bodyJointOffsets) {
    offset = {0.0f, 0.0f, 0.0f};
  }

  data.bodyJointOffsets[static_cast<size_t>(ModBodyPart::Neck)] = {0.0f, 1.0f, 0.0f};
  data.bodyJointOffsets[static_cast<size_t>(ModBodyPart::LeftUpperArm)] = {-1.25f, 1.0f, 0.0f};
  data.bodyJointOffsets[static_cast<size_t>(ModBodyPart::RightUpperArm)] = {1.25f, 1.0f, 0.0f};
  data.bodyJointOffsets[static_cast<size_t>(ModBodyPart::LeftThigh)] = {-0.5f, -1.25f, 0.0f};
  data.bodyJointOffsets[static_cast<size_t>(ModBodyPart::RightThigh)] = {0.5f, -1.25f, 0.0f};

  data.controlPointSnapshots.clear();
  data.timeLimit_ = 30.0f;
  data.totalTimeLimit_ = 30.0f;
  data.isTimeUp_ = false;

  return data;
}

std::unique_ptr<ModBodyCustomizeData> &SharedCustomizeDataStorage() {
  static std::unique_ptr<ModBodyCustomizeData> sharedData = nullptr;
  return sharedData;
}

bool IsValidPartTypeLocal(ModBodyPart part) {
  const size_t index = static_cast<size_t>(part);
  return index < static_cast<size_t>(ModBodyPart::Count);
}

void ClearLegacyPartParamsLocal(ModBodyCustomizeData &data) {
  for (size_t i = 0; i < static_cast<size_t>(ModBodyPart::Count); ++i) {
    data.partParams[i].scale = {1.0f, 1.0f, 1.0f};
    data.partParams[i].length = 1.0f;
    data.partParams[i].count = 0;
    data.partParams[i].enabled = false;
  }
}

void ApplySnapshotToLegacyControlPointLocal(
    ModControlPointData &legacy, const ModControlPointSnapshot &snapshot) {
  switch (snapshot.role) {
  case ModControlPointRole::LeftShoulder:
    legacy.leftShoulderPos = snapshot.localPosition;
    break;
  case ModControlPointRole::LeftElbow:
    legacy.leftElbowPos = snapshot.localPosition;
    break;
  case ModControlPointRole::LeftWrist:
    legacy.leftWristPos = snapshot.localPosition;
    break;

  case ModControlPointRole::RightShoulder:
    legacy.rightShoulderPos = snapshot.localPosition;
    break;
  case ModControlPointRole::RightElbow:
    legacy.rightElbowPos = snapshot.localPosition;
    break;
  case ModControlPointRole::RightWrist:
    legacy.rightWristPos = snapshot.localPosition;
    break;

  case ModControlPointRole::LeftHip:
    legacy.leftHipPos = snapshot.localPosition;
    break;
  case ModControlPointRole::LeftKnee:
    legacy.leftKneePos = snapshot.localPosition;
    break;
  case ModControlPointRole::LeftAnkle:
    legacy.leftAnklePos = snapshot.localPosition;
    break;

  case ModControlPointRole::RightHip:
    legacy.rightHipPos = snapshot.localPosition;
    break;
  case ModControlPointRole::RightKnee:
    legacy.rightKneePos = snapshot.localPosition;
    break;
  case ModControlPointRole::RightAnkle:
    legacy.rightAnklePos = snapshot.localPosition;
    break;

  case ModControlPointRole::Chest:
    legacy.chestPos = snapshot.localPosition;
    break;
  case ModControlPointRole::Belly:
    legacy.bellyPos = snapshot.localPosition;
    break;
  case ModControlPointRole::Waist:
    legacy.waistPos = snapshot.localPosition;
    break;

  case ModControlPointRole::LowerNeck:
    legacy.lowerNeckPos = snapshot.localPosition;
    break;
  case ModControlPointRole::UpperNeck:
    legacy.upperNeckPos = snapshot.localPosition;
    break;
  case ModControlPointRole::HeadCenter:
    legacy.headCenterPos = snapshot.localPosition;
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