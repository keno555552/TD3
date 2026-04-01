#pragma once
#include "JudgeCommentData.h"
#include "GAME/score/ScoreResult.h"
#include "externals/nlohmann/json.hpp"
#include <string>

/// 審査員コメントの読み込み・生成を行うクラス
/// 内部状態を持たず、すべて static 関数で提供する
class JudgeCommentManager {
public:
  /// 審査員コメントデータをフォルダから一括読み込みする
  /// @param dirPath コメントJSONが入ったフォルダパス
  /// @param outTable 読み込み先
  /// @return 成功した読み込み数
  static int LoadCommentTable(const std::string &dirPath,
                              JudgeCommentTable &outTable);

  /// スコア結果から審査員3人分のコメントを生成する
  /// @param table コメントデータ
  /// @param result スコア結果（審査員の個別★が入っている）
  /// @return 生成された JudgeCommentResult 3人分
  static std::vector<JudgeCommentResult> GenerateComments(
      const JudgeCommentTable &table,
      const ScoreResult &result);

private:
  JudgeCommentManager() = delete;

  /// 1ファイルからコメントセットを読み込む
  static bool LoadSingleComment(const std::string &filePath,
                                JudgeCommentSet &outSet);

  /// ★の値からコメントカテゴリを選択する
  /// @return "high", "mid", "low"
  static std::string StarToCategory(int star);
};
