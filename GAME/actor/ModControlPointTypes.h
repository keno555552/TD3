#pragma once

/// <summary>
/// 操作点の役割
/// Ray 選択や移動制限、追従処理の対象を識別するために使う
/// </summary>
enum class ModControlPointRole {
  None = 0,

  Root,      // 接続元となる根元点
  Bend,      // 肘・膝などの曲げ点
  End,       // 手首・足首などの末端点
  Chest,     // 胸
  Belly,     // 腹
  Waist,     // 腰
  LowerNeck, // 下首
  UpperNeck, // 上首
  HeadCenter // 頭中心
};