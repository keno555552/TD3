#pragma once
#include "Data/Render/Types/LightingType.h"
#include "MathsIncluder.h"

class LightManager;
class Light{
public:
	Light();
	~Light() = default;

public:

	LightingType lightingType;

	/// ============= 本データ ============= ///
	Vector3 direction;
	float angle;

	Vector3 position;
	float range;

	Vector3 color;
	float intensity;

	bool ableLight;
};
