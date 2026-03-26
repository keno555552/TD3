#pragma once
#include "GAME/actor/ModBody.h"
#include <string>
#include <vector>

/// トロフィーの保存上限
constexpr int kMaxTrophyCount = 50;

/// 取得済み二つ名 1 件分のデータ
struct EarnedNickname {
  std::string nickname;    // 二つ名テキスト
  bool isRare = false;     // レア二つ名かどうか
  std::string earnedTheme; // 取得時のお題 ID
  std::string earnedRank;  // 取得時の総合ランク
};

/// トロフィー 1 件分のデータ（二つ名＋改造データ＋評価）
struct TrophyData {
  EarnedNickname nickname;                // 二つ名
  std::string themeId;                    // お題 ID
  std::string themeName;                  // お題名
  std::string overallRank;                // 総合ランク
  int totalStars = 0;                     // ★合計
  ModBodyCustomizeData customizeData;     // 改造データ（モデル再現用）
};

/// ユーザーデータ
struct UserData {
  std::string playerName = "Player";
  int playCount = 0;
  std::string bestRank = "D";

  // 取得済み二つ名（毎回自動保存）
  std::vector<EarnedNickname> nicknames;

  // トロフィー（プレイヤーが選んで保存、上限 kMaxTrophyCount 個）
  std::vector<TrophyData> trophies;

  /// トロフィーが上限に達しているか
  bool IsTrophyFull() const {
    return static_cast<int>(trophies.size()) >= kMaxTrophyCount;
  }

  /// トロフィーを追加する（上限チェックは呼び出し側で行うこと）
  void AddTrophy(const TrophyData &trophy) { trophies.push_back(trophy); }

  /// 指定インデックスのトロフィーを入れ替える
  void ReplaceTrophy(int index, const TrophyData &trophy) {
    if (index >= 0 && index < static_cast<int>(trophies.size())) {
      trophies[index] = trophy;
    }
  }

  /// 指定インデックスのトロフィーを削除する
  void RemoveTrophy(int index) {
    if (index >= 0 && index < static_cast<int>(trophies.size())) {
      trophies.erase(trophies.begin() + index);
    }
  }
};
