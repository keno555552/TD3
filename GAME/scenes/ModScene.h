#pragma once
#include "../effect/Fade.h"
#include "BaseScene.h"
#include "GAME/actor/ModAssemblyGraph.h"
#include "GAME/actor/ModBody.h"
#include "Object/Object.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/// <summary>
/// 改造シーン本体
/// 部位の追加、削除、付け替え、パラメータ編集を行い、次のシーンへ渡す改造結果を作る
/// AssemblyGraph で構造を管理し、Object と ModBody で見た目を管理する
/// </summary>
class ModScene : public BaseScene {
public:
  /// <summary>
  /// 改造シーンを初期化する
  /// ライト、カメラ、共有改造データ、初期部位構成を準備する
  /// </summary>
  /// <param name="system">エンジン本体</param>
  ModScene(kEngine *system);

  /// <summary>
  /// 改造シーンを終了する
  /// 作成したカメラとライトを破棄する
  /// </summary>
  ~ModScene();

  /// <summary>
  /// シーンの更新処理
  /// 入力処理、構造変更、見た目反映、共有データ同期、シーン遷移管理を行う
  /// </summary>
  void Update() override;

  /// <summary>
  /// シーンの描画処理
  /// 改造体本体、ImGui、フェードを描画する
  /// </summary>
  void Draw() override;

private:
  Light *light1_ = nullptr; // シーン内で使用するライト

  Camera *camera_ = nullptr;           // 通常カメラ
  DebugCamera *debugCamera_ = nullptr; // デバッグカメラ
  Camera *usingCamera_ = nullptr;      // 現在使用中のカメラ
  bool useDebugCamera_ = true;         // デバッグカメラを使うかどうか

  ModAssemblyGraph assembly_;       // 部位構造と親子関係を管理するグラフ
  std::vector<int> orderedPartIds_; // 描画や更新順に使う部位ID一覧

  std::unordered_map<int, int> modModelHandles_; // 部位IDごとのモデルハンドル
  std::unordered_map<int, std::unique_ptr<Object>>
      modObjects_;                             // 部位IDごとのObject
  std::unordered_map<int, ModBody> modBodies_; // 部位IDごとの改造パラメータ制御

  std::unique_ptr<ModBodyCustomizeData> customizeData_ =
      nullptr; // シーン間共有用の改造結果

  Fade fade_;                      // シーン開始・終了時のフェード演出
  bool isStartTransition_ = false; // フェードアウトによる遷移を開始したかどうか
  std::string selectedPrompt_;     // 現在のお題文

  int selectedPartId_ = -1;      // 現在選択中の部位ID
  int reattachParentId_ = -1;    // 付け替え先として選択中の親部位ID
  int reattachConnectorId_ = -1; // 付け替え先として選択中の親コネクタID

private:
  /// <summary>
  /// 使用するカメラを更新する
  /// デバッグカメラと通常カメラを切り替え、必要ならデバッグ操作も行う
  /// </summary>
  void CameraPart();

  /// <summary>
  /// 初期の改造部位Object一式を作成する
  /// AssemblyGraph を初期人型で作り、その内容に合わせて Object 群を生成する
  /// </summary>
  void SetupModObjects();

  /// <summary>
  /// 初期状態の部位配置を整える
  /// 全部位のローカルスケールを初期値へそろえる
  /// </summary>
  void SetupInitialLayout();

  /// <summary>
  /// AssemblyGraph の内容に合わせて Object 一覧を同期する
  /// 不要になった Object を消し、新しく追加された部位の Object を生成する
  /// </summary>
  void SyncObjectsWithAssembly();

  /// <summary>
  /// 指定ノード用の Object を1つ生成する
  /// モデル読み込み、Object 初期化、ModBody 初期化をまとめて行う
  /// </summary>
  /// <param name="partId">生成対象の部位ID</param>
  /// <param name="node">生成対象ノード</param>
  void CreateObjectForNode(int partId, const PartNode &node);

  /// <summary>
  /// AssemblyGraph の親子構造を Scene 上の Object 階層へ反映する
  /// 親コネクタと子コネクタを使って root の位置を再計算する
  /// </summary>
  void ApplyAssemblyToSceneHierarchy();

  /// <summary>
  /// 現在の共有改造データをシーンへ読み込む
  /// 新方式の partId 単位データを優先し、無ければ旧方式データを使う
  /// </summary>
  void LoadCustomizeData();

  /// <summary>
  /// 現在のシーン状態を共有改造データへ書き戻す
  /// 各部位の親子関係、ローカル変換、改造パラメータを partInstances に保存する
  /// </summary>
  void SyncCustomizeDataFromScene();

  /// <summary>
  /// 新方式の partInstances から旧方式の固定配列データを再構築する
  /// 旧方式参照コードとの互換維持のために使う
  /// </summary>
  void RebuildLegacyCustomizeDataFromInstances();

  /// <summary>
  /// 全部位の改造パラメータを初期状態へ戻す
  /// scale、length、enabled などを各部位でリセットする
  /// </summary>
  void ResetModBodies();

  /// <summary>
  /// 現在選択中部位の改造パラメータを初期状態へ戻す
  /// 選択中部位だけ個別に見た目をリセットしたいときに使う
  /// </summary>
  void ResetSelectedPartParams();

  /// <summary>
  /// 部位構造と見た目を初期人型へ戻す
  /// AssemblyGraph、Object、改造パラメータ、共有データをまとめて初期化する
  /// </summary>
  void ResetToDefaultHumanoid();

  /// <summary>
  /// 改造部位Objectを更新する
  /// 階層反映、見た目反映、Object 更新を順に行う
  /// </summary>
  void UpdateModObjects();

  /// <summary>
  /// 改造部位Objectを描画する
  /// orderedPartIds_ の順に各 Object を描画する
  /// </summary>
  void DrawModObjects();

  /// <summary>
  /// 指定した部位を選択状態にする
  /// 付け替え用の一時選択状態も同時に初期化する
  /// </summary>
  /// <param name="partId">選択する部位ID</param>
  void SelectPart(int partId);

  /// <summary>
  /// 現在の選択状態を有効な部位へ補正する
  /// 選択中部位が存在しない場合は、選択可能な部位を先頭から選び直す
  /// </summary>
  void EnsureValidSelection();

  /// <summary>
  /// 現在選択中の部位を削除する
  /// 前腕や脛が選ばれている場合は、セットの根元部位へ対象を補正して削除する
  /// </summary>
  void DeleteSelectedPart();

  /// <summary>
  /// 現在選択中の部位を別の親へ付け替える
  /// reattachParentId_ と reattachConnectorId_ の設定内容を使って MovePart
  /// を行う
  /// </summary>
  void ReattachSelectedPart();

  /// <summary>
  /// 現在選択中の部位をローカル位置で少し移動する
  /// 矢印キーによる微調整で使う
  /// </summary>
  /// <param name="delta">加算する移動量</param>
  void NudgeSelectedPart(const Vector3 &delta);

  /// <summary>
  /// 構造操作時に実際に処理すべき部位IDを返す
  /// 前腕なら上腕、脛なら腿へ補正し、セット単位操作を行えるようにする
  /// </summary>
  /// <param name="partId">基準にする部位ID</param>
  /// <returns>実際に操作する部位ID</returns>
  int ResolveAssemblyOperationPartId(int partId) const;

#ifdef USE_IMGUI
  /// <summary>
  /// 改造シーン全体の ImGui を描画する
  /// 部位追加、全体リセット、部位一覧、選択部位編集をまとめて表示する
  /// </summary>
  void DrawModGui();

  /// <summary>
  /// 現在選択中部位の編集UIを描画する
  /// ローカル位置、見た目パラメータ、付け替え、削除、個別リセットを扱う
  /// </summary>
  void DrawSelectedPartGui();

  /// <summary>
  /// 部位一覧のUIを描画する
  /// 現在存在している部位を列挙し、クリックで選択できるようにする
  /// </summary>
  void DrawAssemblyGui();

  /// <summary>
  /// ConnectorRole を表示用文字列へ変換する
  /// ImGui 上で接続点情報を見やすく表示するために使う
  /// </summary>
  /// <param name="role">変換対象の接続点役割</param>
  /// <returns>表示用文字列</returns>
  const char *ConnectorRoleName(ConnectorRole role) const;

  /// <summary>
  /// PartSide を表示用文字列へ変換する
  /// ImGui 上で左右属性を見やすく表示するために使う
  /// </summary>
  /// <param name="side">変換対象の左右属性</param>
  /// <returns>表示用文字列</returns>
  const char *SideName(PartSide side) const;
#endif
};