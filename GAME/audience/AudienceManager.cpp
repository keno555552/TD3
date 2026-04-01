#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "AudienceManager.h"
#include "GAME/json/JsonManager.h"
#include "GAME/score/ScoreCalculator.h"
#include "Logger.h"
#include <algorithm>
#include <random>

namespace {

constexpr size_t kPartCount = 12;
constexpr size_t kAudienceCount = 3;

/// 重複なしでランダムに1つ選ぶ（既に使ったものを除外）
std::string RandomPickExcluding(const std::vector<std::string> &list,
                                const std::vector<std::string> &used) {
  std::vector<std::string> candidates;
  for (const auto &item : list) {
    bool alreadyUsed = false;
    for (const auto &u : used) {
      if (u == item) {
        alreadyUsed = true;
        break;
      }
    }
    if (!alreadyUsed) {
      candidates.push_back(item);
    }
  }

  if (candidates.empty()) {
    // 全部使い切った場合は元のリストからランダム
    if (list.empty()) {
      return "";
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(
        0, static_cast<int>(list.size()) - 1);
    return list[dist(gen)];
  }

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dist(
      0, static_cast<int>(candidates.size()) - 1);
  return candidates[dist(gen)];
}

/// パーツ変化量でソートするための構造体
struct PartChangeEntry {
  ModBodyPart partType;
  std::string groupName;
  float changeAmount;
};

} // namespace

/*   JSON から観客コメントデータを読み込む   */
bool AudienceManager::LoadCommentData(const std::string &filePath,
                                      AudienceCommentData &outData) {
  auto json = JsonManager::Load(filePath);
  if (!json.has_value()) {
    Logger::Log("[AudienceManager] Failed to load: %s", filePath.c_str());
    return false;
  }

  try {
    const auto &j = json.value();

    // パーツ固有コメント
    if (j.contains("part_comments")) {
        for (const auto& [group, dirObj] : j.at("part_comments").items()) {
            PartDirectionComments dirComments;
            if (dirObj.contains("bigger")) {
                for (const auto& comment : dirObj.at("bigger")) {
                    dirComments.bigger.push_back(comment.get<std::string>());
                }
            }
            if (dirObj.contains("smaller")) {
                for (const auto& comment : dirObj.at("smaller")) {
                    dirComments.smaller.push_back(comment.get<std::string>());
                }
            }
            outData.partComments[group] = dirComments;
        }
    }

    // テンプレートコメント
    if (j.contains("template_comments")) {
      for (const auto &[tension, comments] :
           j.at("template_comments").items()) {
        std::vector<std::string> list;
        for (const auto &comment : comments) {
          list.push_back(comment.get<std::string>());
        }
        outData.templateComments[tension] = list;
      }
    }

    // 改造なしコメント
    if (j.contains("no_change_comments")) {
      for (const auto &comment : j.at("no_change_comments")) {
        outData.noChangeComments.push_back(comment.get<std::string>());
      }
    }

    Logger::Log("[AudienceManager] Loaded: %d part groups, %d tension levels, "
                "%d no-change comments",
                static_cast<int>(outData.partComments.size()),
                static_cast<int>(outData.templateComments.size()),
                static_cast<int>(outData.noChangeComments.size()));

    return true;
  } catch (const nlohmann::json::exception &e) {
    Logger::Log("[AudienceManager] JSON parse error: %s", e.what());
    return false;
  }
}

/*   観客3人分のコメントを生成する   */
AudienceResult AudienceManager::GenerateComments(
    const AudienceCommentData &data,
    const ModBodyCustomizeData &playerData) {
  AudienceResult result;

  // 改造されたパーツがあるか確認する
  float totalChange = 0.0f;
  std::vector<PartChangeEntry> entries;

  for (size_t i = 0; i < kPartCount; ++i) {
    ModBodyPart partType = static_cast<ModBodyPart>(i);
    float change = ScoreCalculator::GetPartChangeAmountFromCP(partType,
                                                              playerData);
    totalChange += change;

    if (change > 0.01f) {
      PartChangeEntry entry;
      entry.partType = partType;
      entry.groupName = PartTypeToGroupName(partType);
      entry.changeAmount = change;
      entries.push_back(entry);
    }
  }

  // 改造なしの場合
  if (entries.empty()) {
    std::vector<std::string> used;
    for (size_t i = 0; i < kAudienceCount; ++i) {
      std::string comment =
          RandomPickExcluding(data.noChangeComments, used);
      result.comments[i] = comment;
      used.push_back(comment);
    }
    Logger::LogUtf8("[AudienceManager] No changes - using no_change_comments");
    return result;
  }

  // 変化量が大きい順にソートする
  std::sort(entries.begin(), entries.end(),
            [](const PartChangeEntry &a, const PartChangeEntry &b) {
              return a.changeAmount > b.changeAmount;
            });

  // コメントを生成する
  std::vector<std::string> usedComments;
  size_t filled = 0;

  // パーツ固有コメントで埋める（変化量が大きいパーツから）
  for (size_t i = 0; i < entries.size() && filled < kAudienceCount; ++i) {
      auto it = data.partComments.find(entries[i].groupName);
      if (it == data.partComments.end()) {
          continue;
      }

      // 変化の方向に合ったコメントリストを選ぶ
      std::string direction = DetermineDirection(entries[i].partType, playerData);
      const std::vector<std::string>& commentList =
          (direction == "bigger") ? it->second.bigger : it->second.smaller;

      if (commentList.empty()) {
          continue;
      }

      std::string comment = RandomPickExcluding(commentList, usedComments);
      if (!comment.empty()) {
          result.comments[filled] = comment;
          usedComments.push_back(comment);
          ++filled;
      }
  }

  // 残り枠をテンプレートコメントで埋める
  if (filled < kAudienceCount) {
    std::string tension = DetermineTension(totalChange);
    auto it = data.templateComments.find(tension);

    if (it != data.templateComments.end() && !it->second.empty()) {
      while (filled < kAudienceCount) {
        std::string comment = RandomPickExcluding(it->second, usedComments);
        if (comment.empty()) {
          break;
        }
        result.comments[filled] = comment;
        usedComments.push_back(comment);
        ++filled;
      }
    }
  }

  // それでも埋まらない場合のフォールバック
  while (filled < kAudienceCount) {
    result.comments[filled] = "おおー！";
    ++filled;
  }

  for (size_t i = 0; i < kAudienceCount; ++i) {
    Logger::LogUtf8("[AudienceManager] Audience " + std::to_string(i + 1) +
                    ": " + result.comments[i]);
  }

  return result;
}

/*   パーツの変化方向を判定する   */
std::string AudienceManager::DetermineDirection(
    ModBodyPart partType, const ModBodyCustomizeData& playerData) {
    Vector3 scale =
        ScoreCalculator::CalcPartChangeFromControlPoints(partType, playerData);

    // x,y,zの変化量の合計で方向を判定する
    // 1.0より大きい成分が多ければbigger、小さい成分が多ければsmaller
    float biggerAmount = 0.0f;
    float smallerAmount = 0.0f;

    if (scale.x > 1.0f) biggerAmount += scale.x - 1.0f;
    else smallerAmount += 1.0f - scale.x;

    if (scale.y > 1.0f) biggerAmount += scale.y - 1.0f;
    else smallerAmount += 1.0f - scale.y;

    if (scale.z > 1.0f) biggerAmount += scale.z - 1.0f;
    else smallerAmount += 1.0f - scale.z;

    return (biggerAmount >= smallerAmount) ? "bigger" : "smaller";
}

/*   パーツタイプからJSONのグループ名を返す   */
std::string AudienceManager::PartTypeToGroupName(ModBodyPart partType) {
  switch (partType) {
  case ModBodyPart::ChestBody:
    return "ChestBody";
  case ModBodyPart::StomachBody:
    return "StomachBody";
  case ModBodyPart::Neck:
    return "Neck";
  case ModBodyPart::Head:
    return "Head";
  case ModBodyPart::LeftUpperArm:
  case ModBodyPart::RightUpperArm:
    return "UpperArm";
  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
    return "ForeArm";
  case ModBodyPart::LeftThigh:
  case ModBodyPart::RightThigh:
    return "Thigh";
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
    return "Shin";
  default:
    return "";
  }
}

/*   全体の改造量からテンションを判定する   */
std::string AudienceManager::DetermineTension(float totalChange) {
  if (totalChange >= 3.0f) {
    return "high";
  }
  if (totalChange >= 1.0f) {
    return "medium";
  }
  return "low";
}
