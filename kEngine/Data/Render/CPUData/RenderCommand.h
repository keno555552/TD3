#pragma once
#include "Vector2.h"
#include "Vector4.h"

enum class BlurType {
	Box,
	Custom,
	NumOfBlur
};

struct BlurCommand
{
	BlurType blurType = BlurType::Box;
	int kernelSize = 5;
	float kernel[7][7]{};	/// 最大7x7のカーネルを想定している
};

struct RenderCommand
{
	/// =========== CopyImage用コマンド =========== ///
	/// ない
	/// ========== ColorGuard用コマンド =========== ///
	/// この色の明度を元に色を変わる
	float guardColor[3]{ 0.2125f, 0.7154f, 0.0721f };
	/// guardingのAlphaみだいなもの
	float guardAmount = 1.0f;
	/// =========== Vignette用コマンド ============ ///
	/// vignetteの中心座標（0.0～1.0の範囲で、画面のどこに中心があるか）
	Vector2 vignetteCenter{ 0.5f, 0.5f };
	/// vignetteの半径（0.0～1.0の範囲で、どこからvignetteが始まるか）
	float vignetteRadius = 1.0f;
	/// vignetteの柔らかさ（0.0～1.0の範囲で、vignetteの境界がどれくらいぼやけるか）
	float vignetteSoftness = 0.079f;
	/// vignetteの強さ
	float vignetteIntensity = 3.21f;
	/// vignetteの色
	Vector4 vignetteColor = { 0.0f, 0.0f, 0.0f, 1.0f };

	/// ============= Blur用コマンド ============== ///
	BlurCommand blurCommand[5];

};