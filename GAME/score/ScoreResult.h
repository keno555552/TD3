#pragma once
#include "GAME/actor/ModBody.h"
#include "GAME/theme/ThemeData.h"
#include <array>
#include <string>

/// 審査員個別の評価結果
struct JudgeEvaluation {
  std::string judgeId;
  std::string judgeName;
  std::string judgeTitle;
  int star = 3; // ★1〜5
};

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

  // ★評価（各 1〜5）
  int starThemeMatch = 0;  // テーマへの一致度
  int starImpact = 0;      // インパクト（デフォルトからの変化量）
  int starEfficiency = 0;  // 効率（コスパ＋ミニマリスト）
  int starCommitment = 0;  // こだわり（最大変化量パーツの突出度）
  int starJudgeBonus = 0;  // 審査員ボーナス（3 人の平均）

  // ★合計（5 項目の合計、最大 25）
  int totalStars = 0;

  // 審査員個別の評価（3 人分）
  std::array<JudgeEvaluation, 3> judgeEvaluations{};

  // 総合ランク
  std::string overallRank = "D";
};
