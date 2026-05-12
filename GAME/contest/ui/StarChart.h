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
	int textureHandle_ = 0;

	float centerX_ = 0.0f;
	float centerY_ = 0.0f;

	/// ★5 時の外側頂点の最大半径（ピクセル）
	static constexpr float kMaxRadius  = 200.0f;

	/// 内側谷点の半径比（外側半径に対する比率）
	/// 黄金比由来 ≈ 0.382 で正多角形の星形になる。下げるほど痩せた星になる
	static constexpr float kInnerRatio = 0.382f;

	/// ★1 のときの外側頂点半径比（★5=1.0 までを線形補間）
	/// 内側谷点(kInnerRatio≈0.382)より大きい必要がある。
	/// 0.6 → ★1=120px, ★3=160px, ★5=200px
	static constexpr float kMinStarRatio = 0.6f;
};
