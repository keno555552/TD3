#pragma once
#include "GAME/actor/ModAssemblyTypes.h"
#include "Vector3.h"

namespace ModAssemblyUtil {

inline ModAssemblyType GetAssemblyType(ModBodyPart part) {
  switch (part) {
  case ModBodyPart::ChestBody:
  case ModBodyPart::StomachBody:
    return ModAssemblyType::Body;

  case ModBodyPart::Neck:
  case ModBodyPart::Head:
    return ModAssemblyType::Head;

  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightUpperArm:
  case ModBodyPart::RightForeArm:
    return ModAssemblyType::Arm;

  case ModBodyPart::LeftThigh:
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightThigh:
  case ModBodyPart::RightShin:
    return ModAssemblyType::Leg;

  case ModBodyPart::Count:
  default:
    return ModAssemblyType::None;
  }
}

inline PartSide GetAssemblySide(ModBodyPart part) {
  switch (part) {
  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::LeftForeArm:
  case ModBodyPart::LeftThigh:
  case ModBodyPart::LeftShin:
    return PartSide::Left;

  case ModBodyPart::RightUpperArm:
  case ModBodyPart::RightForeArm:
  case ModBodyPart::RightThigh:
  case ModBodyPart::RightShin:
    return PartSide::Right;

  case ModBodyPart::ChestBody:
  case ModBodyPart::StomachBody:
  case ModBodyPart::Neck:
  case ModBodyPart::Head:
  case ModBodyPart::Count:
  default:
    return PartSide::Center;
  }
}

inline ModAssemblyKey GetAssemblyKey(ModBodyPart part) {
  ModAssemblyKey key;
  key.type = GetAssemblyType(part);
  key.side = GetAssemblySide(part);
  return key;
}

inline bool IsAssemblyRootPart(ModBodyPart part) {
  switch (part) {
  case ModBodyPart::ChestBody:
  case ModBodyPart::Neck:
  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::RightUpperArm:
  case ModBodyPart::LeftThigh:
  case ModBodyPart::RightThigh:
    return true;

  case ModBodyPart::StomachBody:
  case ModBodyPart::Head:
  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
  case ModBodyPart::Count:
  default:
    return false;
  }
}

inline ModBodyPart GetAssemblyRootPartType(ModBodyPart part) {
  switch (part) {
  case ModBodyPart::ChestBody:
  case ModBodyPart::StomachBody:
    return ModBodyPart::ChestBody;

  case ModBodyPart::Neck:
  case ModBodyPart::Head:
    return ModBodyPart::Neck;

  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::LeftForeArm:
    return ModBodyPart::LeftUpperArm;

  case ModBodyPart::RightUpperArm:
  case ModBodyPart::RightForeArm:
    return ModBodyPart::RightUpperArm;

  case ModBodyPart::LeftThigh:
  case ModBodyPart::LeftShin:
    return ModBodyPart::LeftThigh;

  case ModBodyPart::RightThigh:
  case ModBodyPart::RightShin:
    return ModBodyPart::RightThigh;

  case ModBodyPart::Count:
  default:
    return part;
  }
}

inline bool IsSameAssembly(ModBodyPart lhs, ModBodyPart rhs) {
  return GetAssemblyKey(lhs) == GetAssemblyKey(rhs);
}

/// <summary>
/// Assembly 内部の固定親子関係か
/// これは「外部接続ルール」とは別
/// </summary>
inline bool IsInternalAssemblyParentChild(ModBodyPart parent,
                                          ModBodyPart child) {
  switch (parent) {
  case ModBodyPart::ChestBody:
    return child == ModBodyPart::StomachBody;

  case ModBodyPart::Neck:
    return child == ModBodyPart::Head;

  case ModBodyPart::LeftUpperArm:
    return child == ModBodyPart::LeftForeArm;

  case ModBodyPart::RightUpperArm:
    return child == ModBodyPart::RightForeArm;

  case ModBodyPart::LeftThigh:
    return child == ModBodyPart::LeftShin;

  case ModBodyPart::RightThigh:
    return child == ModBodyPart::RightShin;

  case ModBodyPart::StomachBody:
  case ModBodyPart::Head:
  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
  case ModBodyPart::Count:
  default:
    return false;
  }
}

/// <summary>
/// 外部接続用の assembly ルール
/// Body -> Head / Arm / Leg
/// Head -> Arm / Leg
/// Arm / Leg -> 何も付けない
/// Body は子にならない
/// </summary>
inline bool CanAssemblyParentChild(ModAssemblyType parent,
                                   ModAssemblyType child) {
  switch (parent) {
  case ModAssemblyType::Body:
    return child == ModAssemblyType::Head || child == ModAssemblyType::Arm ||
           child == ModAssemblyType::Leg;

  case ModAssemblyType::Head:
    return child == ModAssemblyType::Arm || child == ModAssemblyType::Leg;

  case ModAssemblyType::Arm:
  case ModAssemblyType::Leg:
  case ModAssemblyType::None:
  default:
    return false;
  }
}

/// <summary>
/// part 単位から見た親子有効判定
/// 内部固定接続を先に許可し、それ以外は assembly root 同士の外部接続で判定する
/// </summary>
inline bool CanPartParentChild(ModBodyPart parent, ModBodyPart child) {
  if (IsInternalAssemblyParentChild(parent, child)) {
    return true;
  }

  if (!IsAssemblyRootPart(parent) || !IsAssemblyRootPart(child)) {
    return false;
  }

  const ModAssemblyType parentAssembly = GetAssemblyType(parent);
  const ModAssemblyType childAssembly = GetAssemblyType(child);

  if (childAssembly == ModAssemblyType::Body) {
    return false;
  }

  return CanAssemblyParentChild(parentAssembly, childAssembly);
}

inline bool IsSideCompatible(PartSide parentSide, PartSide childSide) {
  if (parentSide == PartSide::Center || childSide == PartSide::Center) {
    return true;
  }

  return parentSide == childSide;
}

inline Vector3 GetFaceLocalNormal(ModAttachFace face) {
  switch (face) {
  case ModAttachFace::PosX:
    return {1.0f, 0.0f, 0.0f};
  case ModAttachFace::NegX:
    return {-1.0f, 0.0f, 0.0f};
  case ModAttachFace::PosY:
    return {0.0f, 1.0f, 0.0f};
  case ModAttachFace::NegY:
    return {0.0f, -1.0f, 0.0f};
  case ModAttachFace::PosZ:
    return {0.0f, 0.0f, 1.0f};
  case ModAttachFace::NegZ:
    return {0.0f, 0.0f, -1.0f};
  default:
    return {0.0f, 1.0f, 0.0f};
  }
}

/// <summary>
/// 各 Assembly の root 接続面ローカル法線
/// 自動回転の基準用
/// </summary>
inline Vector3 GetRootAttachLocalNormal(ModBodyPart rootPart) {
  switch (rootPart) {
  case ModBodyPart::ChestBody:
    return {0.0f, 1.0f, 0.0f};

  case ModBodyPart::Neck:
    return {0.0f, -1.0f, 0.0f};

  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::RightUpperArm:
  case ModBodyPart::LeftThigh:
  case ModBodyPart::RightThigh:
    return {0.0f, 1.0f, 0.0f};

  case ModBodyPart::StomachBody:
  case ModBodyPart::Head:
  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
  case ModBodyPart::Count:
  default:
    return {0.0f, 1.0f, 0.0f};
  }
}

} // namespace ModAssemblyUtil