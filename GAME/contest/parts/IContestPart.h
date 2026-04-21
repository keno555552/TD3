#pragma once
#include "Vector3.h"
#include"Vector4.h"
#include <string>

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
/// ランクに応じた色を返す
/// </summary>
inline Vector4 GetRankColor(const std::string& rank) {
	if (rank == "SS") return { 0.7f, 0.9f, 1.0f, 1.0f };   // 薄い水色
	if (rank == "S")  return { 1.0f, 0.84f, 0.0f, 1.0f };   // 金色
	if (rank == "A")  return { 0.75f, 0.75f, 0.75f, 1.0f };  // 銀色
	if (rank == "B")  return { 0.8f, 0.5f, 0.2f, 1.0f };     // 銅色
	if (rank == "C")  return { 1.0f, 0.2f, 0.6f, 1.0f };     // ビビットピンク
	if (rank == "D")  return { 0.76f, 0.6f, 0.42f, 1.0f };   // ダンボール色
	return { 1.0f, 1.0f, 1.0f, 1.0f }; // デフォルト白
}

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