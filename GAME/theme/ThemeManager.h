#pragma once
#include "ThemeData.h"
#include <string>
#include <vector>

#include "externals/nlohmann/json.hpp"

/// お題の読み込み・ランダム選出を管理するクラス
class ThemeManager {
public:
  /// コンストラクタ：指定フォルダ内の全お題 JSON を読み込む
  /// @param themesDirectory お題 JSON が格納されたフォルダのパス
  ThemeManager(const std::string &themesDirectory);

  ~ThemeManager() = default;

  /// 前回と異なるお題をランダムで選出する
  /// @return 選出されたお題データへのポインタ（内部所有、失敗時は nullptr）
  ThemeData *SelectRandom();

  /// 現在選ばれているお題を取得する
  /// @return 現在のお題データへのポインタ（未選出時は nullptr）
  ThemeData *GetCurrentTheme() const;

  /// 読み込み済みのお題数を取得する
  size_t GetThemeCount() const;

private:
  // 読み込んだ全お題データ
  std::vector<ThemeData> themes_;

  // 現在選ばれているお題のインデックス（未選出時は -1）
  int currentIndex_ = -1;

  // セーブファイルのパス
  static constexpr const char *kSaveFilePath = "release/save/last_theme.json";

  /// JSON からパーツ名を ModBodyPart のインデックスに変換する
  /// @return 成功時はインデックス、失敗時は -1
  static int PartNameToIndex(const std::string &name);

  /// JSON からカウント名を CountPart のインデックスに変換する
  /// @return 成功時はインデックス、失敗時は -1
  static int CountNameToIndex(const std::string &name);

  /// JSON からボーナス名を BonusType のインデックスに変換する
  /// @return 成功時はインデックス、失敗時は -1
  static int BonusNameToIndex(const std::string &name);

  /// 1 つの JSON オブジェクトを ThemeData に変換する
  /// @return 成功時は true
  static bool ParseThemeData(const nlohmann::json &json, ThemeData &outData);

  /// セーブファイルから前回のお題 ID を読み込む
  /// @return 前回のお題 ID（ファイルがなければ空文字）
  static std::string LoadLastThemeId();

  /// セーブファイルに今回のお題 ID を保存する
  static void SaveLastThemeId(const std::string &themeId);
};
