#include "UserDataManager.h"
#include "GAME/json/JsonManager.h"
#include "Logger.h"

/*   コンストラクタ   */
UserDataManager::UserDataManager() {
  if (!Load()) {
    Logger::Log("[UserDataManager] No save file found, using default data");
    userData_ = UserData{};
  }
}

/*   ユーザーデータ取得   */
UserData &UserDataManager::GetUserData() { return userData_; }

const UserData &UserDataManager::GetUserData() const { return userData_; }

/*   セーブ   */
bool UserDataManager::Save() {
  nlohmann::json json = UserDataToJson(userData_);

  if (!JsonManager::Save(kSaveFilePath, json)) {
    Logger::Log("[UserDataManager] Failed to save user data");
    return false;
  }

  Logger::Log("[UserDataManager] User data saved successfully");
  return true;
}

/*   ロード   */
bool UserDataManager::Load() {
  auto json = JsonManager::Load(kSaveFilePath);
  if (!json.has_value()) {
    return false;
  }

  if (!JsonToUserData(json.value(), userData_)) {
    Logger::Log("[UserDataManager] Failed to parse user data");
    return false;
  }

  Logger::Log("[UserDataManager] User data loaded successfully");
  return true;
}

/*   プレイ回数インクリメント   */
void UserDataManager::IncrementPlayCount() { ++userData_.playCount; }

/*   最高ランク更新   */
void UserDataManager::UpdateBestRank(const std::string &rank) {
  if (RankToValue(rank) > RankToValue(userData_.bestRank)) {
    userData_.bestRank = rank;
  }
}

/*   二つ名追加   */
void UserDataManager::AddNickname(const EarnedNickname& nickname) {
    // 同じ二つ名が既にあるかチェック
    for (const auto& existing : userData_.nicknames) {
        if (existing.nickname == nickname.nickname) {
            return; // 重複は保存しない
        }
    }
    userData_.nicknames.push_back(nickname);
}

/*   トロフィー追加   */
bool UserDataManager::AddTrophy(const TrophyData &trophy) {
  if (userData_.IsTrophyFull()) {
    return false;
  }
  userData_.AddTrophy(trophy);
  return true;
}

/*   トロフィー入れ替え   */
void UserDataManager::ReplaceTrophy(int index, const TrophyData &trophy) {
  userData_.ReplaceTrophy(index, trophy);
}

/*   セーブファイル存在チェック   */
bool UserDataManager::SaveFileExists() { return JsonManager::Exists(kSaveFilePath); }

/*   ランクの強さを数値に変換   */
int UserDataManager::RankToValue(const std::string &rank) {
  if (rank == "SS") return 6;
  if (rank == "S") return 5;
  if (rank == "A") return 4;
  if (rank == "B") return 3;
  if (rank == "C") return 2;
  if (rank == "D") return 1;
  return 0;
}

/*   UserData → JSON 変換   */
nlohmann::json UserDataManager::UserDataToJson(const UserData &data) {
  nlohmann::json json;

  json["player_name"] = data.playerName;
  json["play_count"] = data.playCount;
  json["best_rank"] = data.bestRank;

  // 二つ名
  nlohmann::json nicknamesJson = nlohmann::json::array();
  for (const auto &nn : data.nicknames) {
    nlohmann::json nnJson;
    nnJson["nickname"] = nn.nickname;
    nnJson["is_rare"] = nn.isRare;
    nnJson["earned_theme"] = nn.earnedTheme;
    nnJson["earned_rank"] = nn.earnedRank;
    nicknamesJson.push_back(nnJson);
  }
  json["nicknames"] = nicknamesJson;

  // トロフィー
  nlohmann::json trophiesJson = nlohmann::json::array();
  for (const auto &trophy : data.trophies) {
    nlohmann::json tJson;

    // 二つ名情報
    tJson["nickname"] = trophy.nickname.nickname;
    tJson["nickname_is_rare"] = trophy.nickname.isRare;
    tJson["theme_id"] = trophy.themeId;
    tJson["theme_name"] = trophy.themeName;
    tJson["overall_rank"] = trophy.overallRank;
    tJson["total_stars"] = trophy.totalStars;

    // 改造データ
    nlohmann::json partsJson = nlohmann::json::array();
    for (size_t i = 0; i < static_cast<size_t>(ModBodyPart::Count); ++i) {
      const auto &param = trophy.customizeData.partParams[i];
      nlohmann::json pJson;
      pJson["scale_x"] = param.scale.x;
      pJson["scale_y"] = param.scale.y;
      pJson["scale_z"] = param.scale.z;
      pJson["length"] = param.length;
      pJson["count"] = param.count;
      pJson["enabled"] = param.enabled;
      partsJson.push_back(pJson);
    }
    tJson["part_params"] = partsJson;

    // ジョイントオフセット
    nlohmann::json jointsJson = nlohmann::json::array();
    for (size_t i = 0; i < static_cast<size_t>(ModBodyPart::Count); ++i) {
      const auto &offset = trophy.customizeData.bodyJointOffsets[i];
      nlohmann::json jJson;
      jJson["x"] = offset.x;
      jJson["y"] = offset.y;
      jJson["z"] = offset.z;
      jointsJson.push_back(jJson);
    }
    tJson["joint_offsets"] = jointsJson;

    trophiesJson.push_back(tJson);
  }
  json["trophies"] = trophiesJson;

  return json;
}

/*   JSON → UserData 変換   */
bool UserDataManager::JsonToUserData(const nlohmann::json &json,
                                     UserData &outData) {
  try {
    outData.playerName = json.value("player_name", "Player");
    outData.playCount = json.value("play_count", 0);
    outData.bestRank = json.value("best_rank", "D");

    // 二つ名
    outData.nicknames.clear();
    if (json.contains("nicknames")) {
      for (const auto &nnJson : json.at("nicknames")) {
        EarnedNickname nn;
        nn.nickname = nnJson.value("nickname", "");
        nn.isRare = nnJson.value("is_rare", false);
        nn.earnedTheme = nnJson.value("earned_theme", "");
        nn.earnedRank = nnJson.value("earned_rank", "");
        outData.nicknames.push_back(nn);
      }
    }

    // トロフィー
    outData.trophies.clear();
    if (json.contains("trophies")) {
      for (const auto &tJson : json.at("trophies")) {
        TrophyData trophy;

        // 二つ名情報
        trophy.nickname.nickname = tJson.value("nickname", "");
        trophy.nickname.isRare = tJson.value("nickname_is_rare", false);
        trophy.themeId = tJson.value("theme_id", "");
        trophy.themeName = tJson.value("theme_name", "");
        trophy.overallRank = tJson.value("overall_rank", "D");
        trophy.totalStars = tJson.value("total_stars", 0);

        // 改造データ
        if (tJson.contains("part_params")) {
          const auto &partsJson = tJson.at("part_params");
          for (size_t i = 0;
               i < partsJson.size() &&
               i < static_cast<size_t>(ModBodyPart::Count);
               ++i) {
            const auto &pJson = partsJson[i];
            auto &param = trophy.customizeData.partParams[i];
            param.scale.x = pJson.value("scale_x", 1.0f);
            param.scale.y = pJson.value("scale_y", 1.0f);
            param.scale.z = pJson.value("scale_z", 1.0f);
            param.length = pJson.value("length", 1.0f);
            param.count = pJson.value("count", 1);
            param.enabled = pJson.value("enabled", true);
          }
        }

        // ジョイントオフセット
        if (tJson.contains("joint_offsets")) {
          const auto &jointsJson = tJson.at("joint_offsets");
          for (size_t i = 0;
               i < jointsJson.size() &&
               i < static_cast<size_t>(ModBodyPart::Count);
               ++i) {
            const auto &jJson = jointsJson[i];
            auto &offset = trophy.customizeData.bodyJointOffsets[i];
            offset.x = jJson.value("x", 0.0f);
            offset.y = jJson.value("y", 0.0f);
            offset.z = jJson.value("z", 0.0f);
          }
        }

        outData.trophies.push_back(trophy);
      }
    }

    return true;
  } catch (const nlohmann::json::exception &e) {
    Logger::Log("[UserDataManager] JSON parse error: %s", e.what());
    return false;
  }
}
