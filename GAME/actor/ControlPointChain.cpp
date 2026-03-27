#include "ControlPointChain.h"
#include "Vector3.h"
#include <cmath>

namespace {

Vector3 NormalizeSafe(const Vector3 &v, const Vector3 &fallback) {
  const float len = Length(v);
  if (len < 0.0001f) {
    return fallback;
  }

  const float inv = 1.0f / len;
  return {v.x * inv, v.y * inv, v.z * inv};
}

float ClampFloat(float value, float minValue, float maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

} // namespace

void ControlPointChain::Clear() {
  // チェーンを空にする
  nodes_.clear();
}

void ControlPointChain::BuildArmChain() {
  // 既存ノードを破棄して腕チェーンを作り直す
  nodes_.clear();

  // Root（肩）を作る
  ControlPointNode shoulder{};
  shoulder.role = ModControlPointRole::Root;
  shoulder.parentIndex = -1;
  shoulder.localPosition = {0.0f, 0.0f, 0.0f};
  shoulder.radius = 0.09f;
  shoulder.movable = false;
  shoulder.isConnectionPoint = true;
  shoulder.acceptsParent = true;
  shoulder.acceptsChild = false;
  nodes_.push_back(shoulder);

  // Bend（肘）を作る（Rootの子）
  ControlPointNode elbow{};
  elbow.role = ModControlPointRole::Bend;
  elbow.parentIndex = 0;
  elbow.localPosition = {0.0f, -0.55f, 0.0f};
  elbow.radius = 0.08f;
  elbow.movable = true;
  nodes_.push_back(elbow);

  // End（手首）を作る（Bendの子）
  ControlPointNode wrist{};
  wrist.role = ModControlPointRole::End;
  wrist.parentIndex = 1;
  wrist.localPosition = {0.0f, -0.55f, 0.0f};
  wrist.radius = 0.08f;
  wrist.movable = true;
  wrist.isConnectionPoint = true;
  wrist.acceptsChild = true;
  nodes_.push_back(wrist);

  // 階層更新フック（現状は空だが、将来キャッシュ化する場合に備える）
  UpdateHierarchy();
}

void ControlPointChain::BuildLegChain() {
  // 既存ノードを破棄して脚チェーンを作り直す
  nodes_.clear();

  // Root（股関節）を作る
  ControlPointNode hip{};
  hip.role = ModControlPointRole::Root;
  hip.parentIndex = -1;
  hip.localPosition = {0.0f, 0.0f, 0.0f};
  hip.radius = 0.10f;
  hip.movable = false;
  hip.isConnectionPoint = true;
  hip.acceptsParent = true;
  nodes_.push_back(hip);

  // Bend（膝）を作る（Rootの子）
  ControlPointNode knee{};
  knee.role = ModControlPointRole::Bend;
  knee.parentIndex = 0;
  knee.localPosition = {0.0f, -0.70f, 0.0f};
  knee.radius = 0.09f;
  knee.movable = true;
  nodes_.push_back(knee);

  // End（足首）を作る（Bendの子）
  ControlPointNode ankle{};
  ankle.role = ModControlPointRole::End;
  ankle.parentIndex = 1;
  ankle.localPosition = {0.0f, -0.70f, 0.0f};
  ankle.radius = 0.09f;
  ankle.movable = true;
  ankle.isConnectionPoint = true;
  ankle.acceptsChild = true;
  nodes_.push_back(ankle);

  // 階層更新フック
  UpdateHierarchy();
}

void ControlPointChain::BuildTorsoChain() {
  // 既存ノードを破棄して胴体チェーンを作り直す
  nodes_.clear();

  // Chest（胸）を作る（ルート）
  ControlPointNode chest{};
  chest.role = ModControlPointRole::Chest;
  chest.parentIndex = -1;
  chest.localPosition = {0.0f, 0.45f, 0.0f};
  chest.radius = 0.12f;
  chest.movable = true;
  chest.isConnectionPoint = true;
  chest.acceptsChild = true;
  nodes_.push_back(chest);

  // Belly（腹）を作る（Chestの子）
  ControlPointNode belly{};
  belly.role = ModControlPointRole::Belly;
  belly.parentIndex = 0;
  belly.localPosition = {0.0f, -0.45f, 0.0f};
  belly.radius = 0.10f;
  belly.movable = true;
  nodes_.push_back(belly);

  // Waist（腰）を作る（Bellyの子）
  ControlPointNode waist{};
  waist.role = ModControlPointRole::Waist;
  waist.parentIndex = 1;
  waist.localPosition = {0.0f, -0.45f, 0.0f};
  waist.radius = 0.12f;
  waist.movable = true;
  waist.isConnectionPoint = true;
  waist.acceptsChild = true;
  nodes_.push_back(waist);

  // 階層更新フック
  UpdateHierarchy();
}

void ControlPointChain::BuildHeadChain() {
  // 既存ノードを破棄して頭チェーンを作り直す
  nodes_.clear();

  // LowerNeck（首下）を作る（ルート）
  ControlPointNode lower{};
  lower.role = ModControlPointRole::LowerNeck;
  lower.parentIndex = -1;
  lower.localPosition = {0.0f, 0.0f, 0.0f};
  lower.radius = 0.09f;
  lower.movable = false;
  lower.isConnectionPoint = true;
  lower.acceptsParent = true;
  lower.acceptsChild = false;
  nodes_.push_back(lower);

  // UpperNeck（首上）を作る（LowerNeckの子）
  ControlPointNode upper{};
  upper.role = ModControlPointRole::UpperNeck;
  upper.parentIndex = 0;
  upper.localPosition = {0.0f, 0.28f, 0.0f};
  upper.radius = 0.08f;
  upper.movable = true;
  upper.isConnectionPoint = true;
  upper.acceptsParent = false;
  upper.acceptsChild = true;
  nodes_.push_back(upper);

  // HeadCenter（頭中心）を作る（UpperNeckの子）
  ControlPointNode center{};
  center.role = ModControlPointRole::HeadCenter;
  center.parentIndex = 1;
  center.localPosition = {0.0f, 0.52f, 0.0f};
  center.radius = 0.11f;
  center.movable = true;
  center.isConnectionPoint = true;
  center.acceptsParent = false;
  center.acceptsChild = true;
  nodes_.push_back(center);

  // 階層更新フック
  UpdateHierarchy();
}

int ControlPointChain::FindIndex(ModControlPointRole role) const {
  // 役割一致するノードを線形探索する
  for (size_t i = 0; i < nodes_.size(); ++i) {
    if (nodes_[i].role == role) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

bool ControlPointChain::MovePoint(size_t index,
                                  const Vector3 &newLocalPosition) {
  // 範囲外は失敗
  if (index >= nodes_.size()) {
    return false;
  }

  // 固定ノードは移動不可
  if (!nodes_[index].movable) {
    return false;
  }

  // ワールド指定で渡された座標を、親基準ローカルへ変換する（親がいる場合）
  auto ToLocalFromWorld = [this](size_t targetIndex,
                                 const Vector3 &worldPosition) -> Vector3 {
    const int parentIndex = nodes_[targetIndex].parentIndex;
    if (parentIndex >= 0) {
      const Vector3 parentWorld =
          GetWorldPosition(static_cast<size_t>(parentIndex));
      return Subtract(worldPosition, parentWorld);
    }
    return worldPosition;
  };

  // 移動対象の役割を取得する
  const ModControlPointRole role = nodes_[index].role;

  // 代表的なチェーン構造（腕/脚）
  const int rootIndex = FindIndex(ModControlPointRole::Root);
  const int bendIndex = FindIndex(ModControlPointRole::Bend);
  const int endIndex = FindIndex(ModControlPointRole::End);

  // 代表的なチェーン構造（頭）
  const int lowerIndex = FindIndex(ModControlPointRole::LowerNeck);
  const int upperIndex = FindIndex(ModControlPointRole::UpperNeck);
  const int headIndex = FindIndex(ModControlPointRole::HeadCenter);

  // Bend移動（Root基準の半径制限 + End追従）
  if (role == ModControlPointRole::Bend && rootIndex >= 0 && endIndex >= 0) {
    // 現在のRoot/Bend/Endのワールド位置を取得する
    const Vector3 rootPos = GetWorldPosition(static_cast<size_t>(rootIndex));
    const Vector3 oldBendPos = GetWorldPosition(index);
    const Vector3 oldEndPos = GetWorldPosition(static_cast<size_t>(endIndex));

    // Bend->End の相対関係（オフセット）を保持して追従させる
    const Vector3 bendToEnd = Subtract(oldEndPos, oldBendPos);

    // Rootから見た候補方向を作り、半径（距離）を制限する
    Vector3 rootToCandidate = Subtract(newLocalPosition, rootPos);
    Vector3 direction = NormalizeSafe(rootToCandidate, {0.0f, -1.0f, 0.0f});

    float radius = Length(rootToCandidate);
    radius = ClampFloat(radius, 0.30f, 1.80f);

    // 制限後のBend位置（ワールド）を確定する
    const Vector3 clampedBend = Add(rootPos, Multiply(radius, direction));

    // Bendをローカルへ戻して更新する
    nodes_[index].localPosition = ToLocalFromWorld(index, clampedBend);

    // EndはBendからのオフセットを維持して追従させる
    const Vector3 newEndWorld = Add(clampedBend, bendToEnd);
    nodes_[static_cast<size_t>(endIndex)].localPosition =
        ToLocalFromWorld(static_cast<size_t>(endIndex), newEndWorld);

    // 階層更新フック
    UpdateHierarchy();
    return true;
  }

  // End移動（Bend基準の長さ制限）
  if (role == ModControlPointRole::End && bendIndex >= 0) {
    // 現在のBend位置を基準に候補方向と長さを決める
    const Vector3 bendPos = GetWorldPosition(static_cast<size_t>(bendIndex));
    Vector3 bendToCandidate = Subtract(newLocalPosition, bendPos);

    // 長さを制限し、方向を保持してEndを更新する
    Vector3 direction = NormalizeSafe(bendToCandidate, {0.0f, -1.0f, 0.0f});
    float length = Length(bendToCandidate);
    length = ClampFloat(length, 0.25f, 1.80f);

    const Vector3 clampedEnd = Add(bendPos, Multiply(length, direction));
    nodes_[index].localPosition = ToLocalFromWorld(index, clampedEnd);

    UpdateHierarchy();
    return true;
  }

  // UpperNeck移動（LowerNeck基準の半径制限 + HeadCenter追従）
  if (role == ModControlPointRole::UpperNeck && lowerIndex >= 0 &&
      headIndex >= 0) {
    // 現在のLower/Upper/Headのワールド位置を取得する
    const Vector3 lowerPos = GetWorldPosition(static_cast<size_t>(lowerIndex));
    const Vector3 oldUpperPos = GetWorldPosition(index);
    const Vector3 oldHeadPos = GetWorldPosition(static_cast<size_t>(headIndex));

    // Upper->Head のオフセットを保持して追従させる
    const Vector3 upperToHead = Subtract(oldHeadPos, oldUpperPos);

    // Lowerから見た候補方向を作り、半径を制限する
    Vector3 lowerToCandidate = Subtract(newLocalPosition, lowerPos);
    Vector3 direction = NormalizeSafe(lowerToCandidate, {0.0f, 1.0f, 0.0f});

    float radius = Length(lowerToCandidate);
    radius = ClampFloat(radius, 0.15f, 0.80f);

    const Vector3 clampedUpper = Add(lowerPos, Multiply(radius, direction));
    nodes_[index].localPosition = ToLocalFromWorld(index, clampedUpper);

    // HeadCenterをUpperNeckに追従させる
    const Vector3 newHeadWorld = Add(clampedUpper, upperToHead);
    nodes_[static_cast<size_t>(headIndex)].localPosition =
        ToLocalFromWorld(static_cast<size_t>(headIndex), newHeadWorld);

    UpdateHierarchy();
    return true;
  }

  // HeadCenter移動（UpperNeck基準の長さ制限）
  if (role == ModControlPointRole::HeadCenter && upperIndex >= 0) {
    const Vector3 upperPos = GetWorldPosition(static_cast<size_t>(upperIndex));
    Vector3 upperToCandidate = Subtract(newLocalPosition, upperPos);

    Vector3 direction = NormalizeSafe(upperToCandidate, {0.0f, 1.0f, 0.0f});
    float length = Length(upperToCandidate);
    length = ClampFloat(length, 0.20f, 1.50f);

    const Vector3 clampedHead = Add(upperPos, Multiply(length, direction));
    nodes_[index].localPosition = ToLocalFromWorld(index, clampedHead);

    UpdateHierarchy();
    return true;
  }

  // 特別扱い以外は、そのままローカルへ変換して更新する
  nodes_[index].localPosition = ToLocalFromWorld(index, newLocalPosition);

  // 階層更新フック
  UpdateHierarchy();
  return true;
}

Vector3 ControlPointChain::GetLocalPosition(size_t index) const {
  // 範囲外はゼロを返す
  if (index >= nodes_.size()) {
    return {0.0f, 0.0f, 0.0f};
  }

  // 親基準ローカル位置を返す
  return nodes_[index].localPosition;
}

Vector3 ControlPointChain::GetWorldPosition(size_t index) const {
  // 範囲外はゼロを返す
  if (index >= nodes_.size()) {
    return {0.0f, 0.0f, 0.0f};
  }

  // ルートはローカル座標がワールド座標になる
  const ControlPointNode &node = nodes_[index];
  if (node.parentIndex < 0) {
    return node.localPosition;
  }

  // 親のワールド座標 + 自分のローカル座標でワールド座標を求める
  return Add(GetWorldPosition(static_cast<size_t>(node.parentIndex)),
             node.localPosition);
}

void ControlPointChain::UpdateHierarchy() {
  // 現時点では parentIndex と localPosition を直接たどる構造なので
  // ここで特別なキャッシュ再計算は行わない
}