#pragma once
#include "Light.h"

class AreaLight : public Light
{
	public:
	AreaLight();
	~AreaLight() = default;


public:

    Vector3 pos;
    Vector3 right;
    Vector3 up;
    float width;
    float height;
    float range;
    Vector3 color;
    float intensity;

    // 採樣點數量（固定 4）
    static constexpr int pointNumber = 4;

};

