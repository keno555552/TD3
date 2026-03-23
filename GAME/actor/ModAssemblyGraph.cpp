#define NOMINMAX
#include "ModAssemblyGraph.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace {

Vector3 Mul(const Vector3 &a, const Vector3 &b) {
  return {a.x * b.x, a.y * b.y, a.z * b.z};
}

Vector3 AddV(const Vector3 &a, const Vector3 &b) {
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}

float DistSq(const Vector3 &a, const Vector3 &b) {
  const float dx = a.x - b.x;
  const float dy = a.y - b.y;
  const float dz = a.z - b.z;
  return dx * dx + dy * dy + dz * dz;
}

float ClampScale(float v) {
  if (v < 0.2f) {
    return 0.2f;
  }
  if (v > 5.0f) {
    return 5.0f;
  }
  return v;
}

} // namespace

Vector3 ModAssemblyGraph::MakeDefaultLocalTranslate(ModBodyPart part,
                                                    PartSide side) const {
  // 部位ごとの基本配置を返す
  switch (part) {
  case ModBodyPart::Body:
    return {0.0f, 0.0f, 0.0f};

  case ModBodyPart::Neck:
    return {0.0f, 0.0f, 0.0f};
  case ModBodyPart::Head:
    return {0.0f, 0.0f, 0.0f};

  case ModBodyPart::LeftUpperArm:
    return {0.0f, 0.0f, 0.0f};
  case ModBodyPart::RightUpperArm:
    return {0.0f, 0.0f, 0.0f};
  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
    return {0.0f, -0.25f, 0.0f};

  case ModBodyPart::LeftThigh:
    return {0.0f, 0.0f, 0.0f};
  case ModBodyPart::RightThigh:
    return {0.0f, 0.0f, 0.0f};
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
    return {0.0f, 0.25f, 0.0f};

  default:
    break;
  }

  // 例外に入らなかった部位は左右属性から仮の初期位置を返す
  if (side == PartSide::Left) {
    return {-0.35f, 0.0f, 0.0f};
  }
  if (side == PartSide::Right) {
    return {0.35f, 0.0f, 0.0f};
  }
  return {0.0f, 0.0f, 0.0f};
}

ConnectorRole ModAssemblyGraph::DesiredParentConnectorRoleForChild(
    ModBodyPart childPart) const {
  // 子部位の種類から、親に要求する接続点役割を返す
  switch (childPart) {
  case ModBodyPart::Neck:
    return ConnectorRole::Neck;

  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::RightUpperArm:
    return ConnectorRole::Shoulder;

  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
    return ConnectorRole::ArmJoint;

  case ModBodyPart::LeftThigh:
  case ModBodyPart::RightThigh:
    return ConnectorRole::Hip;

  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
    return ConnectorRole::LegJoint;

  case ModBodyPart::Head:
    return ConnectorRole::Neck;

  case ModBodyPart::Body:
  case ModBodyPart::Count:
  default:
    return ConnectorRole::Generic;
  }
}

int ModAssemblyGraph::ScoreConnectorMatch(const ConnectorNode &connector,
                                          ConnectorRole desiredRole,
                                          PartSide childSide) const {
  // 左右属性が合わない接続点は候補から除外する
  if (!IsSideCompatible(connector.side, childSide)) {
    return -1000000;
  }

  int score = 0;

  // 接続点の役割が一致していれば高得点にする
  // 子が汎用接続を求める場合は少しだけ加点する
  if (connector.role == desiredRole) {
    score += 100;
  } else if (desiredRole == ConnectorRole::Generic) {
    score += 10;
  }

  // 左右属性も加味して、より自然な接続点を優先する
  if (childSide != PartSide::Center) {
    if (connector.side == childSide) {
      score += 20;
    } else if (connector.side == PartSide::Center) {
      score += 5;
    }
  } else {
    if (connector.side == PartSide::Center) {
      score += 20;
    }
  }

  return score;
}

void ModAssemblyGraph::InitializeDefaultHumanoid() {
  // 既存構造を初期化する
  nodes_.clear();
  nextPartId_ = 1;
  nextConnectorId_ = 1;
  bodyId_ = -1;
  headId_ = -1;

  // 初期人型の中心となる Body -> Neck -> Head を作る
  bodyId_ = AddPart(ModBodyPart::Body, PartSide::Center, -1, false);
  const int neckId =
      AddPart(ModBodyPart::Neck, PartSide::Center, bodyId_, false);

  // Head は 1 個は残したいが、追加削除はできるようにするので required
  // にはしない
  headId_ = AddPart(ModBodyPart::Head, PartSide::Center, neckId, false);

  // 左腕セットを作る
  const int lUpper =
      AddPart(ModBodyPart::LeftUpperArm, PartSide::Left, bodyId_, false);
  AddPart(ModBodyPart::LeftForeArm, PartSide::Left, lUpper, false);

  // 右腕セットを作る
  const int rUpper =
      AddPart(ModBodyPart::RightUpperArm, PartSide::Right, bodyId_, false);
  AddPart(ModBodyPart::RightForeArm, PartSide::Right, rUpper, false);

  // 左脚セットを作る
  const int lThigh =
      AddPart(ModBodyPart::LeftThigh, PartSide::Left, bodyId_, false);
  AddPart(ModBodyPart::LeftShin, PartSide::Left, lThigh, false);

  // 右脚セットを作る
  const int rThigh =
      AddPart(ModBodyPart::RightThigh, PartSide::Right, bodyId_, false);
  AddPart(ModBodyPart::RightShin, PartSide::Right, rThigh, false);
}

std::vector<int> ModAssemblyGraph::GetNodeIdsSorted() const {
  // すべての部位IDを収集する
  std::vector<int> ids;
  ids.reserve(nodes_.size());

  for (std::unordered_map<int, PartNode>::const_iterator it = nodes_.begin();
       it != nodes_.end(); ++it) {
    ids.push_back(it->first);
  }

  // 昇順で並べて返す
  std::sort(ids.begin(), ids.end());
  return ids;
}

const PartNode *ModAssemblyGraph::FindNode(int partId) const {
  // 指定IDのノードを検索する
  std::unordered_map<int, PartNode>::const_iterator it = nodes_.find(partId);
  if (it == nodes_.end()) {
    return nullptr;
  }

  // 見つかったノードを返す
  return &it->second;
}

const ConnectorNode *ModAssemblyGraph::FindConnector(int partId,
                                                     int connectorId) const {
  // 指定部位を探す
  std::unordered_map<int, PartNode>::const_iterator it = nodes_.find(partId);
  if (it == nodes_.end()) {
    return nullptr;
  }

  // 対象部位の接続点一覧から一致するIDを探す
  const PartNode &node = it->second;
  for (size_t i = 0; i < node.connectors.size(); ++i) {
    if (node.connectors[i].id == connectorId) {
      return &node.connectors[i];
    }
  }

  // 見つからなければ nullptr を返す
  return nullptr;
}

int ModAssemblyGraph::AddPart(ModBodyPart part, PartSide side, int parentId,
                              bool required) {
  // 新しい部位ノードを作成する
  PartNode node;
  node.id = nextPartId_++;
  node.part = part;
  node.side = side;
  node.required = required;

  // 初期 transform を作り、部位ごとの初期位置を設定する
  node.localTransform = CreateDefaultTransform();
  node.localTransform.translate = MakeDefaultLocalTranslate(part, side);

  // 部位種類に応じた接続点を作成する
  node.connectors = MakeDefaultConnectors(part, side);

  // 親が存在する場合は、親子接続情報を設定する
  if (parentId >= 0 && nodes_.count(parentId) > 0) {
    node.parentId = parentId;
    node.parentConnectorId = FindConnectorIdForChild(parentId, part, side);
    node.selfConnectorId = node.connectors.empty() ? -1 : node.connectors[0].id;
  }

  // ノード一覧へ追加して ID を返す
  nodes_[node.id] = node;
  return node.id;
}

std::vector<ConnectorNode>
ModAssemblyGraph::MakeDefaultConnectors(ModBodyPart part, PartSide side) {
  // 返却用の接続点配列を用意する
  std::vector<ConnectorNode> result;
  ConnectorNode c;

  // Body は首、左右肩、左右腰の専用接続点を持つ
  if (part == ModBodyPart::Body) {
    c = {nextConnectorId_++,
         ConnectorRole::Neck,
         PartSide::Center,
         {0.0f, 0.75f, 0.0f}};
    result.push_back(c);

    c = {nextConnectorId_++,
         ConnectorRole::Shoulder,
         PartSide::Left,
         {-0.85f, 0.75f, 0.0f}};
    result.push_back(c);

    c = {nextConnectorId_++,
         ConnectorRole::Shoulder,
         PartSide::Right,
         {0.85f, 0.75f, 0.0f}};
    result.push_back(c);

    c = {nextConnectorId_++,
         ConnectorRole::Hip,
         PartSide::Left,
         {-0.35f, -0.85f, 0.0f}};
    result.push_back(c);

    c = {nextConnectorId_++,
         ConnectorRole::Hip,
         PartSide::Right,
         {0.35f, -0.85f, 0.0f}};
    result.push_back(c);

    return result;
  }

  // それ以外の部位は、まず根元用の汎用接続点を1つ持たせる
  c = {nextConnectorId_++, ConnectorRole::Generic, side, {0.0f, 0.0f, 0.0f}};
  result.push_back(c);

  // 部位種類によって、先端や次段の接続用コネクタを追加する
  if (part == ModBodyPart::Neck) {
    c = {nextConnectorId_++,
         ConnectorRole::Neck,
         PartSide::Center,
         {0.0f, 0.65f, 0.0f}};
    result.push_back(c);
  } else if (part == ModBodyPart::LeftUpperArm ||
             part == ModBodyPart::RightUpperArm) {
    c = {nextConnectorId_++,
         ConnectorRole::ArmJoint,
         side,
         {0.0f, -0.75f, 0.0f}};
    result.push_back(c);
  } else if (part == ModBodyPart::LeftThigh ||
             part == ModBodyPart::RightThigh) {
    c = {nextConnectorId_++,
         ConnectorRole::LegJoint,
         side,
         {0.0f, -0.75f, 0.0f}};
    result.push_back(c);
  }

  return result;
}

bool ModAssemblyGraph::CanParentChild(ModBodyPart parent,
                                      ModBodyPart child) const {
  // Body には首、腕根元、脚根元をつなげる
  if (parent == ModBodyPart::Body) {
    return child == ModBodyPart::Neck || child == ModBodyPart::LeftUpperArm ||
           child == ModBodyPart::RightUpperArm ||
           child == ModBodyPart::LeftThigh || child == ModBodyPart::RightThigh;
  }

  // Neck には Head をつなげる
  if (parent == ModBodyPart::Neck) {
    return child == ModBodyPart::Head;
  }

  // 上腕には前腕をつなげる
  if (parent == ModBodyPart::LeftUpperArm ||
      parent == ModBodyPart::RightUpperArm) {
    return child == ModBodyPart::LeftForeArm ||
           child == ModBodyPart::RightForeArm;
  }

  // 腿には脛をつなげる
  if (parent == ModBodyPart::LeftThigh || parent == ModBodyPart::RightThigh) {
    return child == ModBodyPart::LeftShin || child == ModBodyPart::RightShin;
  }

  // Head は退避先兼、Body 無し時の親として使う
  if (parent == ModBodyPart::Head) {
    return true;
  }

  return false;
}

bool ModAssemblyGraph::IsSideCompatible(PartSide parentSide,
                                        PartSide childSide) const {
  // 子が中央部位なら左右制約なしで接続可能
  if (childSide == PartSide::Center) {
    return true;
  }

  // 左右が一致しているか、親が中央なら接続可能
  return parentSide == childSide || parentSide == PartSide::Center;
}

int ModAssemblyGraph::FindConnectorIdForChild(int parentId,
                                              ModBodyPart childPart,
                                              PartSide childSide) const {
  // 親ノードを検索する
  std::unordered_map<int, PartNode>::const_iterator it = nodes_.find(parentId);
  if (it == nodes_.end()) {
    return -1;
  }

  const PartNode &parent = it->second;

  // 接続点が無ければ接続できない
  if (parent.connectors.empty()) {
    return -1;
  }

  // 子部位が求める接続点役割を取得する
  const ConnectorRole desiredRole =
      DesiredParentConnectorRoleForChild(childPart);

  int bestConnectorId = -1;
  int bestScore = -1000000;

  // 親の全接続点を評価して最適なものを選ぶ
  for (size_t i = 0; i < parent.connectors.size(); ++i) {
    const ConnectorNode &connector = parent.connectors[i];
    const int score = ScoreConnectorMatch(connector, desiredRole, childSide);
    if (score > bestScore) {
      bestScore = score;
      bestConnectorId = connector.id;
    }
  }

  // 最適候補が見つかればそれを返す
  if (bestConnectorId >= 0) {
    return bestConnectorId;
  }

  // 念のため先頭接続点を返す
  return parent.connectors[0].id;
}

std::vector<int> ModAssemblyGraph::GetChildren(int parentId) const {
  // 指定親IDを持つノードを集める
  std::vector<int> ids;
  for (std::unordered_map<int, PartNode>::const_iterator it = nodes_.begin();
       it != nodes_.end(); ++it) {
    if (it->second.parentId == parentId) {
      ids.push_back(it->first);
    }
  }

  return ids;
}

bool ModAssemblyGraph::IsLegRoot(ModBodyPart part) const {
  // 脚根元として扱うのは腿
  return part == ModBodyPart::LeftThigh || part == ModBodyPart::RightThigh;
}

bool ModAssemblyGraph::IsArmRoot(ModBodyPart part) const {
  // 腕根元として扱うのは上腕
  return part == ModBodyPart::LeftUpperArm ||
         part == ModBodyPart::RightUpperArm;
}

bool ModAssemblyGraph::IsLimbRoot(ModBodyPart part) const {
  // 腕根元または脚根元なら四肢根元とみなす
  return IsArmRoot(part) || IsLegRoot(part);
}

bool ModAssemblyGraph::RemoveSubtree(int partId) {
  // まず子を先に再帰削除する
  const std::vector<int> children = GetChildren(partId);

  for (size_t i = 0; i < children.size(); ++i) {
    if (!RemoveSubtree(children[i])) {
      return false;
    }
  }

  // 自分自身を検索する
  std::unordered_map<int, PartNode>::iterator it = nodes_.find(partId);
  if (it == nodes_.end()) {
    return false;
  }

  // 必須部位は削除できない
  if (it->second.required) {
    return false;
  }

  // ノードを削除する
  nodes_.erase(it);

  // Body / Head の管理IDを再探索する
  RefreshManagedPartIds();
  return true;
}

bool ModAssemblyGraph::RemoveBodyAttachedLimbsAndReattachOthers(
    int bodyPartId) {
  // Body 直下の子を取得する
  const std::vector<int> directChildren = GetChildren(bodyPartId);

  for (size_t i = 0; i < directChildren.size(); ++i) {
    const int childId = directChildren[i];

    // 子ノードを検索する
    std::unordered_map<int, PartNode>::iterator it = nodes_.find(childId);
    if (it == nodes_.end()) {
      continue;
    }

    // Body 直下の腕根元・脚根元はセットごと削除する
    if (IsLimbRoot(it->second.part)) {
      if (!RemoveSubtree(childId)) {
        return false;
      }
    }
  }

  // 四肢セットを消したあと、残った子は通常の再接続処理に流す
  return ReattachChildrenOf(bodyPartId);
}

bool ModAssemblyGraph::IsHead(ModBodyPart part) const {
  // Head 判定を返す
  return part == ModBodyPart::Head;
}

int ModAssemblyGraph::CountLegRoots() const {
  // 現在存在する脚根元数を数える
  int count = 0;
  for (std::unordered_map<int, PartNode>::const_iterator it = nodes_.begin();
       it != nodes_.end(); ++it) {
    if (IsLegRoot(it->second.part)) {
      ++count;
    }
  }
  return count;
}

int ModAssemblyGraph::CountHeads() const {
  int count = 0;
  for (std::unordered_map<int, PartNode>::const_iterator it = nodes_.begin();
       it != nodes_.end(); ++it) {
    if (it->second.part == ModBodyPart::Head) {
      ++count;
    }
  }
  return count;
}

bool ModAssemblyGraph::ShouldCascadeDeleteChildren(ModBodyPart part) const {
  // 上腕と腿は子とセットで削除する
  return part == ModBodyPart::LeftUpperArm ||
         part == ModBodyPart::RightUpperArm || part == ModBodyPart::LeftThigh ||
         part == ModBodyPart::RightThigh;
}

void ModAssemblyGraph::RefreshManagedPartIds() {
  bodyId_ = -1;
  headId_ = -1;

  for (std::unordered_map<int, PartNode>::const_iterator it = nodes_.begin();
       it != nodes_.end(); ++it) {
    if (bodyId_ < 0 && it->second.part == ModBodyPart::Body) {
      bodyId_ = it->first;
    }
    if (headId_ < 0 && it->second.part == ModBodyPart::Head) {
      headId_ = it->first;
    }

    if (bodyId_ >= 0 && headId_ >= 0) {
      break;
    }
  }
}

bool ModAssemblyGraph::RemoveChildrenRecursive(int partId) {
  // まず直接の子を取得する
  const std::vector<int> children = GetChildren(partId);

  for (size_t i = 0; i < children.size(); ++i) {
    const int childId = children[i];

    // 子孫を先に削除する
    if (!RemoveChildrenRecursive(childId)) {
      return false;
    }

    // 子ノード自体を削除する
    std::unordered_map<int, PartNode>::iterator childIt = nodes_.find(childId);
    if (childIt != nodes_.end()) {
      if (childIt->second.required) {
        return false;
      }
      nodes_.erase(childIt);
    }
  }

  RefreshManagedPartIds();
  return true;
}

bool ModAssemblyGraph::RemovePart(int partId) {
  // 削除対象ノードを検索する
  std::unordered_map<int, PartNode>::iterator it = nodes_.find(partId);
  if (it == nodes_.end()) {
    return false;
  }

  const PartNode target = it->second;

  // 必須部位は削除不可
  if (target.required) {
    return false;
  }

  // Head は 0 個にはできない
  if (IsHead(target.part) && CountHeads() <= 1) {
    return false;
  }

  // 脚根元は最低2本を下回る場合は削除不可
  if (IsLegRoot(target.part) && CountLegRoots() <= 2) {
    return false;
  }

  // Body は特別扱い
  // 直下の腕脚セットは削除し、それ以外の子は付け替える
  if (target.part == ModBodyPart::Body) {
    if (!RemoveBodyAttachedLimbsAndReattachOthers(partId)) {
      return false;
    }

    nodes_.erase(partId);
    RefreshManagedPartIds();
    return true;
  }

  // 腕根元・脚根元は子もまとめて削除する
  if (ShouldCascadeDeleteChildren(target.part)) {
    if (!RemoveChildrenRecursive(partId)) {
      return false;
    }

    nodes_.erase(partId);
    RefreshManagedPartIds();
    return true;
  }

  // それ以外の部位は子を別の親に付け替えてから削除する
  if (!ReattachChildrenOf(partId)) {
    return false;
  }

  nodes_.erase(partId);
  RefreshManagedPartIds();
  return true;
}

bool ModAssemblyGraph::ReattachChildrenOf(int removedPartId) {
  // 削除対象の直接の子を取得する
  std::vector<int> children = GetChildren(removedPartId);

  for (size_t i = 0; i < children.size(); ++i) {
    const int childId = children[i];

    // 子にとって適した新しい親を探す
    int newParent = FindBestParentForChild(childId);

    // 通常候補が見つからない場合は、Head を退避先として使う
    if (newParent < 0 && headId_ >= 0 && nodes_.count(headId_) > 0 &&
        childId != headId_) {
      newParent = headId_;
    }

    // 親が見つからない場合は再接続失敗
    if (newParent < 0) {
      return false;
    }

    // 新しい親情報と接続コネクタを更新する
    nodes_[childId].parentId = newParent;
    nodes_[childId].parentConnectorId = FindConnectorIdForChild(
        newParent, nodes_[childId].part, nodes_[childId].side);
  }

  return true;
}

int ModAssemblyGraph::FindBestParentForChild(int childId) const {
  // 子ノードを検索する
  std::unordered_map<int, PartNode>::const_iterator childIt =
      nodes_.find(childId);
  if (childIt == nodes_.end()) {
    return -1;
  }

  const PartNode &child = childIt->second;
  const Vector3 childPos = ComputeWorldPosition(childId);

  float bestDist = std::numeric_limits<float>::max();
  int bestId = -1;

  // すべてのノードを候補として走査する
  for (std::unordered_map<int, PartNode>::const_iterator it = nodes_.begin();
       it != nodes_.end(); ++it) {
    const PartNode &candidate = it->second;

    // 自分自身は候補にしない
    if (candidate.id == child.id) {
      continue;
    }

    // 親子関係として不正なら除外する
    if (!CanParentChild(candidate.part, child.part)) {
      continue;
    }

    // 左右属性が合わない候補は除外する
    if (!IsSideCompatible(candidate.side, child.side)) {
      continue;
    }

    // 現在位置に最も近い候補を採用する
    const Vector3 p = ComputeWorldPosition(candidate.id);
    const float d = DistSq(p, childPos);
    if (d < bestDist) {
      bestDist = d;
      bestId = candidate.id;
    }
  }

  return bestId;
}

bool ModAssemblyGraph::MovePart(int partId, int newParentId,
                                int newParentConnectorId) {
  // 対象部位と新しい親が存在するか確認する
  if (nodes_.count(partId) == 0 || nodes_.count(newParentId) == 0) {
    return false;
  }

  PartNode &node = nodes_[partId];
  const PartNode &parent = nodes_.at(newParentId);

  // 親子として接続可能か確認する
  if (!CanParentChild(parent.part, node.part)) {
    return false;
  }
  if (!IsSideCompatible(parent.side, node.side)) {
    return false;
  }

  // 親情報を更新する
  node.parentId = newParentId;

  // 親側コネクタは指定があればそれを使い、無ければ自動選択する
  node.parentConnectorId =
      newParentConnectorId >= 0
          ? newParentConnectorId
          : FindConnectorIdForChild(newParentId, node.part, node.side);

  return true;
}

bool ModAssemblyGraph::SetPartLocalTranslate(int partId,
                                             const Vector3 &localTranslate) {
  // 対象ノードが存在しなければ失敗
  if (nodes_.count(partId) == 0) {
    return false;
  }

  // ローカル位置を更新する
  nodes_[partId].localTransform.translate = localTranslate;
  return true;
}

bool ModAssemblyGraph::SetPartScale(int partId, const Vector3 &localScale) {
  // 対象ノードが存在しなければ失敗
  if (nodes_.count(partId) == 0) {
    return false;
  }

  PartNode &node = nodes_[partId];

  // スケールは極端な値を防ぐためクランプして設定する
  node.localTransform.scale = {
      ClampScale(localScale.x),
      ClampScale(localScale.y),
      ClampScale(localScale.z),
  };
  return true;
}

Vector3 ModAssemblyGraph::ComputeWorldPosition(int partId) const {
  // 対象ノードを検索する
  std::unordered_map<int, PartNode>::const_iterator it = nodes_.find(partId);
  if (it == nodes_.end()) {
    return {0.0f, 0.0f, 0.0f};
  }

  const PartNode &node = it->second;

  // 親が無い場合はローカル位置がそのままワールド位置になる
  if (node.parentId < 0 || nodes_.count(node.parentId) == 0) {
    return node.localTransform.translate;
  }

  // 親のワールド位置と親スケールを使って自分のワールド位置を計算する
  const Vector3 parentPos = ComputeWorldPosition(node.parentId);
  const Vector3 parentScale = nodes_.at(node.parentId).localTransform.scale;
  return AddV(parentPos, Mul(parentScale, node.localTransform.translate));
}

bool ModAssemblyGraph::AddArmAssembly(PartSide side) {
  // 左右指定が不正なら追加しない
  if (side != PartSide::Left && side != PartSide::Right) {
    return false;
  }

  int parentId = -1;

  // Body があれば Body を親にする
  if (bodyId_ >= 0 && nodes_.count(bodyId_) > 0) {
    parentId = bodyId_;
  }
  // Body が無ければ Head を親にする
  else if (headId_ >= 0 && nodes_.count(headId_) > 0) {
    parentId = headId_;
  } else {
    return false;
  }

  // 左右に応じて上腕と前腕の種類を決める
  const ModBodyPart upper = (side == PartSide::Left)
                                ? ModBodyPart::LeftUpperArm
                                : ModBodyPart::RightUpperArm;
  const ModBodyPart fore = (side == PartSide::Left) ? ModBodyPart::LeftForeArm
                                                    : ModBodyPart::RightForeArm;

  // 上腕を追加し、その子として前腕を追加する
  const int upperId = AddPart(upper, side, parentId, false);
  AddPart(fore, side, upperId, false);
  return true;
}

bool ModAssemblyGraph::AddLegAssembly(PartSide side) {
  // 左右指定が不正なら追加しない
  if (side != PartSide::Left && side != PartSide::Right) {
    return false;
  }

  int parentId = -1;

  // Body があれば Body を親にする
  if (bodyId_ >= 0 && nodes_.count(bodyId_) > 0) {
    parentId = bodyId_;
  }
  // Body が無ければ Head を親にする
  else if (headId_ >= 0 && nodes_.count(headId_) > 0) {
    parentId = headId_;
  } else {
    return false;
  }

  // 左右に応じて腿と脛の種類を決める
  const ModBodyPart thigh = (side == PartSide::Left) ? ModBodyPart::LeftThigh
                                                     : ModBodyPart::RightThigh;
  const ModBodyPart shin =
      (side == PartSide::Left) ? ModBodyPart::LeftShin : ModBodyPart::RightShin;

  // 腿を追加し、その子として脛を追加する
  const int thighId = AddPart(thigh, side, parentId, false);
  AddPart(shin, side, thighId, false);
  return true;
}

bool ModAssemblyGraph::AddNeckPart() {
  int parentId = -1;

  // Body があれば Body の下に追加する
  if (bodyId_ >= 0 && nodes_.count(bodyId_) > 0) {
    parentId = bodyId_;
  }
  // Body が無ければ Head の下に追加する
  else if (headId_ >= 0 && nodes_.count(headId_) > 0) {
    parentId = headId_;
  } else {
    return false;
  }

  // Neck 部位を追加する
  AddPart(ModBodyPart::Neck, PartSide::Center, parentId, false);
  return true;
}

bool ModAssemblyGraph::AddHeadPart() {
  int parentId = -1;

  // まず Neck を優先して探す
  for (std::unordered_map<int, PartNode>::const_iterator it = nodes_.begin();
       it != nodes_.end(); ++it) {
    if (it->second.part == ModBodyPart::Neck) {
      parentId = it->first;
      break;
    }
  }

  // Neck が無ければ既存 Head の下へ追加する
  if (parentId < 0 && headId_ >= 0 && nodes_.count(headId_) > 0) {
    parentId = headId_;
  }

  if (parentId < 0) {
    return false;
  }

  const int newHeadId =
      AddPart(ModBodyPart::Head, PartSide::Center, parentId, false);

  if (headId_ < 0 || nodes_.count(headId_) == 0) {
    headId_ = newHeadId;
  }

  return true;
}

bool ModAssemblyGraph::AddBodyPart() {
  // Body が既に存在する場合は追加しない
  if (bodyId_ >= 0 && nodes_.count(bodyId_) > 0) {
    return false;
  }

  int parentId = -1;

  // Body が無い場合は Head の下へ追加する
  if (headId_ >= 0 && nodes_.count(headId_) > 0) {
    parentId = headId_;
  } else {
    return false;
  }

  // 新しい Body を追加して管理IDを更新する
  const int newBodyId =
      AddPart(ModBodyPart::Body, PartSide::Center, parentId, false);

  bodyId_ = newBodyId;

  // Body を再生成したら、Head 直下にいる Body 配下向け部位を Body へ戻す
  const std::vector<int> children = GetChildren(parentId);
  for (size_t i = 0; i < children.size(); ++i) {
    const int childId = children[i];
    if (childId == newBodyId) {
      continue;
    }

    std::unordered_map<int, PartNode>::iterator it = nodes_.find(childId);
    if (it == nodes_.end()) {
      continue;
    }

    if (!CanParentChild(ModBodyPart::Body, it->second.part)) {
      continue;
    }

    it->second.parentId = newBodyId;
    it->second.parentConnectorId =
        FindConnectorIdForChild(newBodyId, it->second.part, it->second.side);

    if (!it->second.connectors.empty()) {
      it->second.selfConnectorId = it->second.connectors[0].id;
    }

    // Body 配下に戻したときは見た目が崩れにくいように初期ローカル位置へ戻す
    it->second.localTransform.translate =
        MakeDefaultLocalTranslate(it->second.part, it->second.side);
  }

  return true;
}