#pragma once
#include "Object/Object.h"

/* ModBodyPart: 改造対象の部位種別
--------------------------------*/
enum class ModBodyPart {
  Body = 0,
  Head,
  LeftArm,
  RightArm,
  LeftLeg,
  RightLeg,
  Count
};

/* 各部位の改造パラメータ
--------------------------------*/
struct ModBodyPartParam {
  Vector3 scale{1.0f, 1.0f, 1.0f};
  float length = 1.0f;
  int count = 1;
  bool enabled = true;
};

/* ModBody: 1部位 = 1Object の改造制御クラス
--------------------------------------------------------------
役割:
- mainPosition.transform      : 接続点(root)
- objectParts_[0].transform   : 見た目(mesh)

このクラスは objectParts_[0] の見た目側 transform を
部位ごとの anchor 方針に従って再計算する。
--------------------------------------------------------------*/
class ModBody {
public:
  /// <summary>
  /// 初期化
  /// </summary>
  /// <param name="target">対象 Object</param>
  /// <param name="part">この Object が表す部位</param>
  void Initialize(Object *target, ModBodyPart part);

  /// <summary>
  /// 改造を対象 Object に適用
  /// </summary>
  /// <param name="target">対象 Object</param>
  void Apply(Object *target);

  /// <summary>
  /// 改造パラメータ取得
  /// </summary>
  ModBodyPartParam &GetParam() { return param_; }

  /// <summary>
  /// 改造パラメータ取得 const
  /// </summary>
  const ModBodyPartParam &GetParam() const { return param_; }

  /// <summary>
  /// この ModBody が担当する部位
  /// </summary>
  ModBodyPart GetPart() const { return part_; }

  /// <summary>
  /// パラメータを初期化
  /// </summary>
  void Reset();

    /// <summary>
  /// root ではなく mesh 側に適用される見た目スケール倍率を取得
  /// </summary>
  Vector3 GetVisualScaleRatio() const;

private:
  /// <summary>
  /// 初期 transform を保存
  /// </summary>
  void CacheBaseTransforms(Object *target);

private:
  ModBodyPart part_ = ModBodyPart::Body;
  ModBodyPartParam param_{};

  Transform baseMainTransform_{};
  Transform baseMeshTransform_{};

  bool isBaseCached_ = false;
};