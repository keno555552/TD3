#pragma once
#include "Object/Object.h"
#include <array>
#include <vector>

/* ModBody: Object の各部位のスケールや長さを変更するためのクラス
--------------------------------------------------------------*/
enum class ModBodyPart {
  Body = 0,
  Head,
  LeftArm,
  RightArm,
  LeftLeg,
  RightLeg,
  Count
};

/* 各部位のパラメータ
--------------------------------*/
struct ModBodyPartParam {
  Vector3 scale{1.0f, 1.0f, 1.0f};
  float length = 1.0f;
  int count = 1;
  bool enabled = true;
};

/* ModBody の全体のデータ
---------------------------------*/
struct ModBodyData {
  std::array<ModBodyPartParam, static_cast<size_t>(ModBodyPart::Count)> parts;
};

/* ModBodyPart と Objectの部位のインデックスを対応させるためのマップ
------------------------------------------------------------------*/
struct ModBodyPartIndexMap {
  std::array<int, static_cast<size_t>(ModBodyPart::Count)> indices{0, 1, 2,
                                                                   3, 4, 5};
};

class ModBody {
public:
  /// <summary>
  /// オブジェクトの初期化
  /// </summary>
  /// <param name="target">初期化するオブジェクトへのポインタ</param>
  void Initialize(Object *target);

  /// <summary>
  /// オブジェクトに適用
  /// </summary>
  /// <param name="target">適用対象のオブジェクトへのポインタ</param>
  void Apply(Object *target);

  /// <summary>
  /// データを取得
  /// </summary>
  /// <returns>ModBodyData オブジェクトへの参照</returns>
  ModBodyData &GetData() { return data_; }

  /// <summary>
  /// モッドボディのデータを取得。
  /// </summary>
  /// <returns>モッドボディデータへの定数参照</returns>
  const ModBodyData &GetData() const { return data_; }

  /// <summary>
  /// ボディパーツのインデックスを設定
  /// </summary>
  /// <param name="part">設定対象のボディパーツ</param>
  /// <param name="index">設定するインデックス値</param>
  void SetPartIndex(ModBodyPart part, int index);

  /// <summary>
  /// ボディパーツのインデックスを取得
  /// </summary>
  /// <param name="part">インデックスを取得するボディパーツ</param>
  /// <returns>指定されたボディパーツのインデックス</returns>
  int GetPartIndex(ModBodyPart part) const;

  /// <summary>
  /// リセット
  /// </summary>
  void Reset();

private:
  /// <summary>
  /// オブジェクトの基本トランスフォームをキャッシュ
  /// </summary>
  /// <param
  /// name="target">基本トランスフォームをキャッシュする対象のオブジェクト</param>
  void CacheBaseTransforms(Object *target);

  /// <summary>
  /// オブジェクトに本体を適用
  /// </summary>
  /// <param name="target">本体を適用する対象のオブジェクト</param>
  void ApplyBody(Object *target);

  /// <summary>
  /// 子パーツをオブジェクトに適用
  /// </summary>
  /// <param name="target">パーツを適用する対象のオブジェクト</param>
  /// <param name="part">適用する子ボディパーツ</param>
  void ApplyChildPart(Object *target, ModBodyPart part);

private:
  ModBodyData data_{}; // データ
  ModBodyPartIndexMap partIndexMap_{}; // 各部位のスケールや長さを保持するデータ

  std::vector<Transform> basePartTransforms_{}; // 基本部位のトランスフォーム
  std::vector<Vector3> baseOffsetsFromBody_{}; // ボディからの基本オフセット
  Transform baseMainTransform_{}; // オブジェクトの基本トランスフォームをキャッシュするための変数
  
  bool isBaseCached_ = false; // 基本トランスフォームがキャッシュできているかフラグ
};