#pragma once
#include "ModControlPointTypes.h"
#include "Vector3.h"
#include <vector>

/// <summary>
/// 1つの関節ノード情報
/// 親子構造を持つ操作点の最小単位
/// </summary>
struct ControlPointNode {
  ModControlPointRole role = ModControlPointRole::None; // 操作点の役割
  int parentIndex = -1; // 親ノードindex（-1でルート）

  Vector3 localPosition{0.0f, 0.0f, 0.0f}; // 親基準ローカル位置

  float radius = 0.08f; // ピック/表示サイズ
  bool movable = true;  // 移動可能か

  bool isConnectionPoint = false; // 接続点として扱うか
  bool acceptsParent = false;     // 親接続を受けるか
  bool acceptsChild = false;      // 子接続を受けるか
};

/// <summary>
/// 肩→肘→手首、股関節→膝→足首、首元→首先→頭先のような親子付き関節チェーンを管理するクラス
/// - 各ノードは parentIndex と localPosition により階層を構成する
/// - Move時は役割に応じた制約（長さ/半径/向き）を適用できる
/// </summary>
class ControlPointChain {
public:
  /// <summary>
  /// 全ノードを消去する
  /// </summary>
  void Clear();

  /// <summary>
  /// 腕チェーンを既定値で構築する（Root -> Bend -> End）
  /// </summary>
  void BuildArmChain();

  /// <summary>
  /// 脚チェーンを既定値で構築する（Root -> Bend -> End）
  /// </summary>
  void BuildLegChain();

  /// <summary>
  /// 胴体チェーンを既定値で構築する（Chest -> Belly -> Waist）
  /// </summary>
  void BuildTorsoChain();

  /// <summary>
  /// 首チェーンを既定値で構築する（Root -> Bend -> End）
  /// Neck が owner となる
  /// </summary>
  void BuildNeckChain();

  /// <summary>
  /// 指定roleのノードindexを返す。見つからない場合は -1
  /// </summary>
  int FindIndex(ModControlPointRole role) const;

  /// <summary>
  /// 指定ノードを移動する
  /// 役割に応じて制約を適用し、必要なら子ノードも追従させる
  /// </summary>
  bool MovePoint(size_t index, const Vector3 &newLocalPosition);

  /// <summary>
  /// 指定ノードのローカル座標（親基準）を返す
  /// </summary>
  Vector3 GetLocalPosition(size_t index) const;

  /// <summary>
  /// 指定ノードのワールド座標（チェーン内）を返す
  /// 親をたどって localPosition を加算する
  /// </summary>
  Vector3 GetWorldPosition(size_t index) const;

  /// <summary>
  /// ノード一覧の読み取り専用参照を返す
  /// </summary>
  const std::vector<ControlPointNode> &GetNodes() const { return nodes_; }

  /// <summary>
  /// ノード一覧の編集用参照を返す
  /// </summary>
  std::vector<ControlPointNode> &GetNodes() { return nodes_; }

  /// <summary>
  /// 階層キャッシュ更新用フック
  /// 現状は逐次計算のため空実装（将来キャッシュ化する場合の差し込み口）
  /// </summary>
  void UpdateHierarchy();

private:
  std::vector<ControlPointNode> nodes_;
};