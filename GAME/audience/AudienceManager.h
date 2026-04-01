#pragma once
#include "AudienceData.h"
#include "GAME/actor/ModBody.h"
#include "externals/nlohmann/json.hpp"
#include <string>

/// 観客コメントの読み込み・生成を行うクラス
/// 内部状態を持たず、すべて static 関数で提供する
class AudienceManager {
public:
	/// 観客コメントデータを JSON から読み込む
	/// @param filePath audience_comments.json のパス
	/// @param outData 読み込み先
	/// @return 成功時は true
	static bool LoadCommentData(const std::string& filePath,
		AudienceCommentData& outData);

	/// 改造データから観客3人分のコメントを生成する
	/// @param data コメントデータ
	/// @param playerData プレイヤーの改造データ
	/// @return 生成された AudienceResult
	static AudienceResult GenerateComments(
		const AudienceCommentData& data,
		const ModBodyCustomizeData& playerData);

private:
	AudienceManager() = delete;

	/// パーツの変化方向を判定する
    /// @return "bigger" or "smaller"
	static std::string DetermineDirection(ModBodyPart partType,
		const ModBodyCustomizeData& playerData);

		/// パーツタイプからJSONのグループ名を返す
		static std::string PartTypeToGroupName(ModBodyPart partType);

	/// 全体の改造量からテンションを判定する
	/// @return "high", "medium", "low"
	static std::string DetermineTension(float totalChange);
};
