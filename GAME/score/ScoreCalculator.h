#pragma once
#include "GAME/actor/ModBody.h"
#include "GAME/score/ScoreResult.h"
#include "GAME/theme/ThemeData.h"

/// スコア計算を行う汎用ユーティリティクラス
/// 内部状態を持たず、すべて static 関数で提供する
class ScoreCalculator {
public:
  /// スコアを計算する
  /// @param theme お題データ
  /// @param playerData プレイヤーの改造データ
  /// @return 計算結果
  static ScoreResult Calculate(const ThemeData &theme,
                               const ModBodyCustomizeData &playerData);

private:
  // インスタンス化禁止
  ScoreCalculator() = delete;

  /// パーツごとの基本スコアを計算する
  static void CalcPartScores(const ScoreSet &scoreSet,
                             const ModBodyCustomizeData &playerData,
                             ScoreResult &result);

  /// 本数スコアを計算する
  static void CalcCountScores(const ScoreSet &scoreSet,
                              const ModBodyCustomizeData &playerData,
                              ScoreResult &result);

  /// ボーナススコアを計算する
  static void CalcBonusScores(const ThemeData &theme,
                              const ModBodyCustomizeData &playerData,
                              const ScoreResult &partResult,
                              ScoreResult &result);

  /// シンメトリーボーナス（0〜1）
  static float CalcSymmetryRaw(const ModBodyCustomizeData &playerData);

  /// コスパボーナス（0〜1）
  static float CalcCostEfficiencyRaw(const ModBodyCustomizeData &playerData,
                                     float totalPartScore);

  /// ミニマリストボーナス（0〜1）
  static float CalcMinimalistRaw(const ModBodyCustomizeData &playerData);

  /// ワイルドカードボーナス（0〜1）
  static float CalcWildcardRaw(const ScoreSet &scoreSet,
                               const ModBodyCustomizeData &playerData);

  /// バランスボーナス（0〜1）
  static float CalcBalanceRaw(const ModBodyCustomizeData &playerData);

  /// スコア詳細をログに出力する
  static void LogScoreDetails(const ThemeData &theme,
                              const ScoreResult &result);

  /// 小数点第二位以下を切り捨てる
  static float TruncateToFirstDecimal(float value);
};
