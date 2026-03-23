#pragma once
#include "GAME/actor/ModBody.h"
#include "GAME/theme/ThemeData.h"
#include <array>

/// スコア計算結果
struct ScoreResult {
  // パーツごとのスケール基本スコア（ModBodyPart 全 11 種）
  std::array<float, static_cast<size_t>(ModBodyPart::Count)> partScores{};
  float totalPartScore = 0.0f;

  // 本数スコア（左右の腕脚 4 種）
  std::array<float, static_cast<size_t>(CountPart::Count)> countScores{};
  float totalCountScore = 0.0f;

  // ボーナススコア（5 種）
  std::array<float, static_cast<size_t>(BonusType::Count)> bonusScores{};
  float totalBonusScore = 0.0f;

  // 最終スコア（全スコアの合計）
  float finalScore = 0.0f;
};
