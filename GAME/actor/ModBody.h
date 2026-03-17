#pragma once
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
  Body = 0, // 胴体
  Neck = 1, // 首
  Head = 2, // 頭

  LeftUpperArm = 3,  // 左上腕
  LeftForeArm = 4,   // 左前腕
  RightUpperArm = 5, // 右上腕
  RightForeArm = 6,  // 右前腕

  LeftThigh = 7,  // 左腿
  LeftShin = 8,   // 左脛
  RightThigh = 9, // 右腿
  RightShin = 10, // 右脛

  // 既存コード互換用の別名
  LeftArm = LeftUpperArm,
  RightArm = RightUpperArm,
  LeftLeg = LeftThigh,
  RightLeg = RightThigh,

  Count = 11
};

/// <summary>
/// 1部位ぶんの改造パラメータ
/// ModScene で編集した値を保持し、Apply で Object の見た目へ反映する
/// </summary>
struct ModBodyPartParam {
  Vector3 scale{1.0f, 1.0f, 1.0f}; // XYZ方向の基本スケール倍率
  float length = 1.0f;             // 長さ倍率。Y方向の長さ調整に使う
  int count = 1;                   // 同種部位の本数。保存や判定用に使う
  bool enabled = true;             // false の場合は非表示扱いにする
};

/// <summary>
/// partId 単位で保存する改造部位データ
/// 可変構成の部位を保存するために、親子関係や接続情報もまとめて持つ
/// </summary>
struct ModPartInstanceData {
  int partId = -1;                          // AssemblyGraph 上の部位ID
  ModBodyPart partType = ModBodyPart::Body; // 部位の種類

  int parentId = -1;          // 親部位のID
  int parentConnectorId = -1; // 親側の接続コネクタID
  int selfConnectorId = -1;   // 自分側の接続コネクタID

  Transform localTransform = CreateDefaultTransform(); // 親基準のローカル変換
  ModBodyPartParam param{}; // この部位に適用する改造パラメータ
};

/// <summary>
/// シーン間で共有する改造結果データ
/// ModScene で作った改造結果を TravelScene や ContestScene へ渡すために使う
/// 新方式の可変構成データと、旧方式互換の固定配列データの両方を持つ
/// </summary>
struct ModBodyCustomizeData {
  std::vector<ModPartInstanceData> partInstances; // 可変構成の部位データ一式

  std::array<ModBodyPartParam, static_cast<size_t>(ModBodyPart::Count)>
      partParams{}; // 旧方式互換の部位ごとの改造パラメータ

  std::array<Vector3, static_cast<size_t>(ModBodyPart::Count)>
      bodyJointOffsets{}; // 旧方式互換の Body 基準接続位置
};

/// <summary>
/// 1部位 = 1Object 前提で改造見た目を制御するクラス
/// mainPosition.transform を接続点として扱い、objectParts_[0].transform
/// を見た目として扱う 初期 transform
/// の保持、改造パラメータの管理、見た目への反映を担当する
/// </summary>
class ModBody {
public:
  /// <summary>
  /// 対象 Object と部位種別を初期設定する
  /// Object の初期 transform
  /// を保存し、編集用パラメータを初期状態へ戻すときに使う 主に生成直後の部位
  /// Object に対して呼ぶ
  /// </summary>
  /// <param name="target">改造対象の Object</param>
  /// <param name="part">この Object が表す部位種別</param>
  void Initialize(Object *target, ModBodyPart part);

  /// <summary>
  /// 現在の改造パラメータを対象 Object へ反映する
  /// root 側は接続基準として維持し、mesh 側にだけスケールと位置補正を適用する
  /// 主に更新処理から毎フレーム呼ぶ
  /// </summary>
  /// <param name="target">反映先の Object</param>
  void Apply(Object *target);

  /// <summary>
  /// 改造パラメータを編集するための参照を返す
  /// ImGui などから直接 scale や length を変更するときに使う
  /// </summary>
  /// <returns>改造パラメータ参照</returns>
  ModBodyPartParam &GetParam() { return param_; }

  /// <summary>
  /// 改造パラメータの読み取り専用参照を返す
  /// 保存処理や参照処理で現在値を読むときに使う
  /// </summary>
  /// <returns>改造パラメータの const 参照</returns>
  const ModBodyPartParam &GetParam() const { return param_; }

  /// <summary>
  /// 改造パラメータをまとめて設定する
  /// 保存データの復元や別データの反映時に使う
  /// </summary>
  /// <param name="param">設定する改造パラメータ</param>
  void SetParam(const ModBodyPartParam &param) { param_ = param; }

  /// <summary>
  /// この ModBody が担当している部位種別を返す
  /// どの部位用インスタンスかを判定したいときに使う
  /// </summary>
  /// <returns>部位種別</returns>
  ModBodyPart GetPart() const { return part_; }

  /// <summary>
  /// 改造パラメータを初期状態へ戻す
  /// scale=1、length=1、count=1、enabled=true に戻す
  /// </summary>
  void Reset();

  /// <summary>
  /// 見た目に実際に反映されるスケール比を返す
  /// length を含めた最終的な見た目倍率を取得したいときに使う
  /// </summary>
  /// <returns>実際の見た目スケール比</returns>
  Vector3 GetVisualScaleRatio() const;

  /// <summary>
  /// デフォルトの改造共有データを生成する
  /// 共有データがまだ無いときの初期値を作るために使う
  /// </summary>
  /// <returns>デフォルト値入りの共有データ</returns>
  static std::unique_ptr<ModBodyCustomizeData> CreateDefaultCustomizeData();

  /// <summary>
  /// 現在の共有改造データをコピーして返す
  /// 共有データを直接触らず、編集用コピーを作りたいときに使う
  /// </summary>
  /// <returns>共有データのコピー。未設定なら nullptr</returns>
  static std::unique_ptr<ModBodyCustomizeData> CopySharedCustomizeData();

  /// <summary>
  /// 改造共有データを静的領域へ保存する
  /// ModScene で作った改造結果を別シーンへ渡したいときに使う
  /// </summary>
  /// <param name="data">保存したい改造データ</param>
  static void SetSharedCustomizeData(const ModBodyCustomizeData &data);

  /// <summary>
  /// 現在共有されている改造データを取得する
  /// 他シーンから改造結果を参照したいときに使う
  /// </summary>
  /// <returns>共有データへのポインタ。未設定なら nullptr</returns>
  static const ModBodyCustomizeData *GetSharedCustomizeData();

private:
  /// <summary>
  /// 対象 Object の初期 transform を一度だけ保存する
  /// mainPosition.transform と objectParts_[0].transform を記録し、
  /// Apply 時に元モデル基準で見た目を再計算できるようにする
  /// </summary>
  /// <param name="target">キャッシュ対象の Object</param>
  void CacheBaseTransforms(Object *target);

private:
  ModBodyPart part_ = ModBodyPart::Body; // このインスタンスが担当する部位種別
  ModBodyPartParam param_{};             // 現在の改造パラメータ

  Transform baseMainTransform_{}; // 初期状態の root transform
  Transform baseMeshTransform_{}; // 初期状態の mesh transform

  bool isBaseCached_ = false; // 初期 transform を保存済みかどうか
};