#include "ModBody.h"

namespace {

size_t ToIndex(ModBodyPart part) { return static_cast<size_t>(part); }

Vector3 AddV(const Vector3 &a, const Vector3 &b) {
  return {a.x + b.x, a.y + b.y, a.z + b.z};
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

  case ModBodyPart::Body:
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

} // namespace

void ModBody::Initialize(Object *target, ModBodyPart part) {
  // この ModBody が担当する部位種別を保存する
  part_ = part;

  // 元の transform を保存して、後で基準状態から再計算できるようにする
  CacheBaseTransforms(target);

  // 改造パラメータを初期状態に戻す
  Reset();
}

void ModBody::Reset() {
  // スケール倍率を等倍に戻す
  param_.scale = {1.0f, 1.0f, 1.0f};

  // 長さ倍率を初期値に戻す
  param_.length = 1.0f;

  // 部位数を初期値に戻す
  param_.count = 1;

  // 表示状態を有効に戻す
  param_.enabled = true;
}

Vector3 ModBody::GetVisualScaleRatio() const {
  // scale と length を合成して、見た目に使う最終倍率を作る
  Vector3 ratio = {param_.scale.x, param_.scale.y * param_.length,
                   param_.scale.z};

  // 無効化されている部位は見た目スケール 0 扱いで返す
  if (!param_.enabled) {
    return {0.0f, 0.0f, 0.0f};
  }

  // 有効な場合はそのまま最終倍率を返す
  return ratio;
}

std::unique_ptr<ModBodyCustomizeData> ModBody::CreateDefaultCustomizeData() {
  // 初期状態の改造共有データを生成して返す
  return std::make_unique<ModBodyCustomizeData>(MakeDefaultCustomizeData());
}

std::unique_ptr<ModBodyCustomizeData> ModBody::CopySharedCustomizeData() {
  // 現在共有されている改造データを参照する
  const std::unique_ptr<ModBodyCustomizeData> &sharedData =
      SharedCustomizeDataStorage();

  // 共有データがまだ存在しない場合は nullptr を返す
  if (sharedData == nullptr) {
    return nullptr;
  }

  // 共有データをコピーして返す
  return std::make_unique<ModBodyCustomizeData>(*sharedData);
}

void ModBody::SetSharedCustomizeData(const ModBodyCustomizeData &data) {
  // 改造共有データをディープコピーして静的領域に保存する
  SharedCustomizeDataStorage() = std::make_unique<ModBodyCustomizeData>(data);
}

const ModBodyCustomizeData *ModBody::GetSharedCustomizeData() {
  // 現在共有されている改造データの実体ポインタを返す
  return SharedCustomizeDataStorage().get();
}

void ModBody::CacheBaseTransforms(Object *target) {
  // 対象が無い場合は何もしない
  if (target == nullptr) {
    return;
  }

  // 見た目パーツが存在しない場合は保存できないので終了する
  if (target->objectParts_.empty()) {
    return;
  }

  // 初回だけ root と mesh の transform を保存する
  if (!isBaseCached_) {
    baseMainTransform_ = target->mainPosition.transform;
    baseMeshTransform_ = target->objectParts_[0].transform;
    isBaseCached_ = true;
  }
}

void ModBody::Apply(Object *target) {
  // 対象が無い場合は何もしない
  if (target == nullptr) {
    return;
  }

  // 見た目パーツが存在しない場合は反映できないので終了する
  if (target->objectParts_.empty()) {
    return;
  }

  // 初期 transform が未保存ならここで保存する
  CacheBaseTransforms(target);

  // root 側は接続基準として使うので、スケールは元の状態に戻しておく
  Transform root = target->mainPosition.transform;
  root.scale = baseMainTransform_.scale;
  target->mainPosition.transform = root;

  // mesh 側は元の transform から毎回作り直す
  Transform mesh = baseMeshTransform_;

  // scale と length から見た目用の最終スケールを計算する
  Vector3 newScale = MakePartScale(baseMeshTransform_.scale, param_);

  // 計算したスケールを mesh に設定する
  mesh.scale = newScale;

  // 部位ごとの根元基準に合わせて、見た目中心の位置を補正する
  mesh.translate =
      AddV(baseMeshTransform_.translate, GetAnchorOffset(part_, newScale));

  // 計算結果を見た目パーツへ反映する
  target->objectParts_[0].transform = mesh;
}