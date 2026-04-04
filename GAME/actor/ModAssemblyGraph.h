#pragma once
#include "ModBody.h"
#include <unordered_map>
#include <vector>

/// <summary>
/// 部位がどの左右属性を持つかを表す
/// 親子接続時に、左用・右用・中央用の接続判定に使う
/// </summary>
enum class PartSide {
  Center = 0, // 中央部位
  Left,       // 左側部位
  Right,      // 右側部位
};

/// <summary>
/// 接続点の役割を表す
/// どの部位をどの接続点へつなぐべきかを判定するために使う
/// </summary>
enum class ConnectorRole {
  Generic = 0, // 汎用接続点
  Neck,        // 首接続用
  Shoulder,    // 肩接続用
  ArmJoint,    // 腕の関節接続用
  Hip,         // 腰接続用
  LegJoint,    // 脚の関節接続用
};

/// <summary>
/// 1つの接続点情報
/// 各 PartNode が持つ接続位置と、その接続点の用途を表す
/// </summary>
struct ConnectorNode {
  int id = -1;                                 // 接続点ID
  ConnectorRole role = ConnectorRole::Generic; // 接続点の役割
  PartSide side = PartSide::Center;            // 接続点の左右属性
  Vector3 localPosition{0.0f, 0.0f, 0.0f};     // 部位ローカル空間での接続位置
};

/// <summary>
/// 1部位ぶんの構成ノード
/// 改造体の親子構造を管理するために、部位情報・親情報・ローカル変換・接続点をまとめて持つ
/// </summary>
struct PartNode {
  int id = -1;                               // 部位ID
  ModBodyPart part = ModBodyPart::ChestBody; // 部位種類
  PartSide side = PartSide::Center;          // 部位の左右属性

  int parentId = -1;          // 親部位ID
  int parentConnectorId = -1; // 親側で接続しているコネクタID
  int selfConnectorId = -1;   // 自分側で接続しているコネクタID

  Transform localTransform = CreateDefaultTransform(); // 親基準のローカル変換

  bool required = false;                 // 削除不可の必須部位かどうか
  std::vector<ConnectorNode> connectors; // この部位が持つ接続点一覧
};

/// <summary>
/// 改造体の親子構造と接続関係を管理するクラス
/// 部位の追加、削除、付け替え、ローカル変換変更、ワールド位置計算を担当する
/// ModScene での部位構成編集の土台として使う
/// </summary>
class ModAssemblyGraph {
public:
  /// <summary>
  /// 初期人型構成を生成する
  /// Body、Neck、Head、左右の腕、左右の脚をまとめて作成し、編集開始時の基本形を作る
  /// </summary>
  void InitializeDefaultHumanoid();

  /// <summary>
  /// 現在の Body 部位IDを返す
  /// </summary>
  /// <returns>Body の部位ID。存在しない場合は -1</returns>
  int GetBodyId() const { return bodyId_; }

  /// <summary>
  /// 現在の Head 部位IDを返す
  /// </summary>
  /// <returns>Head の部位ID。存在しない場合は -1</returns>
  int GetHeadId() const { return headId_; }

  /// <summary>
  /// 全ノード一覧を参照で返す
  /// </summary>
  const std::unordered_map<int, PartNode> &GetNodes() const { return nodes_; }

  /// <summary>
  /// ノードIDを昇順に並べた一覧を返す
  /// </summary>
  std::vector<int> GetNodeIdsSorted() const;

  /// <summary>
  /// 指定した部位IDのノードを取得する
  /// </summary>
  const PartNode *FindNode(int partId) const;

  /// <summary>
  /// 指定した部位の中から接続点を取得する
  /// </summary>
  const ConnectorNode *FindConnector(int partId, int connectorId) const;

  /// <summary>
  /// 指定した部位を構造から削除する
  /// 部位種別に応じて、子の付け替えや子ごとの連鎖削除も行う
  /// </summary>
  bool RemovePart(int partId);

  /// <summary>
  /// 指定した部位の親を変更する（付け替え）
  /// </summary>
  /// <param name="newParentConnectorId">-1 の場合は自動選択</param>
  bool MovePart(int partId, int newParentId, int newParentConnectorId);

  /// <summary>
  /// 指定部位のローカル位置を設定する（親基準）
  /// </summary>
  bool SetPartLocalTranslate(int partId, const Vector3 &localTranslate);

  /// <summary>
  /// 指定部位のローカルスケールを設定する（Graph側の配置用）
  /// </summary>
  bool SetPartScale(int partId, const Vector3 &localScale);

  /// <summary>
  /// 腕セット（上腕+前腕）を追加する
  /// </summary>
  bool AddArmAssembly(PartSide side);

  /// <summary>
  /// 脚セット（腿+脛）を追加する
  /// </summary>
  bool AddLegAssembly(PartSide side);

    /// <summary>
  /// Neck + Head セットを追加する
  /// </summary>
  bool AddNeckPart();

  /// <summary>
  /// Head 単体追加は行わない
  /// 互換用に残すが内部では false を返す
  /// </summary>
  bool AddHeadPart();

  /// <summary>
  /// Body 部位を追加する（既存Bodyが無い場合のみ）
  /// </summary>
  bool AddBodyPart();

  /// <summary>
  /// 指定部位の直接の子一覧を返す
  /// </summary>
  std::vector<int> GetChildren(int parentId) const;

  /// <summary>
  /// 指定部位のワールド位置を計算して返す
  /// 親子構造をたどり、親のスケール影響も反映する
  /// </summary>
  Vector3 ComputeWorldPosition(int partId) const;

  /// <summary>
  /// 親部位上での子部位のデフォルト接続位置を返す（参照用）
  /// </summary>
  Vector3 GetDefaultAttachLocal(ModBodyPart parentPart, ModBodyPart childPart,
                                PartSide childSide) const {
    return MakeDefaultAttachLocal(parentPart, childPart, childSide);
  }

  /// <summary>
  /// 指定種類の最初の部位IDを返す
  /// </summary>
  int FindFirstPartId(ModBodyPart part, int excludeId = -1) const;

private:
  /// <summary>
  /// 部位ノードを1つ追加する
  /// 部位種類、左右属性、親情報、初期transform、接続点を設定して登録する
  /// </summary>
  int AddPart(ModBodyPart part, PartSide side, int parentId, bool required);

  /// <summary>
  /// 指定部位用のデフォルト接続点を生成する
  /// </summary>
  std::vector<ConnectorNode> MakeDefaultConnectors(ModBodyPart part,
                                                   PartSide side);

  /// <summary>
  /// 指定した子部位を接続するのに適した親側コネクタIDを探す
  /// </summary>
  int FindConnectorIdForChild(int parentId, ModBodyPart childPart,
                              PartSide childSide) const;

  /// <summary>
  /// 部位ごとの初期ローカル位置を返す
  /// </summary>
  Vector3 MakeDefaultLocalTranslate(ModBodyPart part, PartSide side) const;

  /// <summary>
  /// 親子の接続組み合わせが有効かを判定する
  /// </summary>
  bool CanParentChild(ModBodyPart parent, ModBodyPart child) const;

  /// <summary>
  /// 親側と子側の左右属性が両立するかを判定する
  /// </summary>
  bool IsSideCompatible(PartSide parentSide, PartSide childSide) const;

  /// <summary>
  /// 指定部位の子を別の親へ付け替える（削除時の救済用）
  /// </summary>
  bool ReattachChildrenOf(int removedPartId);

  /// <summary>
  /// 指定子部位に対し、接続可能な候補から最も近い親を探す
  /// </summary>
  int FindBestParentForChild(int childId) const;

  /// <summary>
  /// 現在存在している脚根元（腿）の数を数える
  /// </summary>
  int CountLegRoots() const;

  /// <summary>
  /// 現在存在しているHeadの数を数える
  /// </summary>
  int CountHeads() const;

  /// <summary>
  /// 指定部位が脚根元（腿）かどうか
  /// </summary>
  bool IsLegRoot(ModBodyPart part) const;

  /// <summary>
  /// 指定部位が腕根元（上腕）かどうか
  /// </summary>
  bool IsArmRoot(ModBodyPart part) const;

  /// <summary>
  /// 指定部位が四肢根元（腕根元/脚根元）かどうか
  /// </summary>
  bool IsLimbRoot(ModBodyPart part) const;

  /// <summary>
  /// 指定部位以下の部分木をまとめて削除する
  /// </summary>
  bool RemoveSubtree(int partId);

  /// <summary>
  /// Body削除時に、直下の腕脚セットを削除し、それ以外は再接続する
  /// </summary>
  bool RemoveBodyAttachedLimbsAndReattachOthers(int bodyPartId);

  /// <summary>
  /// 指定部位がHeadかどうか
  /// </summary>
  bool IsHead(ModBodyPart part) const;

  /// <summary>
  /// 削除時に子もまとめて削除すべき部位かどうか
  /// </summary>
  bool ShouldCascadeDeleteChildren(ModBodyPart part) const;

  /// <summary>
  /// 子部位が親に求める接続点役割を返す
  /// </summary>
  ConnectorRole DesiredParentConnectorRoleForChild(ModBodyPart childPart) const;

  /// <summary>
  /// 接続点が子部位にどれだけ適しているかを点数化する
  /// </summary>
  int ScoreConnectorMatch(const ConnectorNode &connector,
                          ConnectorRole desiredRole, PartSide childSide) const;

  /// <summary>
  /// 指定部位の子孫を再帰的に削除する
  /// </summary>
  bool RemoveChildrenRecursive(int partId);

  /// <summary>
  /// Body/Head の管理IDをnodes_から再探索する
  /// </summary>
  void RefreshManagedPartIds();

  /// <summary>
  /// 指定部位がBody直下に戻す候補かどうか
  /// </summary>
  bool IsBodyChildPart(ModBodyPart part) const;

  /// <summary>
  /// 指定部位がBodyに接続可能なHeadかどうか
  /// </summary>
  bool IsHeadAttachableToBody(ModBodyPart part) const;

  /// <summary>
  /// 付け替え時に優先的に接続したい親を選ぶ（無ければ近さベースへフォールバック）
  /// </summary>
  int FindPreferredParentForChild(int childId, int removedPartId = -1) const;

  /// <summary>
  /// Body復帰時に、関連部位をBody配下へ戻す（互換/救済処理）
  /// </summary>
  void ReattachPartsForBodyRestore(int newBodyId);

  /// <summary>
  /// Headの親子関係を正規化する（Neck優先 → Body → 親なし）
  /// </summary>
  void NormalizeHeadLinks();

  /// <summary>
  /// Body配下に置くべき部位をBodyへ戻す
  /// </summary>
  void NormalizeBodyChildLinks();

  /// <summary>
  /// 指定部位を親なし状態にする
  /// </summary>
  void DetachPartFromParent(int childId);

  /// <summary>
  /// 指定部位を親へ接続する（接続点の自動選択と初期位置補正を含む）
  /// </summary>
  void AttachPartToParent(int childId, int parentId);

  /// <summary>
  /// 親部位上での子部位のデフォルト接続位置を返す
  /// </summary>
  Vector3 MakeDefaultAttachLocal(ModBodyPart parentPart, ModBodyPart childPart,
                                 PartSide childSide) const;

  /// <summary>
  /// 子部位の接続位置を、現在の親に対するデフォルトへ戻す
  /// </summary>
  void ResetChildAttachLocal(PartNode &child);

private:
  std::unordered_map<int, PartNode> nodes_; // 全部位ノード一覧
  int nextPartId_ = 1;                      // 次に発行する部位ID
  int nextConnectorId_ = 1;                 // 次に発行する接続点ID

  int bodyId_ = -1; // 現在の Body 部位ID
  int headId_ = -1; // 現在の Head 部位ID
};