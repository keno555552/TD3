#pragma once
#include <array>
#include <string>
#include <vector>

/// パーツごとのスケール倍率
struct PartWeight {
  float scaleX = 0.0f;
  float scaleY = 0.0f;
  float scaleZ = 0.0f;
};

/// 本数の倍率
struct CountWeight {
  float weight = 0.0f;
  bool invert = false;
};

/// ボーナスの倍率
struct BonusWeight {
  float weight = 0.0f;
  bool invert = false;
};

/// 本数評価の対象（左右の腕脚）
enum class CountPart {
  LeftArm,
  RightArm,
  LeftLeg,
  RightLeg,

  Count
};

/// ボーナスの種類
enum class BonusType {
  Symmetry,       // シンメトリー（左右対称）
  CostEfficiency, // コスパ（改造量の少なさとスコアのバランス）
  Minimalist,     // 改造部位の少なさ
  Wildcard,       // 予想外の改造
  Balance,        // 全体のバランス

  Count
};

/// 1つのスコアセット
struct ScoreSet {
  std::string setName;

  // ModBodyPart 全 11 種に対応するスケール倍率
  // インデックスは ModBodyPart の値をそのまま使用
  std::array<PartWeight, 11> partWeights{};

  // 左右の腕脚 4 種に対応する本数倍率
  std::array<CountWeight, static_cast<size_t>(CountPart::Count)> countWeights{};
};

/// お題データ本体
struct ThemeData {
  std::string themeId;
  std::string themeName;
  std::string category;
  std::string texturePath;

  // スコアセット（通常は 1 つ、抽象お題では複数）
  std::vector<ScoreSet> scoreSets;

  // ボーナス 5 種の倍率（全お題共通の種類、お題ごとに係数が異なる）
  std::array<BonusWeight, static_cast<size_t>(BonusType::Count)> bonusWeights{};
};
