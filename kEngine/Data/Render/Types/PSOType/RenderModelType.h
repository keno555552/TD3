#pragma once

enum class RenderModelType {

	/// 描画モデルの種類
	Static = 0,
	Skinned,
	DebugLine,
	Environment,
	Sprite2D,
	FlameNeonGlow,

	/// PostProcess用の描画モデル
	ColorGradient,
	Vignette,
	Blur,
	FullscreenQuad,

	/// RenderModelの数
	NumOfRenderModels,
};
