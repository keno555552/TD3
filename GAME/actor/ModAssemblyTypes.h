#pragma once
#include "GAME/actor/ModBody.h"

/// <summary>
/// Assembly の種類
/// 編集・接続判定はこの単位で扱う
/// </summary>
enum class ModAssemblyType {
  None = 0,
  Body,
  Head,
  Arm,
  Leg,
};

/// <summary>
/// 左右属性
/// AssemblyType とは別に持つ
/// </summary>
enum class PartSide {
  Center = 0,
  Left,
  Right,
};

/// <summary>
/// 親候補部位の6面
/// 接続候補面・法線選択で使う
/// </summary>
enum class ModAttachFace {
  PosX = 0,
  NegX,
  PosY,
  NegY,
  PosZ,
  NegZ,
};

/// <summary>
/// Assembly 単位で識別するためのキー
/// </summary>
struct ModAssemblyKey {
  ModAssemblyType type = ModAssemblyType::None;
  PartSide side = PartSide::Center;

  bool operator==(const ModAssemblyKey &rhs) const {
    return type == rhs.type && side == rhs.side;
  }

  bool operator!=(const ModAssemblyKey &rhs) const { return !(*this == rhs); }
};