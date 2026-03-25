#pragma once
#include "UserData.h"
#include <string>

#include "externals/nlohmann/json.hpp"

/// ユーザーデータのセーブ・ロードを管理するクラス
class UserDataManager {
public:
  /// コンストラクタ：セーブファイルからユーザーデータを読み込む
  /// ファイルが存在しない場合はデフォルトのデータを作成する
  UserDataManager();

  ~UserDataManager() = default;

  /// 現在のユーザーデータを取得する
  UserData &GetUserData();
  const UserData &GetUserData() const;

  /// ユーザーデータをファイルに保存する
  /// @return 成功時は true
  bool Save();

  /// ユーザーデータをファイルから再読み込みする
  /// @return 成功時は true
  bool Load();

  /// プレイ回数をインクリメントする
  void IncrementPlayCount();

  /// 最高ランクを更新する（現在より上のランクなら更新）
  /// @param rank 今回のランク
  void UpdateBestRank(const std::string &rank);

  /// 二つ名を追加する
  /// @param nickname 取得した二つ名
  void AddNickname(const EarnedNickname &nickname);

  /// トロフィーを追加する（上限チェック付き）
  /// @param trophy 保存するトロフィー
  /// @return 追加できたら true、上限に達していたら false
  bool AddTrophy(const TrophyData &trophy);

  /// トロフィーを入れ替える
  /// @param index 入れ替え対象のインデックス
  /// @param trophy 新しいトロフィー
  void ReplaceTrophy(int index, const TrophyData &trophy);

  /// セーブファイルが存在するか
  static bool SaveFileExists();

private:
  UserData userData_;

  // セーブファイルのパス
  static constexpr const char *kSaveFilePath = "release/save/userdata.json";

  /// UserData を JSON に変換する
  static nlohmann::json UserDataToJson(const UserData &data);

  /// JSON を UserData に変換する
  static bool JsonToUserData(const nlohmann::json &json, UserData &outData);

  /// ランクの強さを数値に変換する（比較用）
  static int RankToValue(const std::string &rank);
};
