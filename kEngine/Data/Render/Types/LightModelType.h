#pragma once
#include <string>

enum class LightModelType {
    NONE = -1,
    Sprite2D,
    Lambert,
    HalfLambert,
    PhongReflection,
    BlinnPhongReflection,
	FlameNeonGlow,
    NumLightModels
};

std::string GetLightModelMacro(LightModelType model);