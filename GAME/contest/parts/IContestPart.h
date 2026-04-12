#pragma once

class kEngine;
class BitmapFont;

/// <summary>
/// コンテストシーン内の各パートの基底クラス（ステートパターン）
/// </summary>
class IContestPart {
public:
	IContestPart(kEngine* system, BitmapFont* font)
		: system_(system), font_(font) {
	}
	virtual ~IContestPart() = default;

	/// <summary>
	/// パートの更新処理
	/// </summary>
	virtual void Update() = 0;

	/// <summary>
	/// パートの描画処理
	/// </summary>
	virtual void Draw() = 0;

	/// <summary>
	/// このパートが完了したかどうか
	/// </summary>
	/// <returns>true: 次のパートへ遷移する</returns>
	virtual bool IsFinished() const = 0;

protected:
	kEngine* system_ = nullptr;
	BitmapFont* font_ = nullptr;
};
