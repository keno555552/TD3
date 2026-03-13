#pragma once
#include <string>

enum class LightingType
{
	None = -1,
    DirectionalLight,
	PointLight,
	SpotLight,
	AreaLight
};

std::string GetLightingTypeMacro(LightingType type);

