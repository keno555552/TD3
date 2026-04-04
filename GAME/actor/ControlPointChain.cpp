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

void ControlPointChain::Clear() { nodes_.clear(); }

void ControlPointChain::BuildArmChain() {
  nodes_.clear();

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

  ControlPointNode elbow{};
  elbow.role = ModControlPointRole::Bend;
  elbow.parentIndex = 0;
  elbow.localPosition = {0.0f, -0.55f, 0.0f};
  elbow.radius = 0.08f;
  elbow.movable = true;
  nodes_.push_back(elbow);

  ControlPointNode wrist{};
  wrist.role = ModControlPointRole::End;
  wrist.parentIndex = 1;
  wrist.localPosition = {0.0f, -0.55f, 0.0f};
  wrist.radius = 0.08f;
  wrist.movable = true;
  wrist.isConnectionPoint = true;
  wrist.acceptsChild = true;
  nodes_.push_back(wrist);

  UpdateHierarchy();
}

void ControlPointChain::BuildLegChain() {
  nodes_.clear();

  ControlPointNode hip{};
  hip.role = ModControlPointRole::Root;
  hip.parentIndex = -1;
  hip.localPosition = {0.0f, 0.0f, 0.0f};
  hip.radius = 0.10f;
  hip.movable = false;
  hip.isConnectionPoint = true;
  hip.acceptsParent = true;
  nodes_.push_back(hip);

  ControlPointNode knee{};
  knee.role = ModControlPointRole::Bend;
  knee.parentIndex = 0;
  knee.localPosition = {0.0f, -0.70f, 0.0f};
  knee.radius = 0.09f;
  knee.movable = true;
  nodes_.push_back(knee);

  ControlPointNode ankle{};
  ankle.role = ModControlPointRole::End;
  ankle.parentIndex = 1;
  ankle.localPosition = {0.0f, -0.70f, 0.0f};
  ankle.radius = 0.09f;
  ankle.movable = true;
  ankle.isConnectionPoint = true;
  ankle.acceptsChild = true;
  nodes_.push_back(ankle);

  UpdateHierarchy();
}

void ControlPointChain::BuildTorsoChain() {
  nodes_.clear();

  ControlPointNode chest{};
  chest.role = ModControlPointRole::Chest;
  chest.parentIndex = -1;
  chest.localPosition = {0.0f, 0.45f, 0.0f};
  chest.radius = 0.12f;
  chest.movable = true;
  chest.isConnectionPoint = true;
  chest.acceptsChild = true;
  nodes_.push_back(chest);

  ControlPointNode belly{};
  belly.role = ModControlPointRole::Belly;
  belly.parentIndex = 0;
  belly.localPosition = {0.0f, -0.45f, 0.0f};
  belly.radius = 0.10f;
  belly.movable = true;
  nodes_.push_back(belly);

  ControlPointNode waist{};
  waist.role = ModControlPointRole::Waist;
  waist.parentIndex = 1;
  waist.localPosition = {0.0f, -0.45f, 0.0f};
  waist.radius = 0.12f;
  waist.movable = true;
  waist.isConnectionPoint = true;
  waist.acceptsChild = true;
  nodes_.push_back(waist);

  UpdateHierarchy();
}

void ControlPointChain::BuildNeckChain() {
  nodes_.clear();

  ControlPointNode root{};
  root.role = ModControlPointRole::Root;
  root.parentIndex = -1;
  root.localPosition = {0.0f, 0.0f, 0.0f};
  root.radius = 0.09f;
  root.movable = false;
  root.isConnectionPoint = true;
  root.acceptsParent = true;
  root.acceptsChild = false;
  nodes_.push_back(root);

  ControlPointNode bend{};
  bend.role = ModControlPointRole::Bend;
  bend.parentIndex = 0;
  bend.localPosition = {0.0f, 0.28f, 0.0f};
  bend.radius = 0.08f;
  bend.movable = true;
  bend.isConnectionPoint = true;
  bend.acceptsParent = false;
  bend.acceptsChild = true;
  nodes_.push_back(bend);

  ControlPointNode end{};
  end.role = ModControlPointRole::End;
  end.parentIndex = 1;
  end.localPosition = {0.0f, 0.52f, 0.0f};
  end.radius = 0.11f;
  end.movable = true;
  end.isConnectionPoint = true;
  end.acceptsParent = false;
  end.acceptsChild = true;
  nodes_.push_back(end);

  UpdateHierarchy();
}

int ControlPointChain::FindIndex(ModControlPointRole role) const {
  for (size_t i = 0; i < nodes_.size(); ++i) {
    if (nodes_[i].role == role) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

bool ControlPointChain::MovePoint(size_t index,
                                  const Vector3 &newLocalPosition) {
  if (index >= nodes_.size()) {
    return false;
  }

  if (!nodes_[index].movable) {
    return false;
  }

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

  const ModControlPointRole role = nodes_[index].role;

  const int rootIndex = FindIndex(ModControlPointRole::Root);
  const int bendIndex = FindIndex(ModControlPointRole::Bend);
  const int endIndex = FindIndex(ModControlPointRole::End);

  if (role == ModControlPointRole::Bend && rootIndex >= 0 && endIndex >= 0) {
    const Vector3 rootPos = GetWorldPosition(static_cast<size_t>(rootIndex));
    const Vector3 oldBendPos = GetWorldPosition(index);
    const Vector3 oldEndPos = GetWorldPosition(static_cast<size_t>(endIndex));

    const Vector3 bendToEnd = Subtract(oldEndPos, oldBendPos);

    Vector3 rootToCandidate = Subtract(newLocalPosition, rootPos);
    Vector3 direction = NormalizeSafe(rootToCandidate, {0.0f, 1.0f, 0.0f});

    float radius = Length(rootToCandidate);
    radius = ClampFloat(radius, 0.15f, 1.80f);

    const Vector3 clampedBend = Add(rootPos, Multiply(radius, direction));

    nodes_[index].localPosition = ToLocalFromWorld(index, clampedBend);

    const Vector3 newEndWorld = Add(clampedBend, bendToEnd);
    nodes_[static_cast<size_t>(endIndex)].localPosition =
        ToLocalFromWorld(static_cast<size_t>(endIndex), newEndWorld);

    UpdateHierarchy();
    return true;
  }

  if (role == ModControlPointRole::End && bendIndex >= 0) {
    const Vector3 bendPos = GetWorldPosition(static_cast<size_t>(bendIndex));
    Vector3 bendToCandidate = Subtract(newLocalPosition, bendPos);

    Vector3 direction = NormalizeSafe(bendToCandidate, {0.0f, 1.0f, 0.0f});
    float length = Length(bendToCandidate);
    length = ClampFloat(length, 0.20f, 1.50f);

    const Vector3 clampedEnd = Add(bendPos, Multiply(length, direction));
    nodes_[index].localPosition = ToLocalFromWorld(index, clampedEnd);

    UpdateHierarchy();
    return true;
  }

  nodes_[index].localPosition = ToLocalFromWorld(index, newLocalPosition);
  UpdateHierarchy();
  return true;
}

Vector3 ControlPointChain::GetLocalPosition(size_t index) const {
  if (index >= nodes_.size()) {
    return {0.0f, 0.0f, 0.0f};
  }

  return nodes_[index].localPosition;
}

Vector3 ControlPointChain::GetWorldPosition(size_t index) const {
  if (index >= nodes_.size()) {
    return {0.0f, 0.0f, 0.0f};
  }

  const ControlPointNode &node = nodes_[index];
  if (node.parentIndex < 0) {
    return node.localPosition;
  }

  return Add(GetWorldPosition(static_cast<size_t>(node.parentIndex)),
             node.localPosition);
}

void ControlPointChain::UpdateHierarchy() {}