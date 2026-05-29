#pragma once
#include "MathsIncluder.h"

// 基本物体情報
struct PhysicalObject {
    Vector3 position;       // ボールの位置
    Vector3 velocity;       // ボールの速度
    Vector3 acceleration;   // ボールの加速度
    float mass;             // ボールの質量
    unsigned int color;     // ボールの色
};