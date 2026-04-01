#pragma once
#include <string>
#include <unordered_map>
#include <vector>

/// パーツ固有コメント
/// key: パーツグループ名（"ChestBody", "Neck", "UpperArm" など）
/// value: 方向別コメント（"bigger" と "smaller"）
struct PartDirectionComments {
  std::vector<std::string> bigger;
  std::vector<std::string> smaller;
};

/// 観客コメントの全データ
struct AudienceCommentData {

  std::unordered_map<std::string, PartDirectionComments> partComments;

  /// テンプレートコメント（テンション別）
  /// key: "high", "medium", "low"
  /// value: コメント候補リスト
  std::unordered_map<std::string, std::vector<std::string>> templateComments;

  /// 改造なし時のコメント
  std::vector<std::string> noChangeComments;
};

/// 観客3人分の生成結果
struct AudienceResult {
  std::string comments[3];
};
