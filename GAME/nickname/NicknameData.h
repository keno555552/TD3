#pragma once
#include <string>
#include <unordered_map>
#include <vector>

/// 通常二つ名の形容詞テーブル（ランクごとの候補リスト）
/// key: ランク文字列（"SS", "S", "A", "B", "C", "D"）
/// value: そのランクで使われる形容詞の候補リスト
using AdjectiveTable = std::unordered_map<std::string, std::vector<std::string>>;

/// 通常二つ名の名詞テーブル（★項目ごとの候補リスト）
/// key: 項目名（"theme_match", "impact", "efficiency", "commitment", "judge_bonus"）
/// value: その項目が最高★のときに使われる名詞の候補リスト
using NounTable = std::unordered_map<std::string, std::vector<std::string>>;

/// レア二つ名の優先度
enum class RareNicknamePriority {
  Highest, // 最高（パーフェクト）
  High,    // 高（ミス・パーフェクト）
  Medium,  // 中（サイボーグ、金色の暴君、ブロンズコレクター）
  Low,     // 低（核融合、一撃必殺、満場一致など）
};

/// レア二つ名の条件タイプ
enum class RareConditionType {
  AllStarsEqual,      // 全項目が指定値と一致
  AllStarsMin,        // 全項目が指定値以上
  FirstRankAchieved,  // 指定ランクを初めて取得
  StarsAndRank,       // 特定の★条件 + 総合ランク条件
  StarsCombo,         // 特定の★条件の組み合わせ
  ChangedPartsMax,    // 改造パーツ数が指定以下
  JudgeStarsAllEqual, // 審査員3人の★が全員同じ
  JudgeStarsDiffMin,  // 審査員の★の最大と最小の差が指定以上
  ClearTimeMax,       // クリアタイムが指定以下（未実装）
};

/// レア二つ名の★条件（StarsAndRank, StarsCombo 用）
struct StarCondition {
  std::string starName; // "theme_match", "impact", "efficiency", "commitment", "judge_bonus"
  int minValue = 0;     // この★が minValue 以上
  int maxValue = 5;     // この★が maxValue 以下
};

/// レア二つ名 1 件分のデータ
struct RareNicknameData {
  std::string rareId;                  // 識別ID
  std::string nickname;                // 二つ名テキスト
  RareNicknamePriority priority = RareNicknamePriority::Low;
  RareConditionType conditionType = RareConditionType::AllStarsMin;

  // 条件パラメータ（条件タイプに応じて使用するものが異なる）
  int intValue = 0;                    // AllStarsEqual, AllStarsMin, ChangedPartsMax, JudgeStarsDiffMin 用
  std::string rankValue;               // FirstRankAchieved, StarsAndRank 用
  std::vector<StarCondition> starConditions; // StarsAndRank, StarsCombo 用
  float floatValue = 0.0f;             // ClearTimeMax 用
};

/// 通常二つ名＋レア二つ名の全データ
struct NicknameTableData {
  AdjectiveTable adjectives; // ランクごとの形容詞候補
  NounTable nouns;           // ★項目ごとの名詞候補
  std::vector<RareNicknameData> rareNicknames; // レア二つ名一覧
};
