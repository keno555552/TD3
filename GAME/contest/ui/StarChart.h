#pragma once

class kEngine;
class SimpleSprite;
struct Vector2;

/// <summary>
/// 五芒星型のレーダーチャート
/// 5枚の三角形スプライトを回転配置し、★値でスケールを変える
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

	/// 頂点の回転角度（ラジアン）
	/// 上から時計回り: テーマ(0°), インパクト(72°), こだわり(144°), 効率(216°), 審査員(288°)
	static constexpr float kRotations[5] = {
		0.0f,       /// テーマ一致度   0°
		1.25664f,   /// インパクト    72°
		2.51327f,   /// こだわり     144°
		3.76991f,   /// 効率        216°
		5.02655f,   /// 審査員評価   288°
	};

	/// ★値(1〜5)からスケール(0.2〜1.0)に変換。0なら非表示(0.0)
	static float StarToScale(int star);
};
