#include "AeraLight.h"

AreaLight::AreaLight() {
	lightingType = LightingType::AreaLight;
	ableLight = true;

	position = Vector3{ 0.0f, 0.0f, 0.0f };
	color = Vector3{ 1.0f, 1.0f, 1.0f };
	intensity = 1.0f;

	// エリアライトのみのテータを初期化
	right = { 1.0f, 0.0f, 0.0f };
	up = { 0.0f,  0.0f , -1.0f };

	width = 1.0f;
	height = 1.0f;

	range = 10.0f;

	//pointNumber = 4;
}
