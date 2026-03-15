#include "ScoreCalculator.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace {

/// パーツ名テーブル（ログ表示用）
const char *kPartNames[] = {
    "Body",         "Neck",        "Head",        "LeftUpperArm",
    "LeftForeArm",  "RightUpperArm", "RightForeArm", "LeftThigh",
    "LeftShin",     "RightThigh",  "RightShin",
};

/// カウント名テーブル（ログ表示用）
const char *kCountNames[] = {
    "LeftArm",
    "RightArm",
    "LeftLeg",
    "RightLeg",
};

/// ボーナス名テーブル（ログ表示用）
const char *kBonusNames[] = {
    "Symmetry",
    "CostEfficiency",
    "Minimalist",
    "Wildcard",
    "Balance",
};

/// 左右ペアの定義（シンメトリー計算用）
struct SymmetryPair {
  size_t left;
  size_t right;
};

const SymmetryPair kSymmetryPairs[] = {
    {static_cast<size_t>(ModBodyPart::LeftUpperArm),
     static_cast<size_t>(ModBodyPart::RightUpperArm)},
    {static_cast<size_t>(ModBodyPart::LeftForeArm),
     static_cast<size_t>(ModBodyPart::RightForeArm)},
    {static_cast<size_t>(ModBodyPart::LeftThigh),
     static_cast<size_t>(ModBodyPart::RightThigh)},
    {static_cast<size_t>(ModBodyPart::LeftShin),
     static_cast<size_t>(ModBodyPart::RightShin)},
};

constexpr size_t kPartCount = static_cast<size_t>(ModBodyPart::Count);
constexpr size_t kCountPartCount = static_cast<size_t>(CountPart::Count);
constexpr size_t kBonusCount = static_cast<size_t>(BonusType::Count);

/// 1パーツの変化量合計（|x-1| + |y-1| + |z-1|）
float GetPartChangeAmount(const ModBodyPartParam &param) {
  return std::abs(param.scale.x - 1.0f) + std::abs(param.scale.y - 1.0f) +
         std::abs(param.scale.z - 1.0f);
}

/// パーツが変化しているか
bool IsPartChanged(const ModBodyPartParam &param) {
  constexpr float kEpsilon = 0.001f;
  return std::abs(param.scale.x - 1.0f) > kEpsilon ||
         std::abs(param.scale.y - 1.0f) > kEpsilon ||
         std::abs(param.scale.z - 1.0f) > kEpsilon;
}

} // namespace

/*   小数点第二位以下を切り捨て   */
float ScoreCalculator::TruncateToFirstDecimal(float value) {
  return std::floor(value * 10.0f) / 10.0f;
}

/*   メイン計算関数   */
ScoreResult ScoreCalculator::Calculate(const ThemeData &theme,
                                       const ModBodyCustomizeData &playerData) {
  ScoreResult result;

  if (theme.scoreSets.empty()) {
    Logger::Log("[ScoreCalculator] No score sets available");
    return result;
  }

  // 最もスコアが高いセットを採用
  ScoreResult bestResult;
  float bestScore = -1.0f;

  for (const auto &scoreSet : theme.scoreSets) {
    ScoreResult tempResult;

    CalcPartScores(scoreSet, playerData, tempResult);
    CalcCountScores(scoreSet, playerData, tempResult);
    CalcBonusScores(theme, playerData, tempResult, tempResult);

    // 最終スコア
    tempResult.finalScore = TruncateToFirstDecimal(
        tempResult.totalPartScore + tempResult.totalCountScore +
        tempResult.totalBonusScore);

    if (tempResult.finalScore > bestScore) {
      bestScore = tempResult.finalScore;
      bestResult = tempResult;
    }
  }

  result = bestResult;

  // ログ出力
  LogScoreDetails(theme, result);

  return result;
}

/*   パーツごとの基本スコア計算   */
void ScoreCalculator::CalcPartScores(const ScoreSet &scoreSet,
                                     const ModBodyCustomizeData &playerData,
                                     ScoreResult &result) {
  float total = 0.0f;

  for (size_t i = 0; i < kPartCount; ++i) {
    const PartWeight &weight = scoreSet.partWeights[i];
    const ModBodyPartParam &param = playerData.partParams[i];

    float score = 0.0f;

    // X 成分
    float deltaX = param.scale.x - 1.0f;
    if (weight.scaleX > 0.0f) {
      score += std::max(0.0f, deltaX) * weight.scaleX;
    } else if (weight.scaleX < 0.0f) {
      score += std::max(0.0f, -deltaX) * std::abs(weight.scaleX);
    }

    // Y 成分
    float deltaY = param.scale.y - 1.0f;
    if (weight.scaleY > 0.0f) {
      score += std::max(0.0f, deltaY) * weight.scaleY;
    } else if (weight.scaleY < 0.0f) {
      score += std::max(0.0f, -deltaY) * std::abs(weight.scaleY);
    }

    // Z 成分
    float deltaZ = param.scale.z - 1.0f;
    if (weight.scaleZ > 0.0f) {
      score += std::max(0.0f, deltaZ) * weight.scaleZ;
    } else if (weight.scaleZ < 0.0f) {
      score += std::max(0.0f, -deltaZ) * std::abs(weight.scaleZ);
    }

    result.partScores[i] = TruncateToFirstDecimal(score);
    total += result.partScores[i];
  }

  result.totalPartScore = TruncateToFirstDecimal(total);
}

/*   本数スコア計算   */
void ScoreCalculator::CalcCountScores(const ScoreSet &scoreSet,
                                      const ModBodyCustomizeData &playerData,
                                      ScoreResult &result) {
  // CountPart と ModBodyPart の対応
  // LeftArm  → LeftUpperArm の count を代表として使う
  // RightArm → RightUpperArm
  // LeftLeg  → LeftThigh
  // RightLeg → RightThigh
  const size_t countPartIndices[] = {
      static_cast<size_t>(ModBodyPart::LeftUpperArm),
      static_cast<size_t>(ModBodyPart::RightUpperArm),
      static_cast<size_t>(ModBodyPart::LeftThigh),
      static_cast<size_t>(ModBodyPart::RightThigh),
  };

  float total = 0.0f;

  for (size_t i = 0; i < kCountPartCount; ++i) {
    const CountWeight &cw = scoreSet.countWeights[i];
    float count =
        static_cast<float>(playerData.partParams[countPartIndices[i]].count);

    float rawScore = count * cw.weight;

    // invert: 正規化して反転
    if (cw.invert) {
      // count が多いほど低スコアにする
      // 上限を 10 として正規化（仮の上限値）
      constexpr float kMaxCount = 10.0f;
      float normalized = std::min(count / kMaxCount, 1.0f);
      rawScore = (1.0f - normalized) * cw.weight;
    }

    result.countScores[i] = TruncateToFirstDecimal(rawScore);
    total += result.countScores[i];
  }

  result.totalCountScore = TruncateToFirstDecimal(total);
}

/*   ボーナススコア計算   */
void ScoreCalculator::CalcBonusScores(const ThemeData &theme,
                                      const ModBodyCustomizeData &playerData,
                                      const ScoreResult &partResult,
                                      ScoreResult &result) {
  // score_sets の最初のセットをワイルドカード計算に使用
  const ScoreSet &scoreSet = theme.scoreSets[0];

  float rawValues[kBonusCount];
  rawValues[static_cast<size_t>(BonusType::Symmetry)] =
      CalcSymmetryRaw(playerData);
  rawValues[static_cast<size_t>(BonusType::CostEfficiency)] =
      CalcCostEfficiencyRaw(playerData, partResult.totalPartScore);
  rawValues[static_cast<size_t>(BonusType::Minimalist)] =
      CalcMinimalistRaw(playerData);
  rawValues[static_cast<size_t>(BonusType::Wildcard)] =
      CalcWildcardRaw(scoreSet, playerData);
  rawValues[static_cast<size_t>(BonusType::Balance)] =
      CalcBalanceRaw(playerData);

  float total = 0.0f;

  for (size_t i = 0; i < kBonusCount; ++i) {
    float raw = rawValues[i];
    const BonusWeight &bw = theme.bonusWeights[i];

    // invert 時は反転
    if (bw.invert) {
      raw = 1.0f - raw;
    }

    float score = raw * bw.weight;
    result.bonusScores[i] = TruncateToFirstDecimal(score);
    total += result.bonusScores[i];
  }

  result.totalBonusScore = TruncateToFirstDecimal(total);
}

/*   シンメトリーボーナス（0〜1）   */
float ScoreCalculator::CalcSymmetryRaw(const ModBodyCustomizeData &playerData) {
  constexpr size_t pairCount = sizeof(kSymmetryPairs) / sizeof(kSymmetryPairs[0]);
  constexpr float kMaxDiffPerAxis = 2.0f; // この差以上で実質 0 点

  float totalSimilarity = 0.0f;

  for (size_t p = 0; p < pairCount; ++p) {
    const ModBodyPartParam &left = playerData.partParams[kSymmetryPairs[p].left];
    const ModBodyPartParam &right =
        playerData.partParams[kSymmetryPairs[p].right];

    // 各軸の差の絶対値
    float diffX = std::abs(left.scale.x - right.scale.x);
    float diffY = std::abs(left.scale.y - right.scale.y);
    float diffZ = std::abs(left.scale.z - right.scale.z);

    // 各軸を 0〜1 に正規化（差が kMaxDiffPerAxis 以上で 0）
    float simX = std::max(0.0f, 1.0f - diffX / kMaxDiffPerAxis);
    float simY = std::max(0.0f, 1.0f - diffY / kMaxDiffPerAxis);
    float simZ = std::max(0.0f, 1.0f - diffZ / kMaxDiffPerAxis);

    // 3軸の平均
    totalSimilarity += (simX + simY + simZ) / 3.0f;
  }

  // 全ペアの平均
  return totalSimilarity / static_cast<float>(pairCount);
}

/*   コスパボーナス（0〜1）   */
float ScoreCalculator::CalcCostEfficiencyRaw(
    const ModBodyCustomizeData &playerData, float totalPartScore) {
  // 全パーツの変化量合計
  float totalChange = 0.0f;
  for (size_t i = 0; i < kPartCount; ++i) {
    totalChange += GetPartChangeAmount(playerData.partParams[i]);
  }

  // 変化量が 0 の場合（何も改造していない）
  if (totalChange < 0.001f) {
    return (totalPartScore > 0.0f) ? 1.0f : 0.5f;
  }

  // 効率 = 基本スコア / 変化量合計
  float efficiency = totalPartScore / totalChange;

  // 効率を 0〜1 に正規化（効率 5.0 以上で 1.0）
  constexpr float kMaxEfficiency = 5.0f;
  return std::min(efficiency / kMaxEfficiency, 1.0f);
}

/*   ミニマリストボーナス（0〜1）   */
float ScoreCalculator::CalcMinimalistRaw(
    const ModBodyCustomizeData &playerData) {
  int unchangedCount = 0;

  for (size_t i = 0; i < kPartCount; ++i) {
    if (!IsPartChanged(playerData.partParams[i])) {
      ++unchangedCount;
    }
  }

  return static_cast<float>(unchangedCount) / static_cast<float>(kPartCount);
}

/*   ワイルドカードボーナス（0〜1）   */
float ScoreCalculator::CalcWildcardRaw(const ScoreSet &scoreSet,
                                       const ModBodyCustomizeData &playerData) {
  // 倍率が低い（全成分の絶対値合計が小さい）パーツの変化量を集める
  constexpr float kLowWeightThreshold = 1.0f; // 倍率合計がこれ以下なら「低倍率」

  float totalWildChange = 0.0f;
  int lowWeightPartCount = 0;

  for (size_t i = 0; i < kPartCount; ++i) {
    const PartWeight &pw = scoreSet.partWeights[i];
    float weightSum =
        std::abs(pw.scaleX) + std::abs(pw.scaleY) + std::abs(pw.scaleZ);

    if (weightSum <= kLowWeightThreshold) {
      float change = GetPartChangeAmount(playerData.partParams[i]);
      totalWildChange += change;
      ++lowWeightPartCount;
    }
  }

  if (lowWeightPartCount == 0) {
    return 0.0f;
  }

  // 変化量の平均を 0〜1 に正規化（平均変化量 2.0 以上で 1.0）
  float avgChange = totalWildChange / static_cast<float>(lowWeightPartCount);
  constexpr float kMaxWildChange = 2.0f;
  return std::min(avgChange / kMaxWildChange, 1.0f);
}

/*   バランスボーナス（0〜1）   */
float ScoreCalculator::CalcBalanceRaw(const ModBodyCustomizeData &playerData) {
  // 全パーツの変化量を集める
  float changes[kPartCount];
  float sum = 0.0f;

  for (size_t i = 0; i < kPartCount; ++i) {
    changes[i] = GetPartChangeAmount(playerData.partParams[i]);
    sum += changes[i];
  }

  float mean = sum / static_cast<float>(kPartCount);

  // 分散を計算
  float variance = 0.0f;
  for (size_t i = 0; i < kPartCount; ++i) {
    float diff = changes[i] - mean;
    variance += diff * diff;
  }
  variance /= static_cast<float>(kPartCount);

  // 分散を 0〜1 に正規化（分散 4.0 以上で実質 0 点）
  constexpr float kMaxVariance = 4.0f;
  return std::max(0.0f, 1.0f - variance / kMaxVariance);
}

/*   スコア詳細をログ出力   */
void ScoreCalculator::LogScoreDetails(const ThemeData &theme,
                                      const ScoreResult &result) {
  Logger::Log("========================================");
  Logger::Log("[ScoreCalculator] Theme: %s", theme.themeName.c_str());
  Logger::Log("========================================");

  // パーツスコア
  Logger::Log("--- Part Scores ---");
  for (size_t i = 0; i < kPartCount; ++i) {
    Logger::Log("  %s = %.1f", kPartNames[i], result.partScores[i]);
  }
  Logger::Log("  totalPartScore = %.1f", result.totalPartScore);

  // 本数スコア
  Logger::Log("--- Count Scores ---");
  for (size_t i = 0; i < kCountPartCount; ++i) {
    Logger::Log("  %s = %.1f", kCountNames[i], result.countScores[i]);
  }
  Logger::Log("  totalCountScore = %.1f", result.totalCountScore);

  // ボーナススコア
  Logger::Log("--- Bonus Scores ---");
  for (size_t i = 0; i < kBonusCount; ++i) {
    Logger::Log("  %s = %.1f", kBonusNames[i], result.bonusScores[i]);
  }
  Logger::Log("  totalBonusScore = %.1f", result.totalBonusScore);

  // 最終スコア
  Logger::Log("========================================");
  Logger::Log("  finalScore = %.1f", result.finalScore);
  Logger::Log("========================================");
}
