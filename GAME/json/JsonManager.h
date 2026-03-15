#pragma once
#include <optional>
#include <string>
#include <vector>

#include "externals/nlohmann/json.hpp"

/// JSON ファイルの読み書きを行う汎用ユーティリティクラス
/// 内部状態を持たず、すべてstatic関数で提供する
class JsonManager {
public:

  /// <summary>
  /// JSONファイルを読み込む
  /// </summary>
  /// <param name="filePath">読み込むファイルのパス</param>
  /// <returns>成功時はnlohmann::json、失敗時はstd::nullopt</returns>
  static std::optional<nlohmann::json> Load(const std::string &filePath);

  /// <summary>
  /// JSON データをファイルに書き出す（インデント付き整形）
  /// </summary>
  /// <param name="filePath">書き出すファイルのパス</param>
  /// <param name="data">書き出すJSONデータ</param>
  /// <returns>成功時はtrue、失敗時はfalseを返す</returns>
  static bool Save(const std::string &filePath, const nlohmann::json &data);

  /// <summary>
  /// 指定フォルダ内の.jsonファイルパス一覧を取得する（サブフォルダも再帰探索）
  /// </summary>
  /// <param name="directoryPath">探索するフォルダのパス</param>
  /// <returns>.jsonファイルのパス一覧</returns>
  static std::vector<std::string> GetFileList(const std::string &directoryPath);

  /// <summary>
  /// ファイルの存在チェック
  /// </summary>
  /// <param name="filePath">チェックするファイルのパス</param>
  /// <returns>存在すればtrueを返す</returns>
  static bool Exists(const std::string &filePath);

private:
  // インスタンス化禁止
  JsonManager() = delete;
};
