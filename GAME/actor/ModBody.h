#pragma once
#include "ControlPointChain.h"
#include "ModControlPointTypes.h"
#include "Object/Object.h"
#include <array>
#include <memory>
#include <vector>

/// <summary>
/// 改造対象となる部位の種類
/// ModScene や TravelScene などで、どの部位を操作しているかを識別するために使う
/// 配列インデックスとして扱えるように値順を固定している
/// </summary>
enum class ModBodyPart {
  ChestBody = 0,
  StomachBody = 1,
  Neck = 2,
  Head = 3,

  LeftUpperArm = 4,
  LeftForeArm = 5,
  RightUpperArm = 6,
  RightForeArm = 7,

  LeftThigh = 8,
  LeftShin = 9,
  RightThigh = 10,
  RightShin = 11,

  LeftArm = LeftUpperArm,
  RightArm = RightUpperArm,
  LeftLeg = LeftThigh,
  RightLeg = RightThigh,
  Body = ChestBody,

  Count = 12
};

/// <summary>
/// 部位の見た目パラメータ
/// </summary>
struct ModBodyPartParam {
  Vector3 scale{1.0f, 1.0f, 1.0f}; // メッシュの太さ倍率（部位ローカル）
  float length = 1.0f;             // 長さ倍率（Y方向）
  int count = 1;                   // 互換用（旧方式データ向け）
  bool enabled = true;             // 表示有効
};

/// <summary>
/// 部位編集で使う操作点
/// 位置、可動性、接続点としての属性をまとめて持つ
/// </summary>
struct ModControlPoint {
  ModControlPointRole role = ModControlPointRole::None; // 役割
  Vector3 localPosition{0.0f, 0.0f, 0.0f};              // 部位ローカル位置
  float radius = 0.08f;                                 // ピック/表示サイズ
  bool movable = true;                                  // 移動可能か

  bool isConnectionPoint = false; // 接続点として扱うか
  bool acceptsParent = false;     // 親接続を受けるか
  bool acceptsChild = false;      // 子接続を受けるか
};

/// <summary>
/// 部位インスタンス単位で保存する改造データ
/// partId をキーに、構造と見た目を復元できるようにする
/// </summary>
struct ModPartInstanceData {
  int partId = -1;
  ModBodyPart partType = ModBodyPart::ChestBody;

  int parentId = -1;
  int parentConnectorId = -1;
  int selfConnectorId = -1;

  Transform localTransform = CreateDefaultTransform();
  ModBodyPartParam param{};
};

/// <summary>
/// 可変長の操作点スナップショット
/// 新方式ではこちらを正とし、ownerPartId で所属部位を判別する
/// </summary>
struct ModControlPointSnapshot {
  int ownerPartId = -1;                               // 所属部位ID
  ModBodyPart ownerPartType = ModBodyPart::ChestBody; // 所属部位種別
  ModControlPointRole role = ModControlPointRole::None;

  Vector3 localPosition{0.0f, 0.0f, 0.0f}; // 部位ローカル位置
  float radius = 0.08f;                    // ピック/表示サイズ
  bool movable = true;                     // 移動可能か

  bool isConnectionPoint = false; // 接続点として扱うか
  bool acceptsParent = false;     // 親接続を受けるか
  bool acceptsChild = false;      // 子接続を受けるか
};

struct ModControlPointData {
  Vector3 leftShoulderPos{0.0f, 0.0f, 0.0f};
  Vector3 leftElbowPos{0.0f, 0.0f, 0.0f};
  Vector3 leftWristPos{0.0f, 0.0f, 0.0f};

  Vector3 rightShoulderPos{0.0f, 0.0f, 0.0f};
  Vector3 rightElbowPos{0.0f, 0.0f, 0.0f};
  Vector3 rightWristPos{0.0f, 0.0f, 0.0f};

  Vector3 leftHipPos{0.0f, 0.0f, 0.0f};
  Vector3 leftKneePos{0.0f, 0.0f, 0.0f};
  Vector3 leftAnklePos{0.0f, 0.0f, 0.0f};

  Vector3 rightHipPos{0.0f, 0.0f, 0.0f};
  Vector3 rightKneePos{0.0f, 0.0f, 0.0f};
  Vector3 rightAnklePos{0.0f, 0.0f, 0.0f};

  Vector3 chestPos{0.0f, 0.0f, 0.0f};
  Vector3 bellyPos{0.0f, 0.0f, 0.0f};
  Vector3 waistPos{0.0f, 0.0f, 0.0f};

  Vector3 lowerNeckPos{0.0f, 0.0f, 0.0f};
  Vector3 upperNeckPos{0.0f, 0.0f, 0.0f};
  Vector3 headCenterPos{0.0f, 0.0f, 0.0f};
};

enum class NpcPresetType {
  Default,
  HeadBig,
  LongLeg,
  BigTorso,
};

// NPCのスタートからゴールまでの進行状況を保存する構造体
struct NpcStartProgressData {
  std::string name;

  float totalTime = 0.0f;
  float elapsedTime = 0.0f;

  bool isFinished = false;
  bool hasStartedMoving = false;

  float moveElapsedTime = 0.0f;

  NpcPresetType presetType = NpcPresetType::Default;

  float runTimingSkill = 1.0f;

  bool hasReachedGoal = false;
};

// リトライ先の選択肢
enum class ModRetryDestination {
  TravelSceneRetry = 0,
  ModSceneRestart,
  PromptSceneRestart,
};

// リトライメニューの状態
enum class RetryMenuState {
  Hidden = 0,
  Active,
  Confirmed,
};

// モッドシーンでのリトライ選択肢
enum class ModSceneRetryChoice {
  ModRestart = 0,
  PromptRestart,
  Count,
};

// トラベルシーンでのリトライ選択肢
enum class TravelSceneRetryChoice {
  TravelRestart = 0,
  ModRestart,
  PromptRestart,
  Count,
};

/// <summary>
/// シーン間で共有する改造データ
/// 旧固定配列は互換維持のため残し、新方式の可変データも併せて持つ
/// </summary>
struct ModBodyCustomizeData {
  int dataVersion = 2;

  // 新方式
  std::vector<ModPartInstanceData> partInstances;
  std::vector<ModControlPointSnapshot> controlPointSnapshots;

  // デフォルト操作点スナップショット（改造前の初期状態）
  std::vector<ModControlPointSnapshot> defaultControlPointSnapshots;

  // 旧方式互換
  std::array<ModBodyPartParam, static_cast<size_t>(ModBodyPart::Count)>
      partParams{};
  std::array<Vector3, static_cast<size_t>(ModBodyPart::Count)>
      bodyJointOffsets{};
  ModControlPointData controlPoints;

  // NPC情報
  std::vector<NpcStartProgressData> npcStartProgressList;

  // ゴール最大規定数
  int qualifyCount = 3;

  // リトライ先設定
  ModRetryDestination modLoseRetryDestination =
      ModRetryDestination::ModSceneRestart;

  // トラブル時のリトライ先は、モッドシーンでのリトライと、トラベルシーンでのリトライ（同じシーン再挑戦）を選べるようにする
  ModRetryDestination travelLoseRetryDestination =
      ModRetryDestination::TravelSceneRetry;

  bool modNpcRaceClosed = false;

  bool modSceneFailed = false;
  bool travelSceneFailed = false;

  RetryMenuState modRetryMenuState = RetryMenuState::Hidden;
  RetryMenuState travelRetryMenuState = RetryMenuState::Hidden;

  int modRetrySelectedIndex = 0;
  int travelRetrySelectedIndex = 0;

  bool retryDecisionConsumed = false;
};

/// <summary>
/// 1部位 = 1Object 前提で改造見た目を制御するクラス
/// - `mainPosition.transform` を接続基準にし、`objectParts_`
/// を見た目として操作する
/// - owner 部位は `ControlPointChain`
/// を持ち、チェーン構造へ段階移行できるようにする
/// </summary>
class ModBody {
public:
  /// <summary>
  /// 部位制御を初期化する
  /// 基準transformのキャッシュ、パラメータ初期化、操作点初期化を行う
  /// </summary>
  /// <param name="target">制御対象Object</param>
  /// <param name="part">部位種類</param>
  void Initialize(Object *target, ModBodyPart part);

  /// <summary>
  /// 現在のパラメータと操作点状態を `target` の見た目へ反映する
  /// 無効状態の場合はメッシュを非表示にする
  /// </summary>
  /// <param name="target">反映対象Object</param>
  void Apply(Object *target);

  /// <summary>
  /// セグメント描画を start 基準（開始点を原点扱い）にするかを設定する
  /// </summary>
  void SetSegmentStartAnchored(bool enabled) {
    segmentStartAnchored_ = enabled;
  }

  /// <summary>
  /// セグメント描画を start 基準にしているかを返す
  /// </summary>
  bool IsSegmentStartAnchored() const { return segmentStartAnchored_; }

  /// <summary>
  /// 見た目パラメータ参照を返す（編集用）
  /// </summary>
  ModBodyPartParam &GetParam() { return param_; }

  /// <summary>
  /// 見た目パラメータ参照を返す（読み取り用）
  /// </summary>
  const ModBodyPartParam &GetParam() const { return param_; }

  /// <summary>
  /// 見た目パラメータを設定する
  /// </summary>
  void SetParam(const ModBodyPartParam &param) { param_ = param; }

  /// <summary>
  /// この `ModBody` が表す部位種類を返す
  /// </summary>
  ModBodyPart GetPart() const { return part_; }

  /// <summary>
  /// 見た目パラメータを初期値へ戻す
  /// </summary>
  void Reset();

  /// <summary>
  /// 見た目スケール倍率（`enabled`/`length`反映後）を返す
  /// 接続位置の動的補正などで参照する
  /// </summary>
  Vector3 GetVisualScaleRatio() const;

  /// <summary>
  /// 操作点一覧を参照で返す（読み取り用）
  /// </summary>
  const std::vector<ModControlPoint> &GetControlPoints() const {
    return controlPoints_;
  }

  /// <summary>
  /// 操作点一覧を参照で返す（編集用）
  /// </summary>
  std::vector<ModControlPoint> &GetControlPoints() { return controlPoints_; }

  /// <summary>
  /// 操作点を初期状態へ戻す
  /// owner 部位の場合は chain も再構築する
  /// </summary>
  void ResetControlPoints();

  /// <summary>
  /// 指定roleの操作点インデックスを返す。見つからない場合は -1
  /// </summary>
  int FindControlPointIndex(ModControlPointRole role) const;

  /// <summary>
  /// 指定操作点を移動する
  /// owner 部位の場合は chain を更新し、結果を操作点へ同期する
  /// </summary>
  /// <param name="index">操作点インデックス</param>
  /// <param name="newLocalPosition">新しい部位ローカル位置</param>
  /// <returns>移動できたら true</returns>
  bool MoveControlPoint(size_t index, const Vector3 &newLocalPosition);

  /// <summary>
  /// 指定操作点を中心に見た目を拡大縮小する
  /// </summary>
  /// <param name="index">操作点インデックス</param>
  /// <param name="scaleFactor">拡大縮小倍率</param>
  /// <returns>拡大縮小できたら true</returns>
  bool ScaleControlPoint(size_t index, float scaleFactor);

  /// <summary>
  /// 指定操作点のワールド位置を返す（Objectの親子transformを加味）
  /// </summary>
  Vector3 GetControlPointWorldPosition(const Object *target,
                                       size_t index) const;

  /// <summary>
  /// 既定の共有改造データを生成して返す
  /// </summary>
  static std::unique_ptr<ModBodyCustomizeData> CreateDefaultCustomizeData();

  /// <summary>
  /// 共有改造データのコピーを生成して返す（存在しない場合は nullptr）
  /// </summary>
  static std::unique_ptr<ModBodyCustomizeData> CopySharedCustomizeData();

  /// <summary>
  /// 共有改造データを設定する（次シーンへ引き継ぐために使う）
  /// </summary>
  static void SetSharedCustomizeData(const ModBodyCustomizeData &data);

  /// <summary>
  /// 現在の共有改造データを参照で返す（存在しない場合は nullptr）
  /// </summary>
  static const ModBodyCustomizeData *GetSharedCustomizeData();

  /// <summary>
  /// 共有改造データを正規化する
  /// 新旧両方式の配列の整合性を保つために使う
  /// </summary>
  static void NormalizeCustomizeData(ModBodyCustomizeData &data);

    /// <summary>
  /// 外部から操作点一覧を丸ごと設定する（デバッグ用途 / 特殊用途向け）
  /// </summary>
  void SetControlPoints(const std::vector<ModControlPoint> &points) {
    controlPoints_ = points;

    // 復元直後に Bend-End / Chest-Belly-Waist の整合を必ず取る
    UpdateControlPointHierarchy();

    // 外部復元時は chain の内部状態とズレる可能性があるため、
    // 以降の操作はフォールバック経路で扱う
    if (HasOwnControlPoints()) {
      chain_.Clear();
    }
  }

  /// <summary>
  /// この部位が owner として操作点チェーンを直接持つかどうか
  /// </summary>
  bool HasOwnControlPoints() const;

  /// <summary>
  /// 外部の操作点列を参照して描画するための設定
  /// 前腕・脛・首など、ownerの点列を共有して描画する部位で使う
  /// </summary>
  void SetExternalSegmentSource(const std::vector<ModControlPoint> *points,
                                ModControlPointRole startRole,
                                ModControlPointRole endRole);

  /// <summary>
  /// 外部の操作点列参照を解除する
  /// </summary>
  void ClearExternalSegmentSource();

  /// <summary>
  /// この部位が保持する関節チェーンを返す（編集用）
  /// </summary>
  ControlPointChain &GetChain() { return chain_; }

  /// <summary>
  /// この部位が保持する関節チェーンを返す（読み取り用）
  /// </summary>
  const ControlPointChain &GetChain() const { return chain_; }

  /// <summary>
  /// 指定した2点役割から、この部位で実際に見た目へ使われるセグメント半径を返す
  /// 判定用カプセル半径を見た目に合わせるために使う
  /// </summary>
  float GetVisualSegmentRadius(ModControlPointRole startRole,
                               ModControlPointRole endRole) const;

private:
  /// <summary>
  /// 対象Objectの基準transformを必要に応じてキャッシュする
  /// 初回のみ `mainPosition` と全meshのtransformを保存する
  /// </summary>
  void CacheBaseTransforms(Object *target);

  /// <summary>
  /// 部位種類に応じた既定操作点を構築する
  /// </summary>
  void BuildDefaultControlPoints();

  /// <summary>
  /// 操作点の整合性を保つための補正を行う
  /// 点が重なって長さ0になっている場合などを回避する
  /// </summary>
  void UpdateControlPointHierarchy();

  /// <summary>
  /// 指定した `objectParts_[partIndex]` を `startPos` から `endPos` までの
  /// 1セグメントとして描画する
  /// </summary>
  void ApplySegmentToObjectPart(Object *target, size_t partIndex,
                                const Vector3 &startPos, const Vector3 &endPos,
                                float startRadius, float endRadius);

  /// <summary>
  /// セグメント描画できない場合のフォールバック処理
  /// </summary>
  /// <param name="target"></param>
  /// <param name="baseMeshTransform"></param>
  /// <param name="baseParts"></param>
  /// <param name="part"></param>
  /// <param name="param"></param>
  void ApplySingleMeshFallback(Object *target,
                               const Transform &baseMeshTransform,
                               const std::vector<Transform> &baseParts,
                               ModBodyPart part, const ModBodyPartParam &param);

  /// <summary>
  /// 複数meshを持つObjectのうち、`startIndex`
  /// 以降のmeshを基準transformに戻して消す
  /// </summary>
  /// <param name="target"></param>
  /// <param name="basePartTransforms"></param>
  /// <param name="startIndex"></param>
  void HideUnusedMeshes(Object *target,
                        const std::vector<Transform> &basePartTransforms,
                        size_t startIndex);

  /// <summary>
  /// 操作点チェーンの状態を、操作点一覧へ同期する
  /// </summary>
  /// <param name="points"></param>
  /// <param name="chain"></param>
  void SyncControlPointsFromChain(std::vector<ModControlPoint> *points,
                                  const ControlPointChain &chain);

  /// <summary>
  /// 隣接する操作点同士が半径ぶんめり込まないように距離を補正する
  /// Root-Bend、Bend-End、LowerNeck-UpperNeck、UpperNeck-HeadCenter に適用する
  /// </summary>
  void EnforceAdjacentPointSpacing();

  /// <summary>
  /// 2点間が最小距離以上になるように後ろ側の点を押し出す
  /// </summary>
  void PushPointToMinimumDistance(int fixedIndex, int movableIndex,
                                  float extraMargin);

  /// <summary>
  /// 部位ローカルの操作点位置を、Objectの親子transformを加味してワールド位置へ変換する
  /// </summary>
  /// <param name="target"></param>
  /// <param name="localPoint"></param>
  /// <returns></returns>
  Vector3 TransformControlPointLocalToWorld(const Object *target,
                                            const Vector3 &localPoint) const;

  /// <summary>
  /// 外部参照している操作点ローカル座標を、この部位 Object
  /// のローカル座標へ変換する
  /// </summary>
  Vector3 ConvertExternalPointToThisObjectLocal(
      const Object *target, const Vector3 &externalOwnerLocalPoint) const;

private:
  ModBodyPart part_ = ModBodyPart::ChestBody;
  ModBodyPartParam param_{};

  /// <summary>
  /// 複数メッシュを持つObject用の初期transform一覧
  /// </summary>
  std::vector<Transform> basePartTransforms_;
  Transform baseMainTransform_ = CreateDefaultTransform();
  Transform baseMeshTransform_ = CreateDefaultTransform();

  bool isBaseCached_ = false; // 基準transformがキャッシュされているかどうか
  bool segmentStartAnchored_ =
      false; // セグメント描画を開始点基準にするかどうか

  /// <summary>
  /// 現在の操作点一覧（見た目/編集UI/ピック判定に使用）
  /// </summary>
  std::vector<ModControlPoint> controlPoints_;

  /// <summary>
  /// owner部位だけが保持する関節チェーン
  /// `controlPoints_` と同期しながら段階的に完全チェーン構造へ移行する
  /// </summary>
  ControlPointChain chain_;

  /// <summary>
  /// 描画のみ別部位の点列を参照するための設定
  /// </summary>
  const std::vector<ModControlPoint> *externalControlPoints_ = nullptr;
  ModControlPointRole externalStartRole_ = ModControlPointRole::None;
  ModControlPointRole externalEndRole_ = ModControlPointRole::None;
};