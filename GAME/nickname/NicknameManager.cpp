#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "NicknameManager.h"
#include "GAME/json/JsonManager.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace {

constexpr size_t kPartCount = 12;
constexpr float kEpsilon = 0.001f;

/// パーツが変化しているか
bool IsPartChanged(const ModBodyPartParam &param) {
  return std::abs(param.scale.x - 1.0f) > kEpsilon ||
         std::abs(param.scale.y - 1.0f) > kEpsilon ||
         std::abs(param.scale.z - 1.0f) > kEpsilon;
}

/// ランダムで vector からひとつ選ぶ
const std::string &RandomPick(const std::vector<std::string> &list) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dist(0,
                                          static_cast<int>(list.size()) - 1);
  return list[dist(gen)];
}

} // namespace

/*   JSON から二つ名テーブルを読み込む   */
bool NicknameManager::LoadNicknameTable(const std::string &filePath,
                                        NicknameTableData &outData) {
  auto json = JsonManager::Load(filePath);
  if (!json.has_value()) {
    Logger::Log("[NicknameManager] Failed to load: %s", filePath.c_str());
    return false;
  }

  try {
    const auto &j = json.value();

    // 形容詞テーブル
    if (j.contains("adjectives")) {
      for (const auto &[rank, words] : j.at("adjectives").items()) {
        std::vector<std::string> list;
        for (const auto &word : words) {
          list.push_back(word.get<std::string>());
        }
        outData.adjectives[rank] = list;
      }
    }

    // 名詞テーブル
    if (j.contains("nouns")) {
      for (const auto &[category, words] : j.at("nouns").items()) {
        std::vector<std::string> list;
        for (const auto &word : words) {
          list.push_back(word.get<std::string>());
        }
        outData.nouns[category] = list;
      }
    }

    // レア二つ名
    if (j.contains("rare_nicknames")) {
      for (const auto &rareJson : j.at("rare_nicknames")) {
        RareNicknameData rare;
        if (ParseRareNickname(rareJson, rare)) {
          outData.rareNicknames.push_back(std::move(rare));
        }
      }
    }

    // 優先度順にソート（highest が先）
    std::sort(outData.rareNicknames.begin(), outData.rareNicknames.end(),
              [](const RareNicknameData &a, const RareNicknameData &b) {
                return static_cast<int>(a.priority) <
                       static_cast<int>(b.priority);
              });

    Logger::Log("[NicknameManager] Loaded nickname table: %d adjective ranks, "
                "%d noun categories, %d rare nicknames",
                static_cast<int>(outData.adjectives.size()),
                static_cast<int>(outData.nouns.size()),
                static_cast<int>(outData.rareNicknames.size()));

    return true;
  } catch (const nlohmann::json::exception &e) {
    Logger::Log("[NicknameManager] JSON parse error: %s", e.what());
    return false;
  }
}

/*   二つ名を生成する   */
EarnedNickname NicknameManager::GenerateNickname(
    const NicknameTableData &table, const ScoreResult &result,
    const UserData &userData, const ModBodyCustomizeData &playerData) {
  EarnedNickname earned;
  earned.earnedRank = result.overallRank;

  // レア二つ名チェック（優先度順）
  const RareNicknameData *rare =
      CheckRareNickname(table, result, userData, playerData);

  if (rare != nullptr) {
    earned.nickname = rare->nickname;
    earned.isRare = true;
    Logger::LogUtf8("[NicknameManager] Rare nickname earned: " +
                    rare->nickname + " (" + rare->rareId + ")");
  } else {
    earned.nickname = GenerateNormalNickname(table, result);
    earned.isRare = false;
    Logger::LogUtf8("[NicknameManager] Normal nickname: " + earned.nickname);
  }

  return earned;
}

/*   レア二つ名の条件チェック（優先度順）   */
const RareNicknameData *NicknameManager::CheckRareNickname(
    const NicknameTableData &table, const ScoreResult &result,
    const UserData &userData, const ModBodyCustomizeData &playerData) {

  // 優先度順にソート済みなので、上から順にチェック
  // 同じ優先度で複数該当した場合は未取得のものからランダム
  std::vector<const RareNicknameData *> samePriorityCandidates;
  RareNicknamePriority currentPriority = RareNicknamePriority::Highest;
  bool foundPriority = false;

  for (const auto &rare : table.rareNicknames) {
    // 優先度が変わったら、前の優先度の候補を確認
    if (foundPriority && rare.priority != currentPriority) {
      // 前の優先度で候補があればその中からランダムで返す
      if (!samePriorityCandidates.empty()) {
        // 未取得のものだけフィルタ
        std::vector<const RareNicknameData *> unearned;
        for (const auto *candidate : samePriorityCandidates) {
          if (!HasNickname(userData, candidate->nickname)) {
            unearned.push_back(candidate);
          }
        }
        // 未取得があればそこからランダム
        if (!unearned.empty()) {
          std::random_device rd;
          std::mt19937 gen(rd());
          std::uniform_int_distribution<int> dist(
              0, static_cast<int>(unearned.size()) - 1);
          return unearned[dist(gen)];
        }
        // 未取得がなければ通常二つ名へ（この優先度はスキップ）
        samePriorityCandidates.clear();
      }
    }

    currentPriority = rare.priority;

    if (EvaluateRareCondition(rare, result, userData, playerData)) {
      samePriorityCandidates.push_back(&rare);
      foundPriority = true;
    }
  }

  // 最後の優先度グループの処理
  if (!samePriorityCandidates.empty()) {
    std::vector<const RareNicknameData *> unearned;
    for (const auto *candidate : samePriorityCandidates) {
      if (!HasNickname(userData, candidate->nickname)) {
        unearned.push_back(candidate);
      }
    }
    if (!unearned.empty()) {
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<int> dist(
          0, static_cast<int>(unearned.size()) - 1);
      return unearned[dist(gen)];
    }
  }

  return nullptr;
}

/*   単一のレア条件を判定する   */
bool NicknameManager::EvaluateRareCondition(
    const RareNicknameData &rare, const ScoreResult &result,
    const UserData &userData, const ModBodyCustomizeData &playerData) {

  switch (rare.conditionType) {
  case RareConditionType::AllStarsMin:
    return result.starThemeMatch >= rare.intValue &&
           result.starImpact >= rare.intValue &&
           result.starEfficiency >= rare.intValue &&
           result.starCommitment >= rare.intValue &&
           result.starJudgeBonus >= rare.intValue;

  case RareConditionType::AllStarsEqual:
    return result.starThemeMatch == rare.intValue &&
           result.starImpact == rare.intValue &&
           result.starEfficiency == rare.intValue &&
           result.starCommitment == rare.intValue &&
           result.starJudgeBonus == rare.intValue;

  case RareConditionType::FirstRankAchieved:
    // 今回のランクが指定ランク以上 かつ 過去に取得したことがない
    return RankToValue(result.overallRank) >= RankToValue(rare.rankValue) &&
           !HasAchievedRank(userData, rare.rankValue);

  case RareConditionType::StarsAndRank: {
    // 総合ランクが指定以上
    if (RankToValue(result.overallRank) < RankToValue(rare.rankValue)) {
      return false;
    }
    // ★条件をすべて満たすか
    for (const auto &cond : rare.starConditions) {
      // 特殊条件: changed_parts_max
      if (cond.starName == "changed_parts_max") {
        int changedCount = GetChangedPartCount(playerData);
        if (changedCount < cond.minValue || changedCount > cond.maxValue) {
          return false;
        }
        continue;
      }
      int star = GetStarByName(result, cond.starName);
      if (star < cond.minValue || star > cond.maxValue) {
        return false;
      }
    }
    return true;
  }

  case RareConditionType::StarsCombo: {
    // ★条件をすべて満たすか（ランク条件なし）
    for (const auto &cond : rare.starConditions) {
      int star = GetStarByName(result, cond.starName);
      if (star < cond.minValue || star > cond.maxValue) {
        return false;
      }
    }
    return true;
  }

  case RareConditionType::ChangedPartsMax:
    return GetChangedPartCount(playerData) <= rare.intValue;

  case RareConditionType::JudgeStarsAllEqual:
    return result.judgeEvaluations[0].star == result.judgeEvaluations[1].star &&
           result.judgeEvaluations[1].star == result.judgeEvaluations[2].star;

  case RareConditionType::JudgeStarsDiffMin: {
      int s0 = result.judgeEvaluations[0].star;
      int s1 = result.judgeEvaluations[1].star;
      int s2 = result.judgeEvaluations[2].star;
      int maxStar = std::max(s0, std::max(s1, s2));
      int minStar = std::min(s0, std::min(s1, s2));
      return (maxStar - minStar) >= rare.intValue;
  }

  case RareConditionType::ClearTimeMax:
    // 未実装：常に false を返す
    return false;

  default:
    return false;
  }
}

/*   通常二つ名を生成する   */
std::string NicknameManager::GenerateNormalNickname(
   const NicknameTableData &table, const ScoreResult &result) {
  // 形容詞：ランクから選択
  std::string adjective;
  auto adjIt = table.adjectives.find(result.overallRank);
  if (adjIt != table.adjectives.end() && !adjIt->second.empty()) {
    adjective = RandomPick(adjIt->second);
  } else {
    adjective = "謎の";
  }

  // 名詞：最も★が高い項目から選択
  struct StarEntry {
    std::string key;
    int value;
  };

  std::vector<StarEntry> stars = {
      {"theme_match", result.starThemeMatch},
      {"impact", result.starImpact},
      {"efficiency", result.starEfficiency},
      {"commitment", result.starCommitment},
      {"judge_bonus", result.starJudgeBonus},
  };

  // 最高★値を取得
  int maxStar = 0;
  for (const auto &s : stars) {
    if (s.value > maxStar) {
      maxStar = s.value;
    }
  }

  // 最高★値の項目を集める
  std::vector<std::string> topCategories;
  for (const auto &s : stars) {
    if (s.value == maxStar) {
      topCategories.push_back(s.key);
    }
  }

  // 同点の場合はランダムで1つ選択
  std::string selectedCategory;
  if (!topCategories.empty()) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(
        0, static_cast<int>(topCategories.size()) - 1);
    selectedCategory = topCategories[dist(gen)];
  }

  // 名詞を選択
  std::string noun;
  auto nounIt = table.nouns.find(selectedCategory);
  if (nounIt != table.nouns.end() && !nounIt->second.empty()) {
    noun = RandomPick(nounIt->second);
  } else {
    noun = "改造師";
  }

  return adjective + noun;
}

/*   ★項目名から値を取得   */
int NicknameManager::GetStarByName(const ScoreResult &result,
                                   const std::string &starName) {
  if (starName == "theme_match") return result.starThemeMatch;
  if (starName == "impact") return result.starImpact;
  if (starName == "efficiency") return result.starEfficiency;
  if (starName == "commitment") return result.starCommitment;
  if (starName == "judge_bonus") return result.starJudgeBonus;
  return 0;
}

/*   改造されたパーツ数を取得   */
int NicknameManager::GetChangedPartCount(
    const ModBodyCustomizeData &playerData) {
  int count = 0;
  for (size_t i = 0; i < kPartCount; ++i) {
    if (IsPartChanged(playerData.partParams[i])) {
      ++count;
    }
  }
  return count;
}

/*   ランクの強さを数値に変換   */
int NicknameManager::RankToValue(const std::string &rank) {
  if (rank == "SS") return 6;
  if (rank == "S") return 5;
  if (rank == "A") return 4;
  if (rank == "B") return 3;
  if (rank == "C") return 2;
  if (rank == "D") return 1;
  return 0;
}

/*   優先度文字列 → enum   */
RareNicknamePriority
NicknameManager::ParsePriority(const std::string &str) {
  if (str == "highest") return RareNicknamePriority::Highest;
  if (str == "high") return RareNicknamePriority::High;
  if (str == "medium") return RareNicknamePriority::Medium;
  if (str == "low") return RareNicknamePriority::Low;
  return RareNicknamePriority::Low;
}

/*   条件タイプ文字列 → enum   */
RareConditionType
NicknameManager::ParseConditionType(const std::string &str) {
  if (str == "all_stars_equal") return RareConditionType::AllStarsEqual;
  if (str == "all_stars_min") return RareConditionType::AllStarsMin;
  if (str == "first_rank_achieved") return RareConditionType::FirstRankAchieved;
  if (str == "stars_and_rank") return RareConditionType::StarsAndRank;
  if (str == "stars_combo") return RareConditionType::StarsCombo;
  if (str == "changed_parts_max") return RareConditionType::ChangedPartsMax;
  if (str == "judge_stars_all_equal") return RareConditionType::JudgeStarsAllEqual;
  if (str == "judge_stars_diff_min") return RareConditionType::JudgeStarsDiffMin;
  if (str == "clear_time_max") return RareConditionType::ClearTimeMax;
  return RareConditionType::AllStarsMin;
}

/*   JSON → RareNicknameData   */
bool NicknameManager::ParseRareNickname(const nlohmann::json &json,
                                        RareNicknameData &outData) {
  try {
    outData.rareId = json.at("rare_id").get<std::string>();
    outData.nickname = json.at("nickname").get<std::string>();
    outData.priority = ParsePriority(json.at("priority").get<std::string>());
    outData.conditionType =
        ParseConditionType(json.at("condition_type").get<std::string>());

    if (json.contains("int_value")) {
      outData.intValue = json.at("int_value").get<int>();
    }
    if (json.contains("rank_value")) {
      outData.rankValue = json.at("rank_value").get<std::string>();
    }
    if (json.contains("float_value")) {
      outData.floatValue = json.at("float_value").get<float>();
    }
    if (json.contains("star_conditions")) {
      for (const auto &sc : json.at("star_conditions")) {
        StarCondition cond;
        cond.starName = sc.at("star_name").get<std::string>();
        cond.minValue = sc.at("min_value").get<int>();
        cond.maxValue = sc.at("max_value").get<int>();
        outData.starConditions.push_back(cond);
      }
    }

    return true;
  } catch (const nlohmann::json::exception &e) {
    Logger::Log("[NicknameManager] Rare nickname parse error: %s", e.what());
    return false;
  }
}

/*   ユーザーが指定ランクを過去に取得したことがあるか   */
bool NicknameManager::HasAchievedRank(const UserData &userData,
                                      const std::string &rank) {
  return RankToValue(userData.bestRank) >= RankToValue(rank);
}

/*   ユーザーが指定の二つ名を既に持っているか   */
bool NicknameManager::HasNickname(const UserData &userData,
                                  const std::string &nickname) {
  for (const auto &nn : userData.nicknames) {
    if (nn.nickname == nickname) {
      return true;
    }
  }
  return false;
}
