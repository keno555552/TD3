#include "ThemeManager.h"
#include "GAME/json/JsonManager.h"
#include "Logger.h"
#include <random>

namespace {

	/// パーツ名テーブル（ModBodyPart の並び順と一致）
	const char* kPartNames[] = {
		"ChestBody",    "StomachBody",   "Neck",          "Head",
		"LeftUpperArm", "LeftForeArm",   "RightUpperArm",
		"RightForeArm", "LeftThigh",     "LeftShin",
		"RightThigh",   "RightShin",
	};

	/// カウント名テーブル（CountPart の並び順と一致）
	const char* kCountNames[] = {
		"left_arm",
		"right_arm",
		"left_leg",
		"right_leg",
	};

	/// ボーナス名テーブル（BonusType の並び順と一致）
	const char* kBonusNames[] = {
		"symmetry",
		"cost_efficiency",
		"minimalist",
		"wildcard",
		"balance",
	};

} // namespace

/*   コンストラクタ：全お題 JSON を読み込む   */
ThemeManager::ThemeManager(const std::string& themesDirectory) {
	auto fileList = JsonManager::GetFileList(themesDirectory);

	Logger::Log("[ThemeManager] Found %d theme file(s) in: %s",
		static_cast<int>(fileList.size()), themesDirectory.c_str());

	for (const auto& filePath : fileList) {
		auto json = JsonManager::Load(filePath);
		if (!json.has_value()) {
			Logger::Log("[ThemeManager] Failed to load: %s", filePath.c_str());
			continue;
		}

		ThemeData data;
		if (ParseThemeData(json.value(), data)) {
			themes_.push_back(std::move(data));
			Logger::LogUtf8("[ThemeManager] Loaded theme: " + themes_.back().themeName +
				" (" + themes_.back().themeId + ")");
		} else {
			Logger::Log("[ThemeManager] Failed to parse: %s", filePath.c_str());
		}
	}

	Logger::Log("[ThemeManager] Total themes loaded: %d",
		static_cast<int>(themes_.size()));
}

/*   前回と異なるお題をランダム選出   */
ThemeData* ThemeManager::SelectRandom() {
	if (themes_.empty()) {
		Logger::Log("[ThemeManager] No themes available");
		return nullptr;
	}

	// お題が 1 つしかない場合はそれを返す
	if (themes_.size() == 1) {
		currentIndex_ = 0;
		SaveLastThemeId(themes_[0].themeId);
		Logger::Log("[ThemeManager] Selected theme: %s (only one available)",
			themes_[0].themeName.c_str());
		return &themes_[0];
	}

	// 前回のお題 ID を取得
	std::string lastThemeId = LoadLastThemeId();

	// 前回以外の候補を集める
	std::vector<int> candidates;
	for (int i = 0; i < static_cast<int>(themes_.size()); ++i) {
		if (themes_[i].themeId != lastThemeId) {
			candidates.push_back(i);
		}
	}

	// 全て除外された場合（通常は起きないが安全策）
	if (candidates.empty()) {
		for (int i = 0; i < static_cast<int>(themes_.size()); ++i) {
			candidates.push_back(i);
		}
	}

	// ランダム選出
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(0,
		static_cast<int>(candidates.size()) - 1);

	currentIndex_ = candidates[dist(gen)];

	// セーブファイルに記録
	SaveLastThemeId(themes_[currentIndex_].themeId);

	Logger::LogUtf8("[ThemeManager] Selected theme: " +
		themes_[currentIndex_].themeName + " (" +
		themes_[currentIndex_].themeId + ")");

	return &themes_[currentIndex_];
}

/*   現在選ばれているお題を取得   */
ThemeData* ThemeManager::GetCurrentTheme() const {
	if (currentIndex_ < 0 ||
		currentIndex_ >= static_cast<int>(themes_.size())) {
		return nullptr;
	}
	return const_cast<ThemeData*>(&themes_[currentIndex_]);
}

/*   読み込み済みのお題数   */
size_t ThemeManager::GetThemeCount() const { return themes_.size(); }

/*   パーツ名 → インデックス変換   */
int ThemeManager::PartNameToIndex(const std::string& name) {
	for (int i = 0; i < 11; ++i) {
		if (name == kPartNames[i]) {
			return i;
		}
	}
	return -1;
}

/*   カウント名 → インデックス変換   */
int ThemeManager::CountNameToIndex(const std::string& name) {
	constexpr int count = static_cast<int>(CountPart::Count);
	for (int i = 0; i < count; ++i) {
		if (name == kCountNames[i]) {
			return i;
		}
	}
	return -1;
}

/*   ボーナス名 → インデックス変換   */
int ThemeManager::BonusNameToIndex(const std::string& name) {
	constexpr int count = static_cast<int>(BonusType::Count);
	for (int i = 0; i < count; ++i) {
		if (name == kBonusNames[i]) {
			return i;
		}
	}
	return -1;
}

/*   JSON → ThemeData 変換   */
bool ThemeManager::ParseThemeData(const nlohmann::json& json,
	ThemeData& outData) {
	try {
		// 基本情報
		outData.themeId = json.at("theme_id").get<std::string>();
		outData.themeName = json.at("theme_name").get<std::string>();
		outData.category = json.at("category").get<std::string>();

		// テクスチャパスを取得
		if (json.contains("texture_path")) {
			outData.texturePath = json.at("texture_path").get<std::string>();
		} else {
			outData.texturePath = "GAME/resources/texture/prompt.png";
		}

		// スコアセット
		const auto& scoreSets = json.at("score_sets");
		for (const auto& setJson : scoreSets) {
			ScoreSet scoreSet;
			scoreSet.setName = setJson.at("set_name").get<std::string>();

			// パーツ倍率
			if (setJson.contains("parts")) {
				for (const auto& [key, value] : setJson.at("parts").items()) {
					int index = PartNameToIndex(key);
					if (index < 0) {
						Logger::Log("[ThemeManager] Unknown part name: %s", key.c_str());
						continue;
					}
					scoreSet.partWeights[index].scaleX =
						value.at("scale_x").get<float>();
					scoreSet.partWeights[index].scaleY =
						value.at("scale_y").get<float>();
					scoreSet.partWeights[index].scaleZ =
						value.at("scale_z").get<float>();
				}
			}

			// 本数倍率
			if (setJson.contains("counts")) {
				for (const auto& [key, value] : setJson.at("counts").items()) {
					int index = CountNameToIndex(key);
					if (index < 0) {
						Logger::Log("[ThemeManager] Unknown count name: %s", key.c_str());
						continue;
					}
					scoreSet.countWeights[index].weight =
						value.at("weight").get<float>();
					scoreSet.countWeights[index].invert =
						value.at("invert").get<bool>();
				}
			}

			outData.scoreSets.push_back(std::move(scoreSet));
		}

		// ボーナス
		if (json.contains("bonuses")) {
			for (const auto& [key, value] : json.at("bonuses").items()) {
				int index = BonusNameToIndex(key);
				if (index < 0) {
					Logger::Log("[ThemeManager] Unknown bonus name: %s", key.c_str());
					continue;
				}
				outData.bonusWeights[index].weight = value.at("weight").get<float>();
				outData.bonusWeights[index].invert = value.at("invert").get<bool>();
			}
		}

		return true;
	}
	catch (const nlohmann::json::exception& e) {
		Logger::Log("[ThemeManager] JSON parse error: %s", e.what());
		return false;
	}
}

/*   セーブファイルから前回のお題 ID を読み込む   */
std::string ThemeManager::LoadLastThemeId() {
	auto json = JsonManager::Load(kSaveFilePath);
	if (!json.has_value()) {
		return "";
	}

	try {
		return json.value().at("last_theme_id").get<std::string>();
	}
	catch (const nlohmann::json::exception&) {
		return "";
	}
}

/*   セーブファイルに今回のお題 ID を保存する   */
void ThemeManager::SaveLastThemeId(const std::string& themeId) {
	nlohmann::json json;
	json["last_theme_id"] = themeId;

	if (!JsonManager::Save(kSaveFilePath, json)) {
		Logger::Log("[ThemeManager] Failed to save last theme ID");
	}
}
