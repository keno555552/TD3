#include "ModBody.h"

/*   「キャラクターの体の改造処理」を行うクラス   */

namespace {
/*   各パーツのスケールを計算する関数   */
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

/*   Bodyのスケール変化に合わせて
各パーツのオフセットをスケールする   */
Vector3 ScaleOffsetByBody(const Vector3 &offset, const Vector3 &bodyScale) {
  return {
      offset.x * bodyScale.x,
      offset.y * bodyScale.y,
      offset.z * bodyScale.z,
  };
}

/*   length変更による位置補正   */
Vector3 GetLengthAnchorOffset(ModBodyPart part, const Vector3 &baseScale,
                              const Vector3 &newScale) {

  // スケール差分
  float diffY = newScale.y - baseScale.y;

  switch (part) {
    // 頭は上方向へ補正
  case ModBodyPart::Head:
    return {0.0f, diffY * 0.5f, 0.0f};
  // 腕脚は下方向へ補正
  case ModBodyPart::LeftArm:
  case ModBodyPart::RightArm:
  case ModBodyPart::LeftLeg:
  case ModBodyPart::RightLeg:
    return {0.0f, -diffY * 0.5f, 0.0f};
  // Bodyは補正なし
  case ModBodyPart::Body:
  case ModBodyPart::Count:
  default:
    return {0.0f, 0.0f, 0.0f};
  }
}
} // namespace

/*   初期化   */
void ModBody::Initialize(Object *target) {
  CacheBaseTransforms(target);
  Reset();
}

/*   改造をObjectへ適用する   */
void ModBody::Apply(Object *target) {
  if (target == nullptr) {
    return;
  }

  CacheBaseTransforms(target);

  // パーツ数が変わっていたら処理しない
  if (basePartTransforms_.size() != target->objectParts_.size()) {
    return;
  }

  // 一度すべて元のTransformへ戻す
  for (size_t i = 0; i < target->objectParts_.size(); ++i) {
    target->objectParts_[i].transform = basePartTransforms_[i];
  }

  // Bodyを先に適用
  ApplyBody(target);

  // 子パーツを順番に適用
  for (size_t i = 0; i < static_cast<size_t>(ModBodyPart::Count); ++i) {
    ModBodyPart part = static_cast<ModBodyPart>(i);
    if (part == ModBodyPart::Body) {
      continue;
    }
    ApplyChildPart(target, part);
  }
}

/*   各パーツとObjectPartのインデックスを対応させる   */
void ModBody::SetPartIndex(ModBodyPart part, int index) {
  partIndexMap_.indices[static_cast<size_t>(part)] = index;
}

/*   パーツのインデックスを取得   */
int ModBody::GetPartIndex(ModBodyPart part) const {
  return partIndexMap_.indices[static_cast<size_t>(part)];
}

/*   改造パラメータを初期値へ戻す   */
void ModBody::Reset() {
  for (auto &part : data_.parts) {
    part.scale = {1.0f, 1.0f, 1.0f};
    part.length = 1.0f;
    part.count = 1;
    part.enabled = true;
  }
}

/*
Objectの元Transformを保存する

初回のみ保存して、パーツ数が変わったら再保存
*/
void ModBody::CacheBaseTransforms(Object *target) {
  if (target == nullptr) {
    return;
  }

  if (!isBaseCached_ ||
      basePartTransforms_.size() != target->objectParts_.size()) {
    // メインTransform保存
    baseMainTransform_ = target->mainPosition.transform;

    // 各パーツのTransform保存
    basePartTransforms_.clear();
    basePartTransforms_.reserve(target->objectParts_.size());

    for (auto &part : target->objectParts_) {
      basePartTransforms_.push_back(part.transform);
    }

    /*
    Bodyから各パーツまでの距離を保存

    改造時にBody基準で再配置するため
    */
    baseOffsetsFromBody_.clear();
    baseOffsetsFromBody_.resize(target->objectParts_.size(),
                                Vector3{0.0f, 0.0f, 0.0f});

    int bodyIndex = GetPartIndex(ModBodyPart::Body);
    if (bodyIndex >= 0 &&
        bodyIndex < static_cast<int>(basePartTransforms_.size())) {
      Vector3 bodyTranslate = basePartTransforms_[bodyIndex].translate;
      for (size_t i = 0; i < basePartTransforms_.size(); ++i) {
        baseOffsetsFromBody_[i] =
            basePartTransforms_[i].translate - bodyTranslate;
      }
    }

    isBaseCached_ = true;
  }
}

/*   Bodyパーツを適用   */
void ModBody::ApplyBody(Object *target) {
  int bodyIndex = GetPartIndex(ModBodyPart::Body);
  if (bodyIndex < 0 ||
      bodyIndex >= static_cast<int>(target->objectParts_.size())) {
    return;
  }

  const ModBodyPartParam &param =
      data_.parts[static_cast<size_t>(ModBodyPart::Body)];
  Transform bodyTransform = basePartTransforms_[bodyIndex];
  bodyTransform.scale = MakePartScale(bodyTransform.scale, param);
  target->objectParts_[bodyIndex].transform = bodyTransform;
}

/*   Body以外のパーツを適用   */
void ModBody::ApplyChildPart(Object *target, ModBodyPart part) {
  int bodyIndex = GetPartIndex(ModBodyPart::Body);
  int index = GetPartIndex(part);

  if (bodyIndex < 0 ||
      bodyIndex >= static_cast<int>(target->objectParts_.size())) {
    return;
  }
  if (index < 0 || index >= static_cast<int>(target->objectParts_.size())) {
    return;
  }

  const ModBodyPartParam &param = data_.parts[static_cast<size_t>(part)];

  Transform transform = basePartTransforms_[index];

  // Body基準で配置
  const Transform &bodyTransform = target->objectParts_[bodyIndex].transform;
  transform.translate =
      Add(bodyTransform.translate,
          ScaleOffsetByBody(baseOffsetsFromBody_[index], bodyTransform.scale));

  // 新しいスケール計算
  Vector3 newScale = MakePartScale(basePartTransforms_[index].scale, param);

  // pivot補正
  transform.translate = Add(
      transform.translate,
      GetLengthAnchorOffset(part, basePartTransforms_[index].scale, newScale));
  transform.scale = newScale;

  target->objectParts_[index].transform = transform;
}