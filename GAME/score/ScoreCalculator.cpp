#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "ScoreCalculator.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace {

	/// パーツ名テーブル（ログ表示用）
	const char* kPartNames[] = {
		"ChestBody",   "StomachBody",    "Neck",         "Head",
		"LeftUpperArm", "LeftForeArm",   "RightUpperArm", "RightForeArm",
		"LeftThigh",    "LeftShin",      "RightThigh",   "RightShin",
	};

	/// カウント名テーブル（ログ表示用）
	const char* kCountNames[] = {
		"LeftArm",
		"RightArm",
		"LeftLeg",
		"RightLeg",
	};

	/// ボーナス名テーブル（ログ表示用）
	const char* kBonusNames[] = {
		"Symmetry",
		"CostEfficiency",
		"Minimalist",
		"Wildcard",
		"Balance",
	};

	/// ★項目名テーブル（ログ表示用）
	const char* kStarNames[] = {
		"ThemeMatch",
		"Impact",
		"Efficiency",
		"Commitment",
		"JudgeBonus",
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

	// ===== ★変換の閾値（後から調整可能） =====

	// テーマ一致度：パーツスコア合計 → ★
	constexpr float kThemeMatchThresholds[] = { 0.0f, 3.0f, 6.0f, 10.0f, 15.0f };

	// インパクト：全体変化量 → ★
	constexpr float kImpactThresholds[] = { 0.0f, 2.0f, 5.0f, 9.0f, 14.0f };

	// 効率：正規化値（0〜1）の平均 → ★
	constexpr float kEfficiencyThresholds[] = { 0.0f, 0.2f, 0.4f, 0.6f, 0.8f };

	// こだわり：最大変化量 → ★
	constexpr float kCommitmentThresholds[] = { 0.0f, 1.0f, 2.0f, 3.5f, 5.0f };

	/// 閾値テーブルから★を決定する（四捨五入考慮）
	int ThresholdToStar(float value, const float thresholds[5]) {
		// 各閾値の中間点で四捨五入的に判定
		for (int i = 4; i >= 0; --i) {
			if (value >= thresholds[i]) {
				// 次の閾値との中間を超えていれば i+1（上限5）
				if (i < 4) {
					float mid = (thresholds[i] + thresholds[i + 1]) * 0.5f;
					if (value >= mid) {
						return std::min(i + 2, 5);
					}
				}
				return i + 1;
			}
		}
		return 1;
	}

	/// 1パーツの変化量合計（|x-1| + |y-1| + |z-1|）
	float GetPartChangeAmount(const ModBodyPartParam& param) {
		return std::abs(param.scale.x - 1.0f) + std::abs(param.scale.y - 1.0f) +
			std::abs(param.scale.z - 1.0f);
	}

	/// パーツが変化しているか
	bool IsPartChanged(const ModBodyPartParam& param) {
		constexpr float kEpsilon = 0.001f;
		return std::abs(param.scale.x - 1.0f) > kEpsilon ||
			std::abs(param.scale.y - 1.0f) > kEpsilon ||
			std::abs(param.scale.z - 1.0f) > kEpsilon;
	}

} // namespace

/*   ★を 1〜5 にクランプ   */
int ScoreCalculator::ClampStar(int value) {
	return std::max(1, std::min(5, value));
}

/*   小数点第二位以下を切り捨て   */
float ScoreCalculator::TruncateToFirstDecimal(float value) {
	return std::floor(value * 10.0f) / 10.0f;
}

/*   メイン計算関数   */
ScoreResult ScoreCalculator::Calculate(const ThemeData& theme,
	const ModBodyCustomizeData& playerData,
	const std::vector<JudgeData>& judges) {
	ScoreResult result;

	if (theme.scoreSets.empty()) {
		Logger::Log("[ScoreCalculator] No score sets available");
		return result;
	}

	// 最もスコアが高いセットを採用
	ScoreResult bestResult;
	float bestScore = -1.0f;

	for (const auto& scoreSet : theme.scoreSets) {
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

	// ===== ★評価の計算 =====

	// テーマ一致度
	result.starThemeMatch = CalcStarThemeMatch(result.totalPartScore);

	// インパクト
	result.starImpact = CalcStarImpact(playerData);

	// 効率
	float costRaw = CalcCostEfficiencyRaw(playerData, result.totalPartScore);
	float miniRaw = CalcMinimalistRaw(playerData);
	result.starEfficiency = CalcStarEfficiency(costRaw, miniRaw);

	// こだわり
	result.starCommitment = CalcStarCommitment(playerData);

	// 審査員ボーナス
	if (!judges.empty()) {
		result.starJudgeBonus = CalcStarJudgeBonus(judges, playerData, result);
	} else {
		result.starJudgeBonus = 3; // 審査員なしの場合は基準値
	}

	// ★合計
	result.totalStars = result.starThemeMatch + result.starImpact +
		result.starEfficiency + result.starCommitment +
		result.starJudgeBonus;

	// 総合ランク
	result.overallRank = DetermineOverallRank(result.totalStars);

	// ログ出力
	LogScoreDetails(theme, result);

	return result;
}

float ScoreCalculator::GetPartChangeAmountFromCP(
	ModBodyPart partType, const ModBodyCustomizeData& playerData)
{
	Vector3 scale = CalcPartChangeFromControlPoints(partType, playerData);
	return std::abs(scale.x - 1.0f) + std::abs(scale.y - 1.0f) +
		std::abs(scale.z - 1.0f);
}

bool ScoreCalculator::IsPartChangedFromCP(ModBodyPart partType, const ModBodyCustomizeData& playerData)
{
	constexpr float kEpsilon = 0.01f;
	Vector3 scale = CalcPartChangeFromControlPoints(partType, playerData);
	return std::abs(scale.x - 1.0f) > kEpsilon ||
		std::abs(scale.y - 1.0f) > kEpsilon ||
		std::abs(scale.z - 1.0f) > kEpsilon;
}

/*   操作点から部位ごとの変化量（scale相当）を計算する   */
Vector3 ScoreCalculator::CalcPartChangeFromControlPoints(
	ModBodyPart partType,
	const ModBodyCustomizeData& playerData) {

	// 部位タイプごとに、対応するroleを定義する
	// owner部位: チェーンの最初のセグメント（role[0]→role[1]）
	// 子部位: チェーンの2番目のセグメント（role[1]→role[2]）

	struct SegmentDef {
		ModBodyPart ownerType;           // snapshotのownerPartType
		ModControlPointRole startRole;   // セグメント開始点のrole
		ModControlPointRole endRole;     // セグメント終了点のrole
		ModControlPointRole radiusRole1; // radius計算に使うrole1
		ModControlPointRole radiusRole2; // radius計算に使うrole2
	};

	SegmentDef segment;
	bool found = false;

	switch (partType) {
		// 胴体: Chest→Belly
	case ModBodyPart::ChestBody:
		segment = { ModBodyPart::ChestBody, ModControlPointRole::Chest,
				   ModControlPointRole::Belly, ModControlPointRole::Chest,
				   ModControlPointRole::Belly };
		found = true;
		Logger::Log("[ChestDebug] looking for ownerType=%d startRole=%d endRole=%d",
			static_cast<int>(ModBodyPart::ChestBody),
			static_cast<int>(ModControlPointRole::Chest),
			static_cast<int>(ModControlPointRole::Belly));
		break;

		// 腹: Belly→Waist
	case ModBodyPart::StomachBody:
		segment = { ModBodyPart::ChestBody, ModControlPointRole::Belly,
				   ModControlPointRole::Waist, ModControlPointRole::Belly,
				   ModControlPointRole::Waist };
		found = true;
		break;

	    // 首: Root→Bend
        case ModBodyPart::Neck:
          segment = {ModBodyPart::Neck, ModControlPointRole::Root,
                     ModControlPointRole::Bend, ModControlPointRole::Root,
                     ModControlPointRole::Bend};
          found = true;
          break;

          // 頭: Bend→End
        case ModBodyPart::Head:
          segment = {ModBodyPart::Neck, ModControlPointRole::Bend,
                     ModControlPointRole::End, ModControlPointRole::Bend,
                     ModControlPointRole::End};
          found = true;
          break;

		// 左上腕: Root→Bend
	case ModBodyPart::LeftUpperArm:
		segment = { ModBodyPart::LeftUpperArm, ModControlPointRole::Root,
				   ModControlPointRole::Bend, ModControlPointRole::Root,
				   ModControlPointRole::Bend };
		found = true;
		break;

		// 左前腕: Bend→End
	case ModBodyPart::LeftForeArm:
		segment = { ModBodyPart::LeftUpperArm, ModControlPointRole::Bend,
				   ModControlPointRole::End, ModControlPointRole::Bend,
				   ModControlPointRole::End };
		found = true;
		break;

		// 右上腕: Root→Bend
	case ModBodyPart::RightUpperArm:
		segment = { ModBodyPart::RightUpperArm, ModControlPointRole::Root,
				   ModControlPointRole::Bend, ModControlPointRole::Root,
				   ModControlPointRole::Bend };
		found = true;
		break;

		// 右前腕: Bend→End
	case ModBodyPart::RightForeArm:
		segment = { ModBodyPart::RightUpperArm, ModControlPointRole::Bend,
				   ModControlPointRole::End, ModControlPointRole::Bend,
				   ModControlPointRole::End };
		found = true;
		break;

		// 左腿: Root→Bend
	case ModBodyPart::LeftThigh:
		segment = { ModBodyPart::LeftThigh, ModControlPointRole::Root,
				   ModControlPointRole::Bend, ModControlPointRole::Root,
				   ModControlPointRole::Bend };
		found = true;
		break;

		// 左脛: Bend→End
	case ModBodyPart::LeftShin:
		segment = { ModBodyPart::LeftThigh, ModControlPointRole::Bend,
				   ModControlPointRole::End, ModControlPointRole::Bend,
				   ModControlPointRole::End };
		found = true;
		break;

		// 右腿: Root→Bend
	case ModBodyPart::RightThigh:
		segment = { ModBodyPart::RightThigh, ModControlPointRole::Root,
				   ModControlPointRole::Bend, ModControlPointRole::Root,
				   ModControlPointRole::Bend };
		found = true;
		break;

		// 右脛: Bend→End
	case ModBodyPart::RightShin:
		segment = { ModBodyPart::RightThigh, ModControlPointRole::Bend,
				   ModControlPointRole::End, ModControlPointRole::Bend,
				   ModControlPointRole::End };
		found = true;
		break;

	default:
		break;
	}

	if (!found) {
		return { 1.0f, 1.0f, 1.0f };
	}

	// ownerTypeに属するsnapshotからroleで操作点を探すヘルパー
	auto findByRole = [](const std::vector<ModControlPointSnapshot>& snapshots,
		ModBodyPart ownerType,
		ModControlPointRole role) -> const ModControlPointSnapshot* {
			for (size_t i = 0; i < snapshots.size(); ++i) {
				if (snapshots[i].ownerPartType == ownerType &&
					snapshots[i].role == role) {
					return &snapshots[i];
				}
			}
			return nullptr;
	};

	// 現在とデフォルトの操作点を取得する
	const auto* curStart = findByRole(playerData.controlPointSnapshots,
		segment.ownerType, segment.startRole);
	const auto* curEnd = findByRole(playerData.controlPointSnapshots,
		segment.ownerType, segment.endRole);
	const auto* defStart = findByRole(playerData.defaultControlPointSnapshots,
		segment.ownerType, segment.startRole);
	const auto* defEnd = findByRole(playerData.defaultControlPointSnapshots,
		segment.ownerType, segment.endRole);

	if (curStart == nullptr || curEnd == nullptr ||
		defStart == nullptr || defEnd == nullptr) {
		return { 1.0f, 1.0f, 1.0f };
	}

	// 長さの変化率（start→endの距離）
	float defLen = Length(Subtract(defEnd->localPosition, defStart->localPosition));
	float curLen = Length(Subtract(curEnd->localPosition, curStart->localPosition));
	float lengthRatio = (defLen > 0.001f) ? curLen / defLen : 1.0f;

	// 太さの変化率（radiusの平均変化率）
	const auto* curR1 = findByRole(playerData.controlPointSnapshots,
		segment.ownerType, segment.radiusRole1);
	const auto* curR2 = findByRole(playerData.controlPointSnapshots,
		segment.ownerType, segment.radiusRole2);
	const auto* defR1 = findByRole(playerData.defaultControlPointSnapshots,
		segment.ownerType, segment.radiusRole1);
	const auto* defR2 = findByRole(playerData.defaultControlPointSnapshots,
		segment.ownerType, segment.radiusRole2);

	float radiusRatio = 1.0f;
	int rc = 0;
	float rSum = 0.0f;

	if (curR1 != nullptr && defR1 != nullptr && defR1->radius > 0.001f) {
		rSum += curR1->radius / defR1->radius;
		++rc;
	}
	if (curR2 != nullptr && defR2 != nullptr && defR2->radius > 0.001f) {
		rSum += curR2->radius / defR2->radius;
		++rc;
	}
	if (rc > 0) {
		radiusRatio = rSum / static_cast<float>(rc);
	}

	return { radiusRatio, lengthRatio, radiusRatio };
}
/*   パーツごとの基本スコア計算   */
void ScoreCalculator::CalcPartScores(const ScoreSet& scoreSet,
	const ModBodyCustomizeData& playerData,
	ScoreResult& result) {
	float total = 0.0f;

	for (size_t i = 0; i < kPartCount; ++i) {
		const PartWeight& weight = scoreSet.partWeights[i];
		ModBodyPart partType = static_cast<ModBodyPart>(i);

		// 操作点から変化量を計算する
		Vector3 scale = CalcPartChangeFromControlPoints(partType, playerData);

		float score = 0.0f;

		// X 成分（太さ）
		float deltaX = scale.x - 1.0f;
		if (weight.scaleX > 0.0f) {
			score += std::max(0.0f, deltaX) * weight.scaleX;
		} else if (weight.scaleX < 0.0f) {
			score += std::max(0.0f, -deltaX) * std::abs(weight.scaleX);
		}

		// Y 成分（長さ）
		float deltaY = scale.y - 1.0f;
		if (weight.scaleY > 0.0f) {
			score += std::max(0.0f, deltaY) * weight.scaleY;
		} else if (weight.scaleY < 0.0f) {
			score += std::max(0.0f, -deltaY) * std::abs(weight.scaleY);
		}

		// Z 成分（太さ）
		float deltaZ = scale.z - 1.0f;
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
void ScoreCalculator::CalcCountScores(const ScoreSet& scoreSet,
	const ModBodyCustomizeData& playerData,
	ScoreResult& result) {
	const size_t countPartIndices[] = {
		static_cast<size_t>(ModBodyPart::LeftUpperArm),
		static_cast<size_t>(ModBodyPart::RightUpperArm),
		static_cast<size_t>(ModBodyPart::LeftThigh),
		static_cast<size_t>(ModBodyPart::RightThigh),
	};

	float total = 0.0f;

	for (size_t i = 0; i < kCountPartCount; ++i) {
		const CountWeight& cw = scoreSet.countWeights[i];
		float count =
			static_cast<float>(playerData.partParams[countPartIndices[i]].count);

		float rawScore = count * cw.weight;

		if (cw.invert) {
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
void ScoreCalculator::CalcBonusScores(const ThemeData& theme,
	const ModBodyCustomizeData& playerData,
	const ScoreResult& partResult,
	ScoreResult& result) {
	const ScoreSet& scoreSet = theme.scoreSets[0];

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
		const BonusWeight& bw = theme.bonusWeights[i];

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
float ScoreCalculator::CalcSymmetryRaw(
	const ModBodyCustomizeData& playerData) {
	constexpr float kMaxDiffPerAxis = 2.0f;

	struct SymmetryPairType {
		ModBodyPart left;
		ModBodyPart right;
	};

	const SymmetryPairType pairs[] = {
		{ModBodyPart::LeftUpperArm, ModBodyPart::RightUpperArm},
		{ModBodyPart::LeftForeArm, ModBodyPart::RightForeArm},
		{ModBodyPart::LeftThigh, ModBodyPart::RightThigh},
		{ModBodyPart::LeftShin, ModBodyPart::RightShin},
	};

	constexpr size_t pairCount = sizeof(pairs) / sizeof(pairs[0]);
	float totalSimilarity = 0.0f;

	for (size_t p = 0; p < pairCount; ++p) {
		Vector3 leftScale = CalcPartChangeFromControlPoints(pairs[p].left, playerData);
		Vector3 rightScale = CalcPartChangeFromControlPoints(pairs[p].right, playerData);

		float diffX = std::abs(leftScale.x - rightScale.x);
		float diffY = std::abs(leftScale.y - rightScale.y);
		float diffZ = std::abs(leftScale.z - rightScale.z);

		float simX = std::max(0.0f, 1.0f - diffX / kMaxDiffPerAxis);
		float simY = std::max(0.0f, 1.0f - diffY / kMaxDiffPerAxis);
		float simZ = std::max(0.0f, 1.0f - diffZ / kMaxDiffPerAxis);

		totalSimilarity += (simX + simY + simZ) / 3.0f;
	}

	return totalSimilarity / static_cast<float>(pairCount);
}

/*   コスパボーナス（0〜1）   */
float ScoreCalculator::CalcCostEfficiencyRaw(
	const ModBodyCustomizeData& playerData, float totalPartScore) {
	float totalChange = 0.0f;
	for (size_t i = 0; i < kPartCount; ++i) {
		totalChange += GetPartChangeAmountFromCP(
			static_cast<ModBodyPart>(i), playerData);
	}

	if (totalChange < 0.001f) {
		return (totalPartScore > 0.0f) ? 1.0f : 0.5f;
	}

	float efficiency = totalPartScore / totalChange;

	constexpr float kMaxEfficiency = 5.0f;
	return std::min(efficiency / kMaxEfficiency, 1.0f);
}

/*   ミニマリストボーナス（0〜1）   */
float ScoreCalculator::CalcMinimalistRaw(
	const ModBodyCustomizeData& playerData) {
	int unchangedCount = 0;

	for (size_t i = 0; i < kPartCount; ++i) {
		if (!IsPartChangedFromCP(static_cast<ModBodyPart>(i), playerData)) {
			++unchangedCount;
		}
	}

	return static_cast<float>(unchangedCount) / static_cast<float>(kPartCount);
}

/*   ワイルドカードボーナス（0〜1）   */
float ScoreCalculator::CalcWildcardRaw(
	const ScoreSet& scoreSet, const ModBodyCustomizeData& playerData) {
	constexpr float kLowWeightThreshold = 1.0f;

	float totalWildChange = 0.0f;
	int lowWeightPartCount = 0;

	for (size_t i = 0; i < kPartCount; ++i) {
		const PartWeight& pw = scoreSet.partWeights[i];
		float weightSum =
			std::abs(pw.scaleX) + std::abs(pw.scaleY) + std::abs(pw.scaleZ);

		if (weightSum <= kLowWeightThreshold) {
			float change = GetPartChangeAmountFromCP(
				static_cast<ModBodyPart>(i), playerData);
			totalWildChange += change;
			++lowWeightPartCount;
		}
	}

	if (lowWeightPartCount == 0) {
		return 0.0f;
	}

	float avgChange = totalWildChange / static_cast<float>(lowWeightPartCount);
	constexpr float kMaxWildChange = 2.0f;
	return std::min(avgChange / kMaxWildChange, 1.0f);
}

/*   バランスボーナス（0〜1）   */
float ScoreCalculator::CalcBalanceRaw(
	const ModBodyCustomizeData& playerData) {
	float changes[kPartCount];
	float sum = 0.0f;

	for (size_t i = 0; i < kPartCount; ++i) {
		changes[i] = GetPartChangeAmountFromCP(
			static_cast<ModBodyPart>(i), playerData);
		sum += changes[i];
	}

	float mean = sum / static_cast<float>(kPartCount);

	float variance = 0.0f;
	for (size_t i = 0; i < kPartCount; ++i) {
		float diff = changes[i] - mean;
		variance += diff * diff;
	}
	variance /= static_cast<float>(kPartCount);

	constexpr float kMaxVariance = 4.0f;
	return std::max(0.0f, 1.0f - variance / kMaxVariance);
}

// ===== ★評価関連 =====

/*   テーマ一致度の★   */
int ScoreCalculator::CalcStarThemeMatch(float totalPartScore) {
	return ClampStar(ThresholdToStar(totalPartScore, kThemeMatchThresholds));
}

/*   インパクトの★   */
int ScoreCalculator::CalcStarImpact(const ModBodyCustomizeData& playerData) {
	float totalChange = 0.0f;
	for (size_t i = 0; i < kPartCount; ++i) {
		totalChange += GetPartChangeAmountFromCP(
			static_cast<ModBodyPart>(i), playerData);
	}
	return ClampStar(ThresholdToStar(totalChange, kImpactThresholds));
}

/*   効率の★   */
int ScoreCalculator::CalcStarEfficiency(float costEfficiencyRaw,
	float minimalistRaw) {
	float avg = (costEfficiencyRaw + minimalistRaw) * 0.5f;
	return ClampStar(ThresholdToStar(avg, kEfficiencyThresholds));
}

/*   こだわりの★   */
int ScoreCalculator::CalcStarCommitment(
	const ModBodyCustomizeData& playerData) {
	float maxChange = 0.0f;
	for (size_t i = 0; i < kPartCount; ++i) {
		float change = GetPartChangeAmountFromCP(
			static_cast<ModBodyPart>(i), playerData);
		if (change > maxChange) {
			maxChange = change;
		}
	}
	return ClampStar(ThresholdToStar(maxChange, kCommitmentThresholds));
}

/*   審査員 1 人分の★計算   */
int ScoreCalculator::CalcSingleJudgeStar(
	const JudgeData& judge, const ModBodyCustomizeData& playerData) {
	// 基準★3
	// 好みパーツ（正の閾値）：上限以上→★5、改造あり→★4、改造なし→★3
	// 嫌いパーツ（負の閾値）：上限以上→★3、改造あり→★2、改造なし→★3

	std::vector<int> partStars;

	for (size_t i = 0; i < 12; ++i) {
		float threshold = judge.partPreferences[i].threshold;

		if (std::abs(threshold) < 0.001f) {
			continue;
		}

		ModBodyPart partType = static_cast<ModBodyPart>(i);
		float changeAmount = GetPartChangeAmountFromCP(partType, playerData);
		bool isChanged = IsPartChangedFromCP(partType, playerData);

		if (threshold > 0.0f) {
			if (changeAmount >= threshold) {
				partStars.push_back(5);
			} else if (isChanged) {
				partStars.push_back(4);
			} else {
				partStars.push_back(3);
			}
		} else {
			float absThreshold = std::abs(threshold);
			if (changeAmount >= absThreshold) {
				partStars.push_back(3);
			} else if (isChanged) {
				partStars.push_back(2);
			} else {
				partStars.push_back(3);
			}
		}
	}

	if (partStars.empty()) {
		return 3;
	}

	float sum = 0.0f;
	for (int s : partStars) {
		sum += static_cast<float>(s);
	}
	float avg = sum / static_cast<float>(partStars.size());
	return ClampStar(static_cast<int>(std::round(avg)));
}

/*   審査員ボーナスの★（3 人の平均を四捨五入）   */
int ScoreCalculator::CalcStarJudgeBonus(
	const std::vector<JudgeData>& judges,
	const ModBodyCustomizeData& playerData, ScoreResult& result) {
	float sum = 0.0f;
	int count = 0;

	for (size_t i = 0; i < judges.size() && i < 3; ++i) {
		int star = CalcSingleJudgeStar(judges[i], playerData);

		result.judgeEvaluations[i].judgeId = judges[i].judgeId;
		result.judgeEvaluations[i].judgeName = judges[i].judgeName;
		result.judgeEvaluations[i].judgeTitle = judges[i].judgeTitle;
		result.judgeEvaluations[i].star = star;

		sum += static_cast<float>(star);
		++count;
	}

	if (count == 0) {
		return 3;
	}

	float avg = sum / static_cast<float>(count);
	return ClampStar(static_cast<int>(std::round(avg)));
}

/*   総合ランク決定   */
std::string ScoreCalculator::DetermineOverallRank(int totalStars) {
	if (totalStars >= 23) return "SS";
	if (totalStars >= 19) return "S";
	if (totalStars >= 15) return "A";
	if (totalStars >= 11) return "B";
	if (totalStars >= 6)  return "C";
	return "D";
}

/*   スコア詳細をログ出力   */
void ScoreCalculator::LogScoreDetails(const ThemeData& theme,
	const ScoreResult& result) {
	Logger::Log("========================================");
	Logger::LogUtf8("[ScoreCalculator] Theme: " + theme.themeName + " (" +
		theme.themeId + ")");
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

	// ★評価
	Logger::Log("--- Star Ratings ---");
	Logger::Log("  ThemeMatch  = %d", result.starThemeMatch);
	Logger::Log("  Impact      = %d", result.starImpact);
	Logger::Log("  Efficiency  = %d", result.starEfficiency);
	Logger::Log("  Commitment  = %d", result.starCommitment);
	Logger::Log("  JudgeBonus  = %d", result.starJudgeBonus);
	Logger::Log("  TotalStars  = %d", result.totalStars);

	// 審査員個別
	Logger::Log("--- Judge Evaluations ---");
	for (size_t i = 0; i < 3; ++i) {
		const auto& je = result.judgeEvaluations[i];
		if (!je.judgeId.empty()) {
			Logger::LogUtf8("  " + je.judgeName + " (" + je.judgeTitle + ") = " +
				std::to_string(je.star));
		}
	}

	// 総合ランク
	Logger::Log("========================================");
	Logger::Log("  Overall Rank = %s", result.overallRank.c_str());
	Logger::Log("  finalScore = %.1f", result.finalScore);
	Logger::Log("========================================");
}
