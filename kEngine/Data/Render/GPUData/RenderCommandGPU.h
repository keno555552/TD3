#pragma once
#include "Data/Render/CPUData/RenderCommand.h"

struct RenderCommandGPU
{
	/// =========== CopyImage用コマンド =========== ///
	/// ない
	/// ========== ColorGuard用コマンド =========== ///
	/// この色の明度を元に色を変わる
	float guardColor[3]{};
	/// guardingのAlphaみだいなもの
	float guardAmount{};
	// ------ padding counter line-------

	/// =========== Vignette用コマンド ============ ///
	/// vignetteの中心座標（0.0～1.0の範囲で、画面のどこに中心があるか）
	float vignetteCenter[2]{};
	/// vignetteの半径（0.0～1.0の範囲で、どこからvignetteが始まるか）
	float vignetteRadius{};
	/// vignetteの柔らかさ（0.0～1.0の範囲で、vignetteの境界がどれくらいぼやけるか）
	float vignetteSoftness{};
	// ------ padding counter line-------
	/// vignetteの強さ（0.0～1.0の範囲で、vignetteがどれくらい暗くなるか）
	float vignetteIntensity{};
	float padding[3]{};
	// ------ padding counter line-------
	/// vignetteの色
	float vignetteColor[4]{};
	// ------ padding counter line-------

	///// ============= Blur用コマンド ============== ///
	///// ここは毎回BlurをDraw前に、CPUDataを使ってこれを変わるように、使用するBlurCommandを選択すること
	///// Blurの種類
	//int blurType;
	///// カーネルのサイズ（例：3なら3x3のカーネルを使う）
	//int kernelSize;
	///// カーネルの対応index
	//int kernelIndex;
	//int padding;
	//// ------ padding counter line-------

};

inline void ConvertRenderCommandToGPU(const RenderCommand& cpu, RenderCommandGPU& gpu) {

	/// ========== ColorGuard =========== ///
	if (gpu.guardColor[0] != cpu.guardColor[0]) gpu.guardColor[0] = cpu.guardColor[0];
	if (gpu.guardColor[1] != cpu.guardColor[1]) gpu.guardColor[1] = cpu.guardColor[1];
	if (gpu.guardColor[2] != cpu.guardColor[2]) gpu.guardColor[2] = cpu.guardColor[2];
	if (gpu.guardAmount != cpu.guardAmount)	gpu.guardAmount = cpu.guardAmount;

	/// ========== Vignette =========== ///
	if (gpu.vignetteCenter[0] != cpu.vignetteCenter.x) gpu.vignetteCenter[0] = cpu.vignetteCenter.x;
	if (gpu.vignetteCenter[1] != cpu.vignetteCenter.y) gpu.vignetteCenter[1] = cpu.vignetteCenter.y;
	if (gpu.vignetteRadius != cpu.vignetteRadius) gpu.vignetteRadius = cpu.vignetteRadius;
	if (gpu.vignetteSoftness != cpu.vignetteSoftness) gpu.vignetteSoftness = cpu.vignetteSoftness;
	if (gpu.vignetteIntensity != cpu.vignetteIntensity) gpu.vignetteIntensity = cpu.vignetteIntensity;
	if (gpu.vignetteColor[0] != cpu.vignetteColor.x) gpu.vignetteColor[0] = cpu.vignetteColor.x;
	if (gpu.vignetteColor[1] != cpu.vignetteColor.y) gpu.vignetteColor[1] = cpu.vignetteColor.y;
	if (gpu.vignetteColor[2] != cpu.vignetteColor.z) gpu.vignetteColor[2] = cpu.vignetteColor.z;
	if (gpu.vignetteColor[3] != cpu.vignetteColor.w) gpu.vignetteColor[3] = cpu.vignetteColor.w;

	/// ========== Blur =========== ///
	//if (gpu.blurType != static_cast<int>(cpu.blurType)) gpu.blurType = static_cast<int>(cpu.blurType);
	//if (gpu.kernelSize != cpu.kernelSize) gpu.kernelSize = cpu.kernelSize;
	//if (gpu.kernelIndex != 0) gpu.kernelIndex = cpu.kernelIndex;
}