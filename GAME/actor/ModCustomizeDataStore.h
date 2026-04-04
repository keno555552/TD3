#pragma once
#include "ModBody.h"
#include <memory>

/// <summary>
/// 改造データの共有・生成・正規化を行うユーティリティクラス
///
/// 【役割】
/// - ModScene で作成した改造データをシーン間で共有する
/// - TravelScene / ContestScene などが同じ改造体を再現できるようにする
/// - 新方式（可変構造）と旧方式（固定配列）のデータ整合性を維持する
///
/// 【重要】
/// 現在の改造データは「可変構造（partInstances /
/// controlPointSnapshots）」が正であり、 以下は互換用に維持されているだけ
/// - partParams
/// - bodyJointOffsets
/// - controlPoints
///
/// → 新しいシーンでは controlPointSnapshots を優先して使うこと
/// → bodyJointOffsets は固定値なのでズレの原因になる
///
/// 特に肩や腰の接続位置は
/// controlPointSnapshots 内の Root（例：LeftUpperArm）を使うこと
/// </summary>
class ModCustomizeDataStore {
public:
  /// <summary>
  /// デフォルトの改造データを生成する
  ///
  /// 【用途】
  /// - ゲーム開始時の初期状態
  /// - 改造データが存在しない場合のフォールバック
  ///
  /// 【内容】
  /// - 各部位の scale / length / enabled を初期値に設定
  /// - bodyJointOffsets に旧方式のデフォルト接続位置を設定
  /// - 制限時間などの進行データも初期化
  ///
  /// 【注意】
  /// bodyJointOffsets は旧方式の固定接続位置なので、
  /// 新方式の controlPointSnapshots と一致しない場合がある
  /// </summary>
  static std::unique_ptr<ModBodyCustomizeData> CreateDefaultCustomizeData();

  /// <summary>
  /// 現在共有されている改造データのコピーを生成する
  ///
  /// 【用途】
  /// - ModScene → TravelScene へのデータ受け渡し
  /// - シーンごとに独立したデータとして扱うためのコピー生成
  ///
  /// 【戻り値】
  /// - データが存在しない場合は nullptr
  /// </summary>
  static std::unique_ptr<ModBodyCustomizeData> CopySharedCustomizeData();

  /// <summary>
  /// 改造データを共有領域へ保存する
  ///
  /// 【用途】
  /// - ModScene 終了時に呼び出して次シーンへ引き継ぐ
  ///
  /// 【処理内容】
  /// - NormalizeCustomizeData() を内部で実行
  /// - 新旧データの整合性を取った状態で保存する
  ///
  /// 【重要】
  /// この関数を通さずにデータを渡すと、
  /// TravelScene 側でズレ（腕が体に埋まるなど）が発生する可能性がある
  /// </summary>
  static void SetSharedCustomizeData(const ModBodyCustomizeData &data);

  /// <summary>
  /// 現在共有されている改造データを取得する
  ///
  /// 【用途】
  /// - TravelScene / ContestScene で改造体を復元するために使用
  ///
  /// 【注意】
  /// - 戻り値は内部ポインタなので書き換え禁止
  /// - 編集したい場合は CopySharedCustomizeData() を使うこと
  /// </summary>
  static const ModBodyCustomizeData *GetSharedCustomizeData();

  /// <summary>
  /// 改造データを正規化する
  ///
  /// 【目的】
  /// 新方式（可変構造）と旧方式（固定配列）のデータを同期する
  ///
  /// 【具体的な処理】
  /// 1. partInstances を基準に partParams を再構築
  /// 2. controlPointSnapshots を controlPoints（旧方式）へ反映
  /// 3. count / enabled の整合性を補正
  /// 4. 制限時間の異常値を補正
  ///
  /// 【超重要】
  /// TravelScene が旧方式（bodyJointOffsets / controlPoints）を参照する場合、
  /// この正規化を通しておかないと位置ズレが発生する
  ///
  /// ただし最終的には
  /// → controlPointSnapshots を直接参照する構造へ移行するのが望ましい
  /// </summary>
  static void NormalizeCustomizeData(ModBodyCustomizeData &data);

private:
  ModCustomizeDataStore() = delete;
};