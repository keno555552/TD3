#pragma once
#include "Vector4.h"
#include "Vector3.h"

struct LightGPU {
    Vector3 direction;
    float angle;

    Vector3 position;
    float range;

    Vector3 color;
    float intensity;

    int type;
    int extra0;
    int extra1;
    int extra2;
};


