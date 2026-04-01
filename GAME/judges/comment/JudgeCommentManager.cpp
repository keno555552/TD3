#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "JudgeCommentManager.h"
#include "GAME/json/JsonManager.h"
#include "Logger.h"
#include <random>

namespace {

/// ランダムで vector から1つ選ぶ
std::string RandomPick(const std::vector<std::string> &list) {
  if (list.empty()) {
    return "";
  }
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dist(0,
                                          static_cast<int>(list.size()) - 1);
  return list[dist(gen)];
}

} // namespace

/*   フォルダから審査員コメントを一括読み込みする   */
int JudgeCommentManager::LoadCommentTable(const std::string &dirPath,
                                          JudgeCommentTable &outTable) {
  auto files = JsonManager::GetFileList(dirPath);
  int loadedCount = 0;

  for (const auto &filePath : files) {
    JudgeCommentSet set;
    if (LoadSingleComment(filePath, set)) {
      outTable.comments[set.judgeId] = set;
      ++loadedCount;
    }
  }

  Logger::Log("[JudgeCommentManager] Loaded %d judge comment sets",
              loadedCount);
  return loadedCount;
}

/*   1ファイルから審査員コメントを読み込む   */
bool JudgeCommentManager::LoadSingleComment(const std::string &filePath,
                                            JudgeCommentSet &outSet) {
  auto json = JsonManager::Load(filePath);
  if (!json.has_value()) {
    Logger::Log("[JudgeCommentManager] Failed to load: %s", filePath.c_str());
    return false;
  }

  try {
    const auto &j = json.value();

    outSet.judgeId = j.at("judge_id").get<std::string>();

    if (j.contains("high_star")) {
      for (const auto &c : j.at("high_star")) {
        outSet.highStar.push_back(c.get<std::string>());
      }
    }

    if (j.contains("mid_star")) {
      for (const auto &c : j.at("mid_star")) {
        outSet.midStar.push_back(c.get<std::string>());
      }
    }

    if (j.contains("low_star")) {
      for (const auto &c : j.at("low_star")) {
        outSet.lowStar.push_back(c.get<std::string>());
      }
    }

    Logger::LogUtf8("[JudgeCommentManager] Loaded comments for: " +
                    outSet.judgeId);
    return true;
  } catch (const nlohmann::json::exception &e) {
    Logger::Log("[JudgeCommentManager] Parse error in %s: %s",
                filePath.c_str(), e.what());
    return false;
  }
}

/*   スコア結果から審査員3人分のコメントを生成する   */
std::vector<JudgeCommentResult> JudgeCommentManager::GenerateComments(
    const JudgeCommentTable &table,
    const ScoreResult &result) {
  std::vector<JudgeCommentResult> results;

  for (size_t i = 0; i < 3; ++i) {
    const auto &eval = result.judgeEvaluations[i];
    JudgeCommentResult commentResult;
    commentResult.judgeId = eval.judgeId;
    commentResult.star = eval.star;

    // この審査員のコメントセットを探す
    auto it = table.comments.find(eval.judgeId);
    if (it == table.comments.end()) {
      commentResult.comment = "...";
      results.push_back(commentResult);
      Logger::LogUtf8("[JudgeCommentManager] No comments for: " +
                      eval.judgeId);
      continue;
    }

    const JudgeCommentSet &set = it->second;

    // ★からコメントカテゴリを選ぶ
    std::string category = StarToCategory(eval.star);

    // カテゴリに応じたコメントリストから選ぶ
    if (category == "high") {
      commentResult.comment = RandomPick(set.highStar);
    } else if (category == "mid") {
      commentResult.comment = RandomPick(set.midStar);
    } else {
      commentResult.comment = RandomPick(set.lowStar);
    }

    if (commentResult.comment.empty()) {
      commentResult.comment = "...";
    }

    results.push_back(commentResult);
    Logger::LogUtf8("[JudgeCommentManager] " + eval.judgeId + " (star=" +
                    std::to_string(eval.star) + "): " +
                    commentResult.comment);
  }

  return results;
}

/*   ★の値からコメントカテゴリを選択する   */
std::string JudgeCommentManager::StarToCategory(int star) {
  if (star >= 4) {
    return "high";
  }
  if (star >= 3) {
    return "mid";
  }
  return "low";
}
