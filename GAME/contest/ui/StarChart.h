#pragma once

class kEngine;
class SimpleSprite;
struct Vector2;
struct Vector4;

/// <summary>
/// 五芒星型のレーダーチャート
/// 5頂点を三角扇（center + v[i] + v[i+1]）で描画する
/// </summary>
class StarChart {
public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="system">エンジン</param>
	/// <param name="centerX">五芒星の中心X座標</param>
	/// <param name="centerY">五芒星の中心Y座標</param>
	void Initialize(kEngine* system, float centerX, float centerY);

	/// <summary>
	/// ★値を設定して星形を更新する
	/// 未表示の項目は0を渡す
	/// </summary>
	/// <param name="theme">テーマ一致度（0〜5）</param>
	/// <param name="impact">インパクト（0〜5）</param>
	/// <param name="commitment">こだわり（0〜5）</param>
	/// <param name="efficiency">効率（0〜5）</param>
	/// <param name="judgeEval">審査員評価（0〜5）</param>
	void SetStars(int theme, int impact, int commitment,
		int efficiency, int judgeEval);

	/// <summary>
	/// 5枚すべての色（textureColor）を設定する。RGBA、半透明可。
	/// </summary>
	void SetColor(const Vector4& color);

	/// <summary>
	/// 5軸×★1〜5の25個の基準ドットを表示する。指定色で表示。
	/// 一度有効化すると Cleanup までドットが描画される。
	/// </summary>
	void EnableLevelDots(const Vector4& color);

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	/// <summary>
	/// 解放
	/// </summary>
	void Cleanup();

private:
	kEngine* system_ = nullptr;
	SimpleSprite* triangles_[5] = {};
	SimpleSprite* dots_[25] = {};   /// 5軸×5レベル
	int textureHandle_ = 0;

	/// ドット1辺のサイズ（ピクセル）
	static constexpr float kDotSize = 8.0f;

	float centerX_ = 0.0f;
	float centerY_ = 0.0f;

	/// ★5 時の外側頂点の最大半径（ピクセル）
	static constexpr float kMaxRadius  = 250.0f;

	/// 谷点の最小半径比（kMaxRadius に対する比率）
	/// 隣接apexが両方とも小さくても、谷がこの値より内側に潜らない
	static constexpr float kDipFloorRatio = 0.25f;

	/// 谷点の半径 = 隣接2つのapex半径の平均 × この比率
	/// 1.0 に近いほど谷が浅く（五角形寄り）、小さいほど深い星形になる
	static constexpr float kDipRatio = 0.4f;

	/// ★1 のときの外側頂点半径比（★5=1.0 までを線形補間）
	/// 内側谷点(kInnerRatio≈0.382)より大きい必要がある。
	/// 0.5 → ★1=125px, ★3=175px, ★5=250px
	static constexpr float kMinStarRatio = 0.5f;
};
