#include "ModBody.h"

/*   1部位1Object 用の改造処理クラス   */

namespace {

/*   各パーツの見た目スケールを計算する   */
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

/*   root 基準で mesh の中心位置補正を返す
----------------------------------------
前提:
- Head の root は首元(下端)
- Arm / Leg の root は肩・股関節(上端)
- Body の root は中心
----------------------------------------*/
Vector3 GetAnchorOffset(ModBodyPart part, const Vector3 &newScale) {
  switch (part) {
  case ModBodyPart::Head:
    return {0.0f, newScale.y * 0.5f, 0.0f};

  case ModBodyPart::LeftArm:
  case ModBodyPart::RightArm:
  case ModBodyPart::LeftLeg:
  case ModBodyPart::RightLeg:
    return {0.0f, -newScale.y * 0.5f, 0.0f};

  case ModBodyPart::Body:
  case ModBodyPart::Count:
  default:
    return {0.0f, 0.0f, 0.0f};
  }
}

} // namespace

/*   初期化   */
void ModBody::Initialize(Object *target, ModBodyPart part) {
  part_ = part;
  CacheBaseTransforms(target);
  Reset();
}

/*   改造パラメータを初期値へ戻す   */
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

/*   初期 transform 保存   */
void ModBody::CacheBaseTransforms(Object *target) {
  if (target == nullptr) {
    return;
  }

  if (target->objectParts_.empty()) {
    return;
  }

  if (!isBaseCached_) {
    baseMainTransform_ = target->mainPosition.transform;
    baseMeshTransform_ = target->objectParts_[0].transform;
    isBaseCached_ = true;
  }
}

/*   改造を適用   */
void ModBody::Apply(Object *target) {
  if (target == nullptr) {
    return;
  }
  if (target->objectParts_.empty()) {
    return;
  }

  CacheBaseTransforms(target);

  /* root 側
  ------------------------------------------------
  mainPosition は接続点として使う。
  位置と回転はシーンや ImGui 側で触る。
  scale は root に持たせず、初期値へ戻す。
  ------------------------------------------------*/
  Transform root = target->mainPosition.transform;
  root.scale = baseMainTransform_.scale;
  target->mainPosition.transform = root;

  /* mesh 側
  ------------------------------------------------
  見た目の translate / scale を root 基準で再計算する。
  ------------------------------------------------*/
  Transform mesh = baseMeshTransform_;
  Vector3 newScale = MakePartScale(baseMeshTransform_.scale, param_);

  mesh.scale = newScale;
  mesh.translate =
      Add(baseMeshTransform_.translate, GetAnchorOffset(part_, newScale));

  target->objectParts_[0].transform = mesh;
}