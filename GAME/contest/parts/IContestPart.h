#pragma once
#include "Vector3.h"

class kEngine;
class BitmapFont;

/// <summary>
/// パートのカメラ設定
/// </summary>
struct PartCameraTransform {
	Vector3 position = { 0.0f, 0.0f, 0.0f };
	Vector3 rotation = { 0.0f, 0.0f, 0.0f };
};

/// <summary>
/// コンテストシーン内の各パートの基底クラス（ステートパターン）
/// </summary>
class IContestPart {
public:
	IContestPart(kEngine* system, BitmapFont* font)
		: system_(system), font_(font) {
	}
	virtual ~IContestPart() = default;

	virtual void Update() = 0;
	virtual void Draw() = 0;
	virtual bool IsFinished() const = 0;

	/// <summary>
	/// 現在のカメラ設定を返す
	/// </summary>
	virtual PartCameraTransform GetCameraTransform() const = 0;

protected:
	kEngine* system_ = nullptr;
	BitmapFont* font_ = nullptr;
};