#pragma once
#include "NicknameData.h"
#include "GAME/score/ScoreResult.h"
#include "GAME/userData/UserData.h"
#include "externals/nlohmann/json.hpp"
#include "GAME/score/ScoreCalculator.h"
#include <string>

/// 二つ名の生成・レア判定を行うクラス
/// 内部状態を持たず、すべて static 関数で提供する
class NicknameManager {
public:
  /// 二つ名テーブルを JSON から読み込む
  /// @param filePath nicknames.json のパス
  /// @param outData 読み込み先
  /// @return 成功時は true
  static bool LoadNicknameTable(const std::string &filePath,
                                NicknameTableData &outData);

  /// スコア結果から二つ名を生成する
  /// レア条件を優先度順にチェックし、該当すればレア二つ名を返す
  /// 該当しなければ通常二つ名を生成する
  /// @param table 二つ名テーブル
  /// @param result スコア結果
  /// @param userData ユーザーデータ（初取得判定・重複チェック用）
  /// @param playerData プレイヤーの改造データ（改造パーツ数判定用）
  /// @return 生成された EarnedNickname
  static EarnedNickname GenerateNickname(
      const NicknameTableData &table, const ScoreResult &result,
      const UserData &userData, const ModBodyCustomizeData &playerData);

private:
  // インスタンス化禁止
  NicknameManager() = delete;

  /// レア二つ名の条件をチェックする
  /// @return 該当するレア二つ名があればそのポインタ、なければ nullptr
  static const RareNicknameData *CheckRareNickname(
      const NicknameTableData &table, const ScoreResult &result,
      const UserData &userData, const ModBodyCustomizeData &playerData);

  /// 単一のレア二つ名の条件を判定する
  static bool EvaluateRareCondition(const RareNicknameData &rare,
                                    const ScoreResult &result,
                                    const UserData &userData,
                                    const ModBodyCustomizeData &playerData);

  /// 通常二つ名を生成する
  static std::string GenerateNormalNickname(const NicknameTableData &table,
                                            const ScoreResult &result);

  /// ★項目名から ScoreResult の★値を取得する
  static int GetStarByName(const ScoreResult &result,
                           const std::string &starName);

  /// 改造されたパーツ数を取得する
  static int GetChangedPartCount(const ModBodyCustomizeData &playerData);

  /// ランクの強さを数値に変換する（比較用）
  static int RankToValue(const std::string &rank);

  /// 優先度文字列を enum に変換する
  static RareNicknamePriority ParsePriority(const std::string &str);

  /// 条件タイプ文字列を enum に変換する
  static RareConditionType ParseConditionType(const std::string &str);

  /// JSON からレア二つ名 1 件をパースする
  static bool ParseRareNickname(const nlohmann::json &json,
                                RareNicknameData &outData);

  /// ユーザーが指定ランクを過去に取得したことがあるか
  static bool HasAchievedRank(const UserData &userData,
                              const std::string &rank);

  /// ユーザーが指定の二つ名を既に持っているか
  static bool HasNickname(const UserData &userData,
                          const std::string &nickname);
};
