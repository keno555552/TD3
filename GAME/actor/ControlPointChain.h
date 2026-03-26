#pragma once
#include "ModControlPointTypes.h"
#include "Vector3.h"
#include <vector>

/// <summary>
/// 1つの関節ノード情報
/// 完全なチェーン構造へ移行するための最小単位
/// </summary>
struct ControlPointNode {
  ModControlPointRole role = ModControlPointRole::None;
  int parentIndex = -1;

  Vector3 localPosition{0.0f, 0.0f, 0.0f};

  float radius = 0.08f;
  bool movable = true;

  bool isConnectionPoint = false;
  bool acceptsParent = false;
  bool acceptsChild = false;
};

/// <summary>
/// 肩→肘→手首、股関節→膝→足首のような
/// 親子付き関節チェーンを管理するクラス
/// </summary>
class ControlPointChain {
public:
  /// <summary>
  /// 全ノードを消去する
  /// </summary>
  void Clear();

  /// <summary>
  /// 腕チェーンを初期化する
  /// Root -> Bend -> End
  /// </summary>
  void BuildArmChain();

  /// <summary>
  /// 脚チェーンを初期化する
  /// Root -> Bend -> End
  /// </summary>
  void BuildLegChain();

  /// <summary>
  /// 胴体チェーンを初期化する
  /// Chest -> Belly -> Waist
  /// </summary>
  void BuildTorsoChain();

  /// <summary>
  /// 頭チェーンを初期化する
  /// LowerNeck -> UpperNeck -> HeadCenter
  /// </summary>
  void BuildHeadChain();

  /// <summary>
  /// 指定 role のノード index を返す
  /// 見つからない場合は -1
  /// </summary>
  int FindIndex(ModControlPointRole role) const;

  /// <summary>
  /// ノードを移動する
  /// </summary>
  bool MovePoint(size_t index, const Vector3 &newLocalPosition);

  /// <summary>
  /// 指定ノードのローカル座標を返す
  /// </summary>
  Vector3 GetLocalPosition(size_t index) const;

  /// <summary>
  /// 親子をたどったワールド座標を返す
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

private:
  /// <summary>
  /// 必要なら階層再計算を行う
  /// 現状はローカル親子だけを使うので空実装
  /// </summary>
  void UpdateHierarchy();

private:
  std::vector<ControlPointNode> nodes_;
};