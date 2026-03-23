#pragma once
#include "JudgeData.h"
#include <string>
#include <vector>

#include "externals/nlohmann/json.hpp"

/// 審査員の読み込み・ランダム選出を管理するクラス
class JudgeManager {
public:
  /// コンストラクタ：指定フォルダ内の全審査員 JSON を読み込む
  /// @param judgesDirectory 審査員 JSON が格納されたフォルダのパス
  JudgeManager(const std::string &judgesDirectory);

  ~JudgeManager() = default;

  /// ランダムで審査員を選出する
  /// @param count 選出する人数
  /// @return 選出された審査員データへのポインタ配列
  std::vector<const JudgeData *> SelectRandom(int count = 3);

  /// 現在選ばれている審査員を取得する
  /// @return 現在の審査員データへのポインタ配列
  std::vector<const JudgeData *> GetCurrentJudges() const;

  /// 読み込み済みの審査員数を取得する
  size_t GetJudgeCount() const;

private:
  // 読み込んだ全審査員データ
  std::vector<JudgeData> judges_;

  // 現在選ばれている審査員のインデックス
  std::vector<int> currentIndices_;

  /// JSON からパーツ名をインデックスに変換する
  /// @return 成功時はインデックス、失敗時は -1
  static int PartNameToIndex(const std::string &name);

  /// 1 つの JSON オブジェクトを JudgeData に変換する
  /// @return 成功時は true
  static bool ParseJudgeData(const nlohmann::json &json, JudgeData &outData);
};
