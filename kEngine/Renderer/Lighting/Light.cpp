#include "Light.h"

Light::Light():
	lightingType(LightingType::DirectionalLight),
	direction(-0.5f, -0.5f, 0.4f),
	angle(0),
	position(0,0,0),
	range(0),
	color(1, 1, 1),
	intensity(1.0f),
	ableLight(true)
{}
