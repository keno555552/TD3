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

struct ModBodyPartParam {
  Vector3 scale{1.0f, 1.0f, 1.0f};
  float length = 1.0f;
  int count = 1;
  bool enabled = true;
};

struct ModControlPoint {
  ModControlPointRole role = ModControlPointRole::None;
  Vector3 localPosition{0.0f, 0.0f, 0.0f};
  float radius = 0.08f;
  bool movable = true;

  bool isConnectionPoint = false;
  bool acceptsParent = false;
  bool acceptsChild = false;
};

struct ModPartInstanceData {
  int partId = -1;
  ModBodyPart partType = ModBodyPart::ChestBody;

  int parentId = -1;
  int parentConnectorId = -1;
  int selfConnectorId = -1;

  Transform localTransform = CreateDefaultTransform();
  ModBodyPartParam param{};
};

struct ModBodyCustomizeData {
  std::vector<ModPartInstanceData> partInstances;

  std::array<ModBodyPartParam, static_cast<size_t>(ModBodyPart::Count)>
      partParams{};

  std::array<Vector3, static_cast<size_t>(ModBodyPart::Count)>
      bodyJointOffsets{};
};

/// <summary>
/// 1部位 = 1Object 前提で改造見た目を制御するクラス
/// mainPosition.transform を接続基準にし、objectParts_ を見た目として扱う
/// さらに owner 部位は ControlPointChain
/// を持ち、完全チェーン構造へ移行しやすくする
/// </summary>
class ModBody {
public:
  void Initialize(Object *target, ModBodyPart part);
  void Apply(Object *target);

  // セグメント描画を start 基準にするか
  void SetSegmentStartAnchored(bool enabled) { segmentStartAnchored_ = enabled; }
  bool IsSegmentStartAnchored() const { return segmentStartAnchored_; }

  ModBodyPartParam &GetParam() { return param_; }
  const ModBodyPartParam &GetParam() const { return param_; }
  void SetParam(const ModBodyPartParam &param) { param_ = param; }

  ModBodyPart GetPart() const { return part_; }

  void Reset();
  Vector3 GetVisualScaleRatio() const;

  const std::vector<ModControlPoint> &GetControlPoints() const {
    return controlPoints_;
  }

  std::vector<ModControlPoint> &GetControlPoints() { return controlPoints_; }

  void ResetControlPoints();
  int FindControlPointIndex(ModControlPointRole role) const;
  bool MoveControlPoint(size_t index, const Vector3 &newLocalPosition);
  Vector3 GetControlPointWorldPosition(const Object *target,
                                       size_t index) const;

  static std::unique_ptr<ModBodyCustomizeData> CreateDefaultCustomizeData();
  static std::unique_ptr<ModBodyCustomizeData> CopySharedCustomizeData();
  static void SetSharedCustomizeData(const ModBodyCustomizeData &data);
  static const ModBodyCustomizeData *GetSharedCustomizeData();

  /// <summary>
  /// 外部から操作点一覧を丸ごと設定する
  /// デバッグ用途や特殊用途向け
  /// </summary>
  void SetControlPoints(const std::vector<ModControlPoint> &points) {
    controlPoints_ = points;
  }

  /// <summary>
  /// この部位が owner として操作点チェーンを直接持つかどうか
  /// </summary>
  bool HasOwnControlPoints() const;

  /// <summary>
  /// 外部の操作点列を参照して描画するための設定
  /// 前腕・脛・首のような共有セグメント描画に使う
  /// </summary>
  void SetExternalSegmentSource(const std::vector<ModControlPoint> *points,
                                ModControlPointRole startRole,
                                ModControlPointRole endRole);

  /// <summary>
  /// 外部の操作点列参照を解除する
  /// </summary>
  void ClearExternalSegmentSource();

  /// <summary>
  /// この部位が保持する関節チェーンを返す
  /// </summary>
  ControlPointChain &GetChain() { return chain_; }

  /// <summary>
  /// この部位が保持する関節チェーンを読み取り専用で返す
  /// </summary>
  const ControlPointChain &GetChain() const { return chain_; }

private:
  void CacheBaseTransforms(Object *target);
  void BuildDefaultControlPoints();
  void UpdateControlPointHierarchy();

  /// <summary>
  /// 指定した objectParts_[partIndex] を、startPos から endPos までの
  /// 1セグメントとして描画する
  /// </summary>
  void ApplySegmentToObjectPart(Object *target, size_t partIndex,
                                const Vector3 &startPos, const Vector3 &endPos);

private:
  ModBodyPart part_ = ModBodyPart::ChestBody;
  ModBodyPartParam param_{};

  Transform baseMainTransform_{};
  Transform baseMeshTransform_{};

  /// <summary>
  /// 複数メッシュを持つ Object 用の初期 transform 一覧
  /// Body の 2segment 描画で使う
  /// </summary>
  std::vector<Transform> basePartTransforms_;

  bool isBaseCached_ = false;

  std::vector<ModControlPoint> controlPoints_;

  /// <summary>
  /// owner 部位だけが保持する関節チェーン
  /// controlPoints_ と同期しながら段階的に完全チェーン構造へ移行する
  /// </summary>
  ControlPointChain chain_;

  const std::vector<ModControlPoint> *externalControlPoints_ = nullptr;
  ModControlPointRole externalStartRole_ = ModControlPointRole::None;
  ModControlPointRole externalEndRole_ = ModControlPointRole::None;

  bool segmentStartAnchored_ = false;
};