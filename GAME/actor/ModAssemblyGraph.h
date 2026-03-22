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
  int id = -1;                          // 部位ID
  ModBodyPart part = ModBodyPart::Body; // 部位種類
  PartSide side = PartSide::Center;     // 部位の左右属性

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
  /// Body、Neck、Head、左右の腕、左右の脚をまとめて作成し、
  /// 編集開始時の基本形を作るときに使う
  /// </summary>
  void InitializeDefaultHumanoid();

  /// <summary>
  /// 現在の Body 部位IDを返す
  /// Body を基準に追加や接続を行う処理で使う
  /// </summary>
  /// <returns>Body の部位ID。存在しない場合は -1</returns>
  int GetBodyId() const { return bodyId_; }

  /// <summary>
  /// 現在の Head 部位IDを返す
  /// Head を基準に付け替えや補助判定を行う処理で使う
  /// </summary>
  /// <returns>Head の部位ID。存在しない場合は -1</returns>
  int GetHeadId() const { return headId_; }

  /// <summary>
  /// 全ノード一覧を参照で返す
  /// 外部で現在の構造を走査したいときに使う
  /// </summary>
  /// <returns>部位IDをキーにしたノード一覧</returns>
  const std::unordered_map<int, PartNode> &GetNodes() const { return nodes_; }

  /// <summary>
  /// ノードIDを昇順に並べた一覧を返す
  /// 安定した順番で走査したいときに使う
  /// </summary>
  /// <returns>昇順の部位ID一覧</returns>
  std::vector<int> GetNodeIdsSorted() const;

  /// <summary>
  /// 指定した部位IDのノードを取得する
  /// 部位の状態を参照したいときに使う
  /// </summary>
  /// <param name="partId">取得したい部位ID</param>
  /// <returns>ノードへのポインタ。存在しない場合は nullptr</returns>
  const PartNode *FindNode(int partId) const;

  /// <summary>
  /// 指定した部位の中から接続点を取得する
  /// 接続点の位置や役割を参照したいときに使う
  /// </summary>
  /// <param name="partId">対象部位ID</param>
  /// <param name="connectorId">取得したい接続点ID</param>
  /// <returns>接続点へのポインタ。見つからない場合は nullptr</returns>
  const ConnectorNode *FindConnector(int partId, int connectorId) const;

  /// <summary>
  /// 指定した部位を構造から削除する
  /// 部位種別に応じて、子の付け替えや子ごとの連鎖削除も行う
  /// </summary>
  /// <param name="partId">削除したい部位ID</param>
  /// <returns>削除できたら true</returns>
  bool RemovePart(int partId);

  /// <summary>
  /// 指定した部位の親を変更する
  /// 部位を別の接続先へ付け替えたいときに使う
  /// </summary>
  /// <param name="partId">移動する部位ID</param>
  /// <param name="newParentId">新しい親部位ID</param>
  /// <param name="newParentConnectorId">新しい親側接続点ID。-1
  /// の場合は自動選択</param> <returns>移動できたら true</returns>
  bool MovePart(int partId, int newParentId, int newParentConnectorId);

  /// <summary>
  /// 指定部位のローカル位置を設定する
  /// 親基準での配置を変更したいときに使う
  /// </summary>
  /// <param name="partId">対象部位ID</param>
  /// <param name="localTranslate">設定するローカル位置</param>
  /// <returns>設定できたら true</returns>
  bool SetPartLocalTranslate(int partId, const Vector3 &localTranslate);

  /// <summary>
  /// 指定部位のローカルスケールを設定する
  /// Graph 側で保持する配置用スケールを変更したいときに使う
  /// </summary>
  /// <param name="partId">対象部位ID</param>
  /// <param name="localScale">設定するローカルスケール</param>
  /// <returns>設定できたら true</returns>
  bool SetPartScale(int partId, const Vector3 &localScale);

  /// <summary>
  /// 腕セットを追加する
  /// 上腕と前腕を1セットで生成し、Body の下へ接続する
  /// </summary>
  /// <param name="side">追加する側。Left または Right</param>
  /// <returns>追加できたら true</returns>
  bool AddArmAssembly(PartSide side);

  /// <summary>
  /// 脚セットを追加する
  /// 腿と脛を1セットで生成し、Body の下へ接続する
  /// </summary>
  /// <param name="side">追加する側。Left または Right</param>
  /// <returns>追加できたら true</returns>
  bool AddLegAssembly(PartSide side);

  /// <summary>
  /// Neck 部位を1つ追加する
  /// Body があれば Body の下へ、無ければ Head の下へ追加する
  /// </summary>
  /// <returns>追加できたら true</returns>
  bool AddNeckPart();

  /// <summary>
  /// Head 部位を1つ追加する
  /// </summary>
  /// <returns>追加できたら true</returns>
  bool AddHeadPart();

  /// <summary>
  /// Body 部位を1つ追加する
  /// 既に Body が存在しない場合のみ追加でき、Head の下へ接続する
  /// </summary>
  /// <returns>追加できたら true</returns>
  bool AddBodyPart();

  /// <summary>
  /// 指定部位の直接の子一覧を返す
  /// 削除処理や再接続処理で使う
  /// </summary>
  /// <param name="parentId">親部位ID</param>
  /// <returns>直接の子部位ID一覧</returns>
  std::vector<int> GetChildren(int parentId) const;

  /// <summary>
  /// 指定部位のワールド位置を計算して返す
  /// 親子構造をたどりながら、ローカル位置と親スケールを考慮して求める
  /// </summary>
  /// <param name="partId">対象部位ID</param>
  /// <returns>ワールド位置</returns>
  Vector3 ComputeWorldPosition(int partId) const;

private:
  /// <summary>
  /// 部位ノードを1つ追加する
  /// 部位種類、左右属性、親情報、初期 transform、接続点を設定して nodes_
  /// に登録する
  /// </summary>
  /// <param name="part">追加する部位種類</param>
  /// <param name="side">左右属性</param>
  /// <param name="parentId">親部位ID</param>
  /// <param name="required">削除不可部位かどうか</param>
  /// <returns>追加された部位ID</returns>
  int AddPart(ModBodyPart part, PartSide side, int parentId, bool required);

  /// <summary>
  /// 指定部位用のデフォルト接続点を生成する
  /// 部位ごとに、首・肩・脚などの役割に応じた接続点を用意する
  /// </summary>
  /// <param name="part">対象部位種類</param>
  /// <param name="side">左右属性</param>
  /// <returns>接続点一覧</returns>
  std::vector<ConnectorNode> MakeDefaultConnectors(ModBodyPart part,
                                                   PartSide side);

  /// <summary>
  /// 指定した子部位をつなぐのに最も適した親側コネクタIDを探す
  /// 接続点の役割と左右属性を見て最適候補を選ぶ
  /// </summary>
  /// <param name="parentId">親部位ID</param>
  /// <param name="childPart">子部位種類</param>
  /// <param name="childSide">子部位の左右属性</param>
  /// <returns>親側コネクタID。見つからない場合は -1</returns>
  int FindConnectorIdForChild(int parentId, ModBodyPart childPart,
                              PartSide childSide) const;

  /// <summary>
  /// 部位ごとの初期ローカル位置を返す
  /// 部位追加時の基本配置を決めるために使う
  /// </summary>
  /// <param name="part">対象部位種類</param>
  /// <param name="side">左右属性</param>
  /// <returns>初期ローカル位置</returns>
  Vector3 MakeDefaultLocalTranslate(ModBodyPart part, PartSide side) const;

  /// <summary>
  /// 親部位と子部位の接続組み合わせが有効かを判定する
  /// 例えば Body には Neck や腕根元や脚根元のみ接続可能にする
  /// </summary>
  /// <param name="parent">親部位種類</param>
  /// <param name="child">子部位種類</param>
  /// <returns>接続可能なら true</returns>
  bool CanParentChild(ModBodyPart parent, ModBodyPart child) const;

  /// <summary>
  /// 親側と子側の左右属性が両立するかを判定する
  /// 左右一致または中央接続を許可する
  /// </summary>
  /// <param name="parentSide">親側の左右属性</param>
  /// <param name="childSide">子側の左右属性</param>
  /// <returns>接続可能なら true</returns>
  bool IsSideCompatible(PartSide parentSide, PartSide childSide) const;

  /// <summary>
  /// 指定部位の子を別の親へ付け替える
  /// 部位削除後に子を構造内へ残したいときに使う
  /// </summary>
  /// <param name="removedPartId">削除対象だった部位ID</param>
  /// <returns>付け替えできたら true</returns>
  bool ReattachChildrenOf(int removedPartId);

  /// <summary>
  /// 指定した子部位に対して最も適した新しい親を探す
  /// 接続可能な候補の中から、現在位置に最も近い部位を選ぶ
  /// </summary>
  /// <param name="childId">付け替え対象の子部位ID</param>
  /// <returns>新しい親部位ID。見つからない場合は -1</returns>
  int FindBestParentForChild(int childId) const;

  /// <summary>
  /// 現在存在している脚根元の数を数える
  /// 最低本数制限の判定に使う
  /// </summary>
  /// <returns>脚根元数</returns>
  int CountLegRoots() const;

  /// <summary>
  /// 現在存在している腕根元の数を数える
  /// </summary>
  /// <returns>腕根元数</returns>
  int CountHeads() const;

  /// <summary>
  /// 指定部位が脚根元かどうかを判定する
  /// 腿部位を脚の開始点として扱う
  /// </summary>
  /// <param name="part">判定対象部位</param>
  /// <returns>脚根元なら true</returns>
  bool IsLegRoot(ModBodyPart part) const;

  /// <summary>
  /// 指定部位が腕根元かどうかを判定する
  /// 上腕部位を腕の開始点として扱う
  /// </summary>
  /// <param name="part">判定対象部位</param>
  /// <returns>腕根元なら true</returns>
  bool IsArmRoot(ModBodyPart part) const;

  /// <summary>
  /// 指定部位が腕根元または脚根元かを判定する
  /// Body 直下の四肢セット判定に使う
  /// </summary>
  /// <param name="part">判定対象部位</param>
  /// <returns>四肢根元なら true</returns>
  bool IsLimbRoot(ModBodyPart part) const;

  /// <summary>
  /// 指定部位以下の部分木をまとめて削除する
  /// 子を再接続せず、構造ごと消したいときに使う
  /// </summary>
  /// <param name="partId">削除開始部位ID</param>
  /// <returns>削除できたら true</returns>
  bool RemoveSubtree(int partId);

  /// <summary>
  /// Body 直下の腕・脚セットを削除し、それ以外の子は再接続する
  /// Body 削除時の特別処理として使う
  /// </summary>
  /// <param name="bodyPartId">削除対象の Body 部位ID</param>
  /// <returns>処理できたら true</returns>
  bool RemoveBodyAttachedLimbsAndReattachOthers(int bodyPartId);

  /// <summary>
  /// 指定部位が Head かどうかを判定する
  /// Head の削除禁止判定などに使う
  /// </summary>
  /// <param name="part">判定対象部位</param>
  /// <returns>Head なら true</returns>
  bool IsHead(ModBodyPart part) const;

  /// <summary>
  /// 指定部位が削除の際に子もまとめて消すべきかを判定する
  /// </summary>
  /// <param name="part">判定対象部位</param>
  /// <returns>子も連鎖削除すべきなら true</returns>
  bool ShouldCascadeDeleteChildren(ModBodyPart part) const;

  /// <summary>
  /// 指定した子部位が親に求める接続点役割を返す
  /// 最適な親コネクタ選択に使う
  /// </summary>
  /// <param name="childPart">子部位種類</param>
  /// <returns>必要な親コネクタ役割</returns>
  ConnectorRole DesiredParentConnectorRoleForChild(ModBodyPart childPart) const;

  /// <summary>
  /// 接続点が子部位にどれだけ適しているかを点数化する
  /// 役割一致や左右一致の優先度をつけて比較するために使う
  /// </summary>
  /// <param name="connector">評価対象の接続点</param>
  /// <param name="desiredRole">子が求める接続点役割</param>
  /// <param name="childSide">子部位の左右属性</param>
  /// <returns>適合度スコア</returns>
  int ScoreConnectorMatch(const ConnectorNode &connector,
                          ConnectorRole desiredRole, PartSide childSide) const;

  /// <summary>
  /// 指定部位の子孫を再帰的に削除する
  /// 腕根元や脚根元を消すときに、先にぶら下がっている子を消すために使う
  /// </summary>
  /// <param name="partId">起点部位ID</param>
  /// <returns>削除できたら true</returns>
  bool RemoveChildrenRecursive(int partId);

  /// <summary>
  /// Body や Head の管理IDを必要に応じて無効化する
  /// </summary>
  void RefreshManagedPartIds();

private:
  std::unordered_map<int, PartNode> nodes_; // 全部位ノード一覧
  int nextPartId_ = 1;                      // 次に発行する部位ID
  int nextConnectorId_ = 1;                 // 次に発行する接続点ID

  int bodyId_ = -1; // 現在の Body 部位ID
  int headId_ = -1; // 現在の Head 部位ID
};