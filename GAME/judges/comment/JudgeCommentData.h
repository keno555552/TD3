#pragma once
#include <string>
#include <unordered_map>
#include <vector>

/// 審査員1人分のコメントデータ
struct JudgeCommentSet {
  std::string judgeId;
  std::vector<std::string> highStar;  // ★4〜5 のときのコメント
  std::vector<std::string> midStar;   // ★3 のときのコメント
  std::vector<std::string> lowStar;   // ★1〜2 のときのコメント
};

/// 全審査員のコメントデータ
/// key: judgeId
struct JudgeCommentTable {
  std::unordered_map<std::string, JudgeCommentSet> comments;
};

/// 審査員1人分の生成結果
struct JudgeCommentResult {
  std::string judgeId;
  std::string comment;
  int star = 0;
};
