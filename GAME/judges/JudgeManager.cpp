#include "JudgeManager.h"
#include "GAME/json/JsonManager.h"
#include "Logger.h"
#include <algorithm>
#include <random>

namespace {

/// パーツ名テーブル（ModBodyPart の並び順と一致）
const char *kPartNames[] = {
    "Body",         "Neck",          "Head",
    "LeftUpperArm", "LeftForeArm",   "RightUpperArm",
    "RightForeArm", "LeftThigh",     "LeftShin",
    "RightThigh",   "RightShin",
};

constexpr size_t kPartCount = 11;

} // namespace

/*   コンストラクタ：全審査員 JSON を読み込む   */
JudgeManager::JudgeManager(const std::string &judgesDirectory) {
  auto fileList = JsonManager::GetFileList(judgesDirectory);

  Logger::Log("[JudgeManager] Found %d judge file(s) in: %s",
              static_cast<int>(fileList.size()), judgesDirectory.c_str());

  for (const auto &filePath : fileList) {
    auto json = JsonManager::Load(filePath);
    if (!json.has_value()) {
      Logger::Log("[JudgeManager] Failed to load: %s", filePath.c_str());
      continue;
    }

    JudgeData data;
    if (ParseJudgeData(json.value(), data)) {
      judges_.push_back(std::move(data));
      Logger::LogUtf8("[JudgeManager] Loaded judge: " +
          judges_.back().judgeName + " (" +
          judges_.back().judgeId + ")");
    } else {
      Logger::Log("[JudgeManager] Failed to parse: %s", filePath.c_str());
    }
  }

  Logger::Log("[JudgeManager] Total judges loaded: %d",
              static_cast<int>(judges_.size()));
}

/*   ランダムで審査員を選出   */
std::vector<const JudgeData *> JudgeManager::SelectRandom(int count) {
  currentIndices_.clear();

  if (judges_.empty()) {
    Logger::Log("[JudgeManager] No judges available");
    return {};
  }

  // 要求人数が全体より多い場合は全員選出
  if (count >= static_cast<int>(judges_.size())) {
    for (int i = 0; i < static_cast<int>(judges_.size()); ++i) {
      currentIndices_.push_back(i);
    }
  } else {
    // 全インデックスを作ってシャッフルし、先頭から count 人を選ぶ
    std::vector<int> allIndices;
    for (int i = 0; i < static_cast<int>(judges_.size()); ++i) {
      allIndices.push_back(i);
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(allIndices.begin(), allIndices.end(), gen);

    for (int i = 0; i < count; ++i) {
      currentIndices_.push_back(allIndices[i]);
    }
  }

  // 選ばれた審査員をログに出力
  for (int index : currentIndices_) {
      Logger::LogUtf8("[JudgeManager] Selected judge: " +
          judges_[index].judgeName + " (" +
          judges_[index].judgeId + ")");
  }

  return GetCurrentJudges();
}

/*   現在選ばれている審査員を取得   */
std::vector<const JudgeData *> JudgeManager::GetCurrentJudges() const {
  std::vector<const JudgeData *> result;

  for (int index : currentIndices_) {
    if (index >= 0 && index < static_cast<int>(judges_.size())) {
      result.push_back(&judges_[index]);
    }
  }

  return result;
}

/*   読み込み済みの審査員数   */
size_t JudgeManager::GetJudgeCount() const { return judges_.size(); }

/*   パーツ名 → インデックス変換   */
int JudgeManager::PartNameToIndex(const std::string &name) {
  for (int i = 0; i < static_cast<int>(kPartCount); ++i) {
    if (name == kPartNames[i]) {
      return i;
    }
  }
  return -1;
}

/*   JSON → JudgeData 変換   */
bool JudgeManager::ParseJudgeData(const nlohmann::json &json,
                                  JudgeData &outData) {
  try {
    outData.judgeId = json.at("judge_id").get<std::string>();
    outData.judgeName = json.at("judge_name").get<std::string>();
    outData.judgeTitle = json.at("judge_title").get<std::string>();

    // テクスチャパス（省略時はデフォルト）
    if (json.contains("texture_path")) {
      outData.texturePath = json.at("texture_path").get<std::string>();
    } else {
      outData.texturePath = "GAME/resources/texture/judge_default.png";
    }

    // 好みパーツ
    if (json.contains("favorite_parts")) {
      for (const auto &[key, value] : json.at("favorite_parts").items()) {
        int index = PartNameToIndex(key);
        if (index < 0) {
          Logger::Log("[JudgeManager] Unknown part name: %s", key.c_str());
          continue;
        }
        outData.partPreferences[index].threshold = value.get<float>();
      }
    }

    return true;
  } catch (const nlohmann::json::exception &e) {
    Logger::Log("[JudgeManager] JSON parse error: %s", e.what());
    return false;
  }
}
