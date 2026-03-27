#pragma once
#include <array>
#include <string>

/// 審査員の好みパーツの倍率
/// 正の値 = 好き（その方向に改造されていると高評価）
/// 負の値 = 嫌い（その方向に改造されていると低評価）
/// 0.0 = 関心なし
struct JudgePartPreference {
  float threshold = 0.0f; // 正:好みの上限値 負:嫌いの上限値
};

/// 審査員 1 人分のデータ
struct JudgeData {
  std::string judgeId;
  std::string judgeName;
  std::string judgeTitle;   // 二つ名（好みを反映した肩書き）
  std::string texturePath;

  // 各パーツへの好み（12パーツ分）
  // インデックスは ModBodyPart の値をそのまま使用
  std::array<JudgePartPreference, 12> partPreferences{};
};
