#pragma once

enum class PSOType {
	NONE = -1,

	/// 2D系
	Sprite2D = 0,

	/// 3D系
	Lambert,
	HalfLambert,
	PhongReflection,
	BlinnPhongReflection,
	FlameNeonGlow,
	NUMBER_OF_PSO,

	/// デフォルトPSO
	defaultPSO = Lambert,
};

