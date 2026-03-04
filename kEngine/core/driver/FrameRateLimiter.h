#pragma once
#include "DirectXCore.h"
#include <chrono>

class FrameRateLimiter {
public:
    FrameRateLimiter(double targetFPS = 60.0f);

    void Wait();

private:
    LARGE_INTEGER frequency;
    LARGE_INTEGER lastTime;
    double targetFrameTime;
};

class FixFPS {
public:
    void Initialize();

    void Update();

private: // メンバ関数
    // 記録時間(FPS固定用)
    std::chrono::steady_clock::time_point reference_ = std::chrono::steady_clock::now();
    //LARGE_INTEGER frequency;
    //LARGE_INTEGER lastTime;
    //double targetFrameTime;
};


