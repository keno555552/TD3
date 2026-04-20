#define NOMINMAX
#include "ModAssemblyGraph.h"
#include "GAME/actor/ModAssemblyUtil.h"
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

void ModAssemblyGraph::Clear() {
  nodes_.clear();
  nextPartId_ = 1;
  nextConnectorId_ = 1;
  bodyId_ = -1;
  headId_ = -1;
  neckId_ = -1;
}

void ModAssemblyGraph::AddNode(const PartNode &node) {
  nodes_[node.id] = node;

  if (node.id >= nextPartId_) {
    nextPartId_ = node.id + 1;
  }

  for (size_t i = 0; i < node.connectors.size(); ++i) {
    const int connectorId = node.connectors[i].id;
    if (connectorId >= nextConnectorId_) {
      nextConnectorId_ = connectorId + 1;
    }
  }

  RefreshManagedPartIds();
}

Vector3 ModAssemblyGraph::MakeDefaultLocalTranslate(ModBodyPart part,
                                                    PartSide side) const {
  switch (part) {
  case ModBodyPart::ChestBody:
    return {0.0f, 0.0f, 0.0f};

  case ModBodyPart::StomachBody:
    return {0.0f, 0.0f, 0.0f};

  case ModBodyPart::Neck:
    return {0.0f, 0.0f, 0.0f};

  case ModBodyPart::Head:
    return {0.0f, 0.0f, 0.0f};

  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::RightUpperArm:
  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
  case ModBodyPart::LeftThigh:
  case ModBodyPart::RightThigh:
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
    return {0.0f, 0.0f, 0.0f};

  default:
    break;
  }

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

  case ModBodyPart::Head:
    // Head は Neck があれば Neck に付き、無ければ Body にも付く
    // Body 側では Neck ロールの接続点を頭の受け口としても使う
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

  case ModBodyPart::ChestBody:
  case ModBodyPart::StomachBody:
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
  nodes_.clear();
  nextPartId_ = 1;
  nextConnectorId_ = 1;
  bodyId_ = -1;
  headId_ = -1;

  bodyId_ = AddPart(ModBodyPart::ChestBody, PartSide::Center, -1, false);
  const int stomachId =
      AddPart(ModBodyPart::StomachBody, PartSide::Center, -1, false);

  const int neckId =
      AddPart(ModBodyPart::Neck, PartSide::Center, bodyId_, false);

  headId_ = AddPart(ModBodyPart::Head, PartSide::Center, neckId, false);

  const int lUpper =
      AddPart(ModBodyPart::LeftUpperArm, PartSide::Left, bodyId_, false);
  AddPart(ModBodyPart::LeftForeArm, PartSide::Left, lUpper, false);

  const int rUpper =
      AddPart(ModBodyPart::RightUpperArm, PartSide::Right, bodyId_, false);
  AddPart(ModBodyPart::RightForeArm, PartSide::Right, rUpper, false);

  const int lThigh =
      AddPart(ModBodyPart::LeftThigh, PartSide::Left, stomachId, false);
  AddPart(ModBodyPart::LeftShin, PartSide::Left, lThigh, false);

  const int rThigh =
      AddPart(ModBodyPart::RightThigh, PartSide::Right, stomachId, false);
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
  PartNode node;
  node.id = nextPartId_++;
  node.part = part;
  node.side = side;
  node.required = required;

  node.localTransform = CreateDefaultTransform();
  node.localTransform.translate = MakeDefaultLocalTranslate(part, side);

  node.connectors = MakeDefaultConnectors(part, side);

  if (parentId >= 0 && nodes_.count(parentId) > 0) {
    node.parentId = parentId;
    node.parentConnectorId = FindConnectorIdForChild(parentId, part, side);
    node.selfConnectorId = node.connectors.empty() ? -1 : node.connectors[0].id;

    const PartNode &parent = nodes_.at(parentId);
    node.localTransform.translate =
        MakeDefaultAttachLocal(parent.part, part, side);
  }

  nodes_[node.id] = node;
  return node.id;
}

std::vector<ConnectorNode>
ModAssemblyGraph::MakeDefaultConnectors(ModBodyPart part, PartSide side) {
  std::vector<ConnectorNode> result;
  ConnectorNode c;

  if (part == ModBodyPart::ChestBody) {
    c = {nextConnectorId_++,
         ConnectorRole::Generic,
         PartSide::Center,
         {0.0f, 0.0f, 0.0f}};
    result.push_back(c);

    c = {nextConnectorId_++,
         ConnectorRole::Neck,
         PartSide::Center,
         {0.0f, 0.45f, 0.0f}};
    result.push_back(c);

    c = {nextConnectorId_++,
         ConnectorRole::Shoulder,
         PartSide::Left,
         {-0.75f, 0.30f, 0.0f}};
    result.push_back(c);

    c = {nextConnectorId_++,
         ConnectorRole::Shoulder,
         PartSide::Right,
         {0.75f, 0.30f, 0.0f}};
    result.push_back(c);

    c = {nextConnectorId_++,
         ConnectorRole::Generic,
         PartSide::Center,
         {0.0f, -0.45f, 0.0f}};
    result.push_back(c);

    return result;
  }

  if (part == ModBodyPart::StomachBody) {
    c = {nextConnectorId_++,
         ConnectorRole::Generic,
         PartSide::Center,
         {0.0f, 0.0f, 0.0f}};
    result.push_back(c);

    c = {nextConnectorId_++,
         ConnectorRole::Hip,
         PartSide::Left,
         {-0.35f, -0.45f, 0.0f}};
    result.push_back(c);

    c = {nextConnectorId_++,
         ConnectorRole::Hip,
         PartSide::Right,
         {0.35f, -0.45f, 0.0f}};
    result.push_back(c);

    return result;
  }

  c = {nextConnectorId_++, ConnectorRole::Generic, side, {0.0f, 0.0f, 0.0f}};
  result.push_back(c);

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
  return ModAssemblyUtil::CanPartParentChild(parent, child);
}

bool ModAssemblyGraph::IsSideCompatible(PartSide parentSide,
                                        PartSide childSide) const {
  return ModAssemblyUtil::IsSideCompatible(parentSide, childSide);
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
  const std::vector<int> directChildren = GetChildren(bodyPartId);

  // まず Body 直下の腕・脚セットだけ消す
  for (size_t i = 0; i < directChildren.size(); ++i) {
    const int childId = directChildren[i];

    std::unordered_map<int, PartNode>::iterator it = nodes_.find(childId);
    if (it == nodes_.end()) {
      continue;
    }

    if (IsLimbRoot(it->second.part)) {
      if (!RemoveSubtree(childId)) {
        return false;
      }
    }
  }

  // 残った直下子を再接続する
  const std::vector<int> remainChildren = GetChildren(bodyPartId);

  for (size_t i = 0; i < remainChildren.size(); ++i) {
    const int childId = remainChildren[i];

    if (nodes_.count(childId) == 0) {
      continue;
    }

    PartNode &child = nodes_[childId];

    // Neck は Body 削除後はルートへ
    if (child.part == ModBodyPart::Neck) {
      DetachPartFromParent(childId);
      continue;
    }

    // Head も Body 削除後はいったんルートへ
    if (child.part == ModBodyPart::Head) {
      DetachPartFromParent(childId);
      continue;
    }

    DetachPartFromParent(childId);
  }

  return true;
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

bool ModAssemblyGraph::IsBodyChildPart(ModBodyPart part) const {
  // Body 直下に戻す候補
  // Head は首優先なのでここには含めない
  return part == ModBodyPart::Neck || part == ModBodyPart::LeftUpperArm ||
         part == ModBodyPart::RightUpperArm || part == ModBodyPart::LeftThigh ||
         part == ModBodyPart::RightThigh;
}

bool ModAssemblyGraph::IsHeadAttachableToBody(ModBodyPart part) const {
  return part == ModBodyPart::Head;
}

int ModAssemblyGraph::FindFirstPartId(ModBodyPart part, int excludeId) const {
  std::vector<int> ids = GetNodeIdsSorted();

  for (size_t i = 0; i < ids.size(); ++i) {
    const int id = ids[i];
    if (id == excludeId) {
      continue;
    }

    std::unordered_map<int, PartNode>::const_iterator it = nodes_.find(id);
    if (it == nodes_.end()) {
      continue;
    }

    if (it->second.part == part) {
      return id;
    }
  }

  return -1;
}

int ModAssemblyGraph::FindPreferredParentForChild(int childId,
                                                  int removedPartId) const {
  std::unordered_map<int, PartNode>::const_iterator childIt =
      nodes_.find(childId);
  if (childIt == nodes_.end()) {
    return -1;
  }

  const PartNode &child = childIt->second;

  const bool hasBody =
      bodyId_ >= 0 && nodes_.count(bodyId_) > 0 && bodyId_ != removedPartId;
  const bool hasHead =
      headId_ >= 0 && nodes_.count(headId_) > 0 && headId_ != removedPartId;

  // Head は Neck 優先、無ければ Body、さらに無ければ親なし
  if (child.part == ModBodyPart::Head) {
    const int neckId = FindFirstPartId(ModBodyPart::Neck, removedPartId);
    if (neckId >= 0 && neckId != childId) {
      return neckId;
    }

    if (hasBody && bodyId_ != childId) {
      return bodyId_;
    }

    return -1;
  }

  // Neck と腕根元は Body 優先、無ければ Head
  if (child.part == ModBodyPart::Neck ||
      child.part == ModBodyPart::LeftUpperArm ||
      child.part == ModBodyPart::RightUpperArm) {
    if (hasBody && bodyId_ != childId) {
      return bodyId_;
    }

    if (hasHead && headId_ != childId) {
      return headId_;
    }

    return -1;
  }

  // 腿は Stomach 優先、無ければ Body
  if (child.part == ModBodyPart::LeftThigh ||
      child.part == ModBodyPart::RightThigh) {

    int stomachId = FindFirstPartId(ModBodyPart::StomachBody, removedPartId);
    if (stomachId >= 0)
      return stomachId;
    if (hasHead && headId_ != childId) {
      return headId_;
    }
  }

  // 前腕は対応する上腕優先
  if (child.part == ModBodyPart::LeftForeArm) {
    const int upperId =
        FindFirstPartId(ModBodyPart::LeftUpperArm, removedPartId);
    if (upperId >= 0) {
      return upperId;
    }
  }

  if (child.part == ModBodyPart::RightForeArm) {
    const int upperId =
        FindFirstPartId(ModBodyPart::RightUpperArm, removedPartId);
    if (upperId >= 0) {
      return upperId;
    }
  }

  // 脛は対応する腿優先
  if (child.part == ModBodyPart::LeftShin) {
    const int thighId = FindFirstPartId(ModBodyPart::LeftThigh, removedPartId);
    if (thighId >= 0) {
      return thighId;
    }
  }

  if (child.part == ModBodyPart::RightShin) {
    const int thighId = FindFirstPartId(ModBodyPart::RightThigh, removedPartId);
    if (thighId >= 0) {
      return thighId;
    }
  }

  // 最後の保険として近さベースに落とす
  return FindBestParentForChild(childId);
}

void ModAssemblyGraph::ReattachPartsForBodyRestore(int newBodyId) {
  std::vector<int> ids = GetNodeIdsSorted();

  // まず Body 直下に戻すべき部位を Body に戻す
  for (size_t i = 0; i < ids.size(); ++i) {
    const int id = ids[i];
    if (id == newBodyId) {
      continue;
    }

    std::unordered_map<int, PartNode>::iterator it = nodes_.find(id);
    if (it == nodes_.end()) {
      continue;
    }

    if (!IsBodyChildPart(it->second.part)) {
      continue;
    }

    it->second.parentId = newBodyId;
    it->second.parentConnectorId =
        FindConnectorIdForChild(newBodyId, it->second.part, it->second.side);

    if (!it->second.connectors.empty()) {
      it->second.selfConnectorId = it->second.connectors[0].id;
    }

    it->second.localTransform.translate =
        MakeDefaultLocalTranslate(it->second.part, it->second.side);
  }

  // 次に Head を Neck 優先でつなぎ直す。Neck が無ければ Body に戻す
  const int neckId = FindFirstPartId(ModBodyPart::Neck);
  ids = GetNodeIdsSorted();

  for (size_t i = 0; i < ids.size(); ++i) {
    const int id = ids[i];
    if (id == newBodyId) {
      continue;
    }

    std::unordered_map<int, PartNode>::iterator it = nodes_.find(id);
    if (it == nodes_.end()) {
      continue;
    }

    if (it->second.part != ModBodyPart::Head) {
      continue;
    }

    int newParentId = -1;
    if (neckId >= 0) {
      newParentId = neckId;
    } else {
      newParentId = newBodyId;
    }

    if (newParentId < 0) {
      continue;
    }

    it->second.parentId = newParentId;
    it->second.parentConnectorId =
        FindConnectorIdForChild(newParentId, it->second.part, it->second.side);

    if (!it->second.connectors.empty()) {
      it->second.selfConnectorId = it->second.connectors[0].id;
    }

    it->second.localTransform.translate =
        MakeDefaultLocalTranslate(it->second.part, it->second.side);
  }
}

void ModAssemblyGraph::AttachPartToParent(int childId, int parentId) {
  if (nodes_.count(childId) == 0) {
    return;
  }
  if (nodes_.count(parentId) == 0) {
    return;
  }

  PartNode &child = nodes_[childId];
  const PartNode &parent = nodes_.at(parentId);

  if (!CanParentChild(parent.part, child.part)) {
    return;
  }
  if (!IsSideCompatible(parent.side, child.side)) {
    return;
  }

  child.parentId = parentId;
  child.parentConnectorId =
      FindConnectorIdForChild(parentId, child.part, child.side);
  child.selfConnectorId =
      child.connectors.empty() ? -1 : child.connectors[0].id;

  ResetChildAttachLocal(child);
}

Vector3 ModAssemblyGraph::MakeDefaultAttachLocal(ModBodyPart parentPart,
                                                 ModBodyPart childPart,
                                                 PartSide childSide) const {
  switch (parentPart) {
  case ModBodyPart::ChestBody: {
    switch (childPart) {
    case ModBodyPart::Neck:
      return {0.0f, 0.45f, 0.0f};

    case ModBodyPart::LeftUpperArm:
      return {-0.75f, 0.30f, 0.0f};

    case ModBodyPart::RightUpperArm:
      return {0.75f, 0.30f, 0.0f};

    case ModBodyPart::LeftThigh:
      return {-0.35f, -0.70f, 0.0f};

    case ModBodyPart::RightThigh:
      return {0.35f, -0.70f, 0.0f};

    default:
      break;
    }
    break;
  }

  case ModBodyPart::StomachBody: {
    switch (childPart) {
    case ModBodyPart::LeftThigh:
      return {-0.35f, -0.45f, 0.0f};

    case ModBodyPart::RightThigh:
      return {0.35f, -0.45f, 0.0f};

    default:
      break;
    }
    break;
  }

  case ModBodyPart::Neck: {
    if (childPart == ModBodyPart::Head) {
      return {0.0f, 0.0f, 0.0f};
    }
    break;
  }

  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::RightUpperArm: {
    if (childPart == ModBodyPart::LeftForeArm ||
        childPart == ModBodyPart::RightForeArm) {
      return {0.0f, 0.0f, 0.0f};
    }
    break;
  }

  case ModBodyPart::LeftThigh:
  case ModBodyPart::RightThigh: {
    if (childPart == ModBodyPart::LeftShin ||
        childPart == ModBodyPart::RightShin) {
      return {0.0f, 0.0f, 0.0f};
    }
    break;
  }

  case ModBodyPart::Head:
  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
  case ModBodyPart::Count:
  default:
    break;
  }

  if (childSide == PartSide::Left) {
    return {-0.35f, 0.0f, 0.0f};
  }
  if (childSide == PartSide::Right) {
    return {0.35f, 0.0f, 0.0f};
  }
  return {0.0f, 0.0f, 0.0f};
}

void ModAssemblyGraph::ResetChildAttachLocal(PartNode &child) {
  if (child.parentId < 0 || nodes_.count(child.parentId) == 0) {
    child.localTransform.translate =
        MakeDefaultLocalTranslate(child.part, child.side);
    return;
  }

  const PartNode &parent = nodes_.at(child.parentId);
  child.localTransform.translate =
      MakeDefaultAttachLocal(parent.part, child.part, child.side);
}

void ModAssemblyGraph::DetachPartFromParent(int childId) {
  if (nodes_.count(childId) == 0) {
    return;
  }

  PartNode &child = nodes_[childId];
  child.parentId = -1;
  child.parentConnectorId = -1;
  child.selfConnectorId =
      child.connectors.empty() ? -1 : child.connectors[0].id;
}

void ModAssemblyGraph::NormalizeBodyChildLinks() {
  if (bodyId_ < 0 || nodes_.count(bodyId_) == 0) {
    return;
  }

  std::vector<int> ids = GetNodeIdsSorted();

  for (size_t i = 0; i < ids.size(); ++i) {
    const int id = ids[i];
    if (id == bodyId_) {
      continue;
    }

    std::unordered_map<int, PartNode>::const_iterator it = nodes_.find(id);
    if (it == nodes_.end()) {
      continue;
    }

    const ModBodyPart part = it->second.part;

    if (part == ModBodyPart::Neck || part == ModBodyPart::LeftUpperArm ||
        part == ModBodyPart::RightUpperArm) {
      AttachPartToParent(id, bodyId_);
    }

    if (part == ModBodyPart::LeftThigh || part == ModBodyPart::RightThigh) {
      int stomachId = FindFirstPartId(ModBodyPart::StomachBody);
      if (stomachId >= 0) {
        AttachPartToParent(id, stomachId);
      } else {
        AttachPartToParent(id, bodyId_);
      }
    }
  }
}

void ModAssemblyGraph::NormalizeHeadLinks() {
  std::vector<int> ids = GetNodeIdsSorted();

  for (size_t i = 0; i < ids.size(); ++i) {
    const int id = ids[i];

    std::unordered_map<int, PartNode>::iterator it = nodes_.find(id);
    if (it == nodes_.end()) {
      continue;
    }

    PartNode &head = it->second;
    if (head.part != ModBodyPart::Head) {
      continue;
    }

    // すでに Neck の子ならそのまま維持する
    if (head.parentId >= 0 && nodes_.count(head.parentId) > 0) {
      const PartNode &parent = nodes_.at(head.parentId);
      if (parent.part == ModBodyPart::Neck) {
        continue;
      }
    }

    // Neck 親でない、または親を失った Head だけ再接続先を探す
    int targetNeckId = -1;

    // できればまだ Head を持っていない Neck を優先する
    for (size_t j = 0; j < ids.size(); ++j) {
      const int neckCandidateId = ids[j];

      std::unordered_map<int, PartNode>::const_iterator neckIt =
          nodes_.find(neckCandidateId);
      if (neckIt == nodes_.end()) {
        continue;
      }

      if (neckIt->second.part != ModBodyPart::Neck) {
        continue;
      }

      const int existingHeadChild =
          FindFirstChildPartId(*this, neckCandidateId, ModBodyPart::Head);

      if (existingHeadChild < 0) {
        targetNeckId = neckCandidateId;
        break;
      }
    }

    // 空き Neck が無ければ、どれか1つの Neck へ
    if (targetNeckId < 0) {
      targetNeckId = FindFirstPartId(ModBodyPart::Neck);
    }

    if (targetNeckId >= 0 && targetNeckId != id) {
      AttachPartToParent(id, targetNeckId);
      continue;
    }

    // Neck が無ければ Body へ
    if (bodyId_ >= 0 && nodes_.count(bodyId_) > 0 && bodyId_ != id) {
      AttachPartToParent(id, bodyId_);
      continue;
    }

    // どこにも付けられなければ親なし
    DetachPartFromParent(id);
  }

  RefreshManagedPartIds();
}

bool ModAssemblyGraph::ShouldCascadeDeleteChildren(ModBodyPart part) const {
  return part == ModBodyPart::Neck || part == ModBodyPart::LeftUpperArm ||
         part == ModBodyPart::RightUpperArm || part == ModBodyPart::LeftThigh ||
         part == ModBodyPart::RightThigh;
}

void ModAssemblyGraph::RefreshManagedPartIds() {
  bodyId_ = -1;
  headId_ = -1;
  neckId_ = -1;

  for (std::unordered_map<int, PartNode>::const_iterator it = nodes_.begin();
       it != nodes_.end(); ++it) {
    if (bodyId_ < 0 && it->second.part == ModBodyPart::ChestBody) {
      bodyId_ = it->first;
    }
    if (headId_ < 0 && it->second.part == ModBodyPart::Head) {
      headId_ = it->first;
    }
    if (neckId_ < 0 && it->second.part == ModBodyPart::Neck) {
      neckId_ = it->first;
    }

    if (bodyId_ >= 0 && headId_ >= 0 && neckId_ >= 0) {
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
  std::unordered_map<int, PartNode>::iterator it = nodes_.find(partId);
  if (it == nodes_.end()) {
    return false;
  }

  PartNode target = it->second;

  if (target.part == ModBodyPart::Head) {
    if (target.parentId >= 0 && nodes_.count(target.parentId) > 0) {
      std::unordered_map<int, PartNode>::const_iterator parentIt =
          nodes_.find(target.parentId);
      if (parentIt != nodes_.end() &&
          parentIt->second.part == ModBodyPart::Neck) {
        partId = parentIt->second.id;
        it = nodes_.find(partId);
        if (it == nodes_.end()) {
          return false;
        }
        target = it->second;
      }
    }
  }

  if (target.required) {
    return false;
  }

  if (IsHead(target.part) && CountHeads() <= 1) {
    return false;
  }

  if (IsLegRoot(target.part) && CountLegRoots() <= 2) {
    return false;
  }

  if (target.part == ModBodyPart::ChestBody ||
      target.part == ModBodyPart::StomachBody) {
    if (!RemoveBodyAttachedLimbsAndReattachOthers(partId)) {
      return false;
    }

    nodes_.erase(partId);
    RefreshManagedPartIds();
    NormalizeHeadLinks();
    return true;
  }

  if (ShouldCascadeDeleteChildren(target.part)) {
    if (!RemoveChildrenRecursive(partId)) {
      return false;
    }

    nodes_.erase(partId);
    RefreshManagedPartIds();
    NormalizeHeadLinks();
    return true;
  }

  if (!ReattachChildrenOf(partId)) {
    return false;
  }

  nodes_.erase(partId);
  RefreshManagedPartIds();
  NormalizeHeadLinks();
  return true;
}

bool ModAssemblyGraph::ReattachChildrenOf(int removedPartId) {
  std::vector<int> children = GetChildren(removedPartId);

  for (size_t i = 0; i < children.size(); ++i) {
    const int childId = children[i];

    if (nodes_.count(childId) == 0) {
      continue;
    }

    PartNode &child = nodes_[childId];

        if (child.part == ModBodyPart::Head) {
      int targetNeckId = -1;
      const std::vector<int> allIds = GetNodeIdsSorted();

      for (size_t j = 0; j < allIds.size(); ++j) {
        const int candidateId = allIds[j];
        if (candidateId == removedPartId || candidateId == childId) {
          continue;
        }

        if (nodes_.count(candidateId) == 0) {
          continue;
        }

        const PartNode &candidate = nodes_.at(candidateId);
        if (candidate.part != ModBodyPart::Neck) {
          continue;
        }

        const int existingHeadChild =
            FindFirstChildPartId(*this, candidateId, ModBodyPart::Head);

        if (existingHeadChild < 0) {
          targetNeckId = candidateId;
          break;
        }
      }

      if (targetNeckId < 0) {
        targetNeckId = FindFirstPartId(ModBodyPart::Neck, removedPartId);
      }

      if (targetNeckId >= 0 && targetNeckId != childId) {
        AttachPartToParent(childId, targetNeckId);
      } else if (bodyId_ >= 0 && nodes_.count(bodyId_) > 0 &&
                 bodyId_ != removedPartId && bodyId_ != childId) {
        AttachPartToParent(childId, bodyId_);
      } else {
        DetachPartFromParent(childId);
      }
      continue;
    }

    if (child.part == ModBodyPart::Neck) {
      if (bodyId_ >= 0 && nodes_.count(bodyId_) > 0 &&
          bodyId_ != removedPartId) {
        AttachPartToParent(childId, bodyId_);
      } else {
        DetachPartFromParent(childId);
      }
      continue;
    }

    if (child.part == ModBodyPart::LeftUpperArm ||
        child.part == ModBodyPart::RightUpperArm) {
      if (bodyId_ >= 0 && nodes_.count(bodyId_) > 0 &&
          bodyId_ != removedPartId) {
        AttachPartToParent(childId, bodyId_);
      } else if (headId_ >= 0 && nodes_.count(headId_) > 0 &&
                 headId_ != removedPartId && headId_ != childId) {
        AttachPartToParent(childId, headId_);
      } else {
        DetachPartFromParent(childId);
      }
      continue;
    }

    if (child.part == ModBodyPart::LeftThigh ||
        child.part == ModBodyPart::RightThigh) {
      int stomachId = FindFirstPartId(ModBodyPart::StomachBody, removedPartId);

      if (stomachId >= 0 && stomachId != childId) {
        AttachPartToParent(childId, stomachId);
      } else if (bodyId_ >= 0 && nodes_.count(bodyId_) > 0 &&
                 bodyId_ != removedPartId && bodyId_ != childId) {
        AttachPartToParent(childId, bodyId_);
      } else if (headId_ >= 0 && nodes_.count(headId_) > 0 &&
                 headId_ != removedPartId && headId_ != childId) {
        AttachPartToParent(childId, headId_);
      } else {
        DetachPartFromParent(childId);
      }
      continue;
    }

    int newParent = FindPreferredParentForChild(childId, removedPartId);

    if (newParent >= 0) {
      AttachPartToParent(childId, newParent);
    } else {
      DetachPartFromParent(childId);
    }
  }

  NormalizeHeadLinks();
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
  if (nodes_.count(partId) == 0 || nodes_.count(newParentId) == 0) {
    return false;
  }

  PartNode &node = nodes_[partId];
  const PartNode &parent = nodes_.at(newParentId);

  if (!CanParentChild(parent.part, node.part)) {
    return false;
  }
  if (!IsSideCompatible(parent.side, node.side)) {
    return false;
  }

  node.parentId = newParentId;
  node.parentConnectorId =
      newParentConnectorId >= 0
          ? newParentConnectorId
          : FindConnectorIdForChild(newParentId, node.part, node.side);
  node.selfConnectorId = node.connectors.empty() ? -1 : node.connectors[0].id;

  ResetChildAttachLocal(node);
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

int ModAssemblyGraph::FindFirstChildPartId(const ModAssemblyGraph &assembly,
                                           int parentId,
                         ModBodyPart wantedPart) {
  const std::vector<int> children = assembly.GetChildren(parentId);
  for (size_t i = 0; i < children.size(); ++i) {
    const PartNode *child = assembly.FindNode(children[i]);
    if (child != nullptr && child->part == wantedPart) {
      return child->id;
    }
  }
  return -1;
}

bool ModAssemblyGraph::SetPartLocalRotate(int partId,
                                          const Vector3 &localRotate) {
  std::unordered_map<int, PartNode>::iterator it = nodes_.find(partId);
  if (it == nodes_.end()) {
    return false;
  }

  it->second.localTransform.rotate = localRotate;
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
  if (side != PartSide::Left && side != PartSide::Right) {
    return false;
  }

  int parentId = -1;

  if (bodyId_ >= 0 && nodes_.count(bodyId_) > 0) {
    parentId = bodyId_;
  } else if (headId_ >= 0 && nodes_.count(headId_) > 0) {
    parentId = headId_;
  } else {
    return false;
  }

  const ModBodyPart upper = (side == PartSide::Left)
                                ? ModBodyPart::LeftUpperArm
                                : ModBodyPart::RightUpperArm;
  const ModBodyPart fore = (side == PartSide::Left) ? ModBodyPart::LeftForeArm
                                                    : ModBodyPart::RightForeArm;

  const int upperId = AddPart(upper, side, parentId, false);
  AddPart(fore, side, upperId, false);
  return true;
}

bool ModAssemblyGraph::AddLegAssembly(PartSide side) {
  if (side != PartSide::Left && side != PartSide::Right) {
    return false;
  }

  int parentId = -1;

  int stomachId = FindFirstPartId(ModBodyPart::StomachBody);

  if (stomachId >= 0) {
    parentId = stomachId;
  } else if (bodyId_ >= 0 && nodes_.count(bodyId_) > 0) {
    parentId = bodyId_;
  } else if (headId_ >= 0 && nodes_.count(headId_) > 0) {
    parentId = headId_;
  } else {
    return false;
  }

  const ModBodyPart thigh = (side == PartSide::Left) ? ModBodyPart::LeftThigh
                                                     : ModBodyPart::RightThigh;
  const ModBodyPart shin =
      (side == PartSide::Left) ? ModBodyPart::LeftShin : ModBodyPart::RightShin;

  const int thighId = AddPart(thigh, side, parentId, false);
  AddPart(shin, side, thighId, false);
  return true;
}

bool ModAssemblyGraph::AddNeckPart() {
  RefreshManagedPartIds();

  int parentId = -1;
  if (bodyId_ >= 0 && nodes_.count(bodyId_) > 0) {
    parentId = bodyId_;
  }

  const int newNeckId =
      AddPart(ModBodyPart::Neck, PartSide::Center, parentId, false);
  if (newNeckId < 0) {
    return false;
  }

  const int newHeadId =
      AddPart(ModBodyPart::Head, PartSide::Center, newNeckId, false);
  if (newHeadId < 0) {
    nodes_.erase(newNeckId);
    RefreshManagedPartIds();
    return false;
  }

  RefreshManagedPartIds();
  return true;
}

bool ModAssemblyGraph::AddHeadPart() {
  // 互換用。頭単体ではなく頭首セット追加へ統一する
  return AddNeckPart();
}

bool ModAssemblyGraph::AddBodyPart() {
  // Body が既に存在する場合は追加しない
  if (bodyId_ >= 0 && nodes_.count(bodyId_) > 0) {
    return false;
  }

  // Body は最上位として追加する
  const int newBodyId =
      AddPart(ModBodyPart::ChestBody, PartSide::Center, -1, false);

  if (newBodyId < 0) {
    return false;
  }

  bodyId_ = newBodyId;

  // Body の子に戻すべき部位を戻す
  NormalizeBodyChildLinks();

  // Head は Neck 優先、無ければ Body に戻す
  NormalizeHeadLinks();

  return true;
}