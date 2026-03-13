#include "FrameRateLimiter.h"
#include <pplwin.h>

FrameRateLimiter::FrameRateLimiter(double targetFPS) {
	targetFrameTime = 1.0f / targetFPS;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&lastTime);
}

void FrameRateLimiter::Wait() {
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);

	double elapsed = static_cast<double>(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;
	double remaining = targetFrameTime - elapsed;

	if (remaining > 0.001f) {
		DWORD sleepMs = static_cast<DWORD>((remaining - 0.001f) * 1000.0f);
		if (sleepMs > 0) {
			Sleep(sleepMs);
		}
	}

	// busy wait
	do {
		QueryPerformanceCounter(&currentTime);
		elapsed = static_cast<double>(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;
	} while (elapsed < targetFrameTime);

	// 更新為實際時間點
	lastTime = currentTime;
}

void FixFPS::Initialize() {
	reference_ = std::chrono::steady_clock::now();
}

void FixFPS::Update() {
	/// 1/6秒ぴったりの時間
	const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f));
	/// 1/6秒よりわずかに短い時間
	const std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / 65.0f));

	/// 現在の時刻を取得する
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	/// 前回記録の時刻との差を取得する
	std::chrono::microseconds elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);

	/// 1/60秒（よりわずかに短い時間）経っていない場合
	if (elapsed < kMinCheckTime) {
		/// 1/60秒経過するまで待つスリープを繰り返す
		while (std::chrono::steady_clock::now() - reference_ < kMinTime) {
			/// 1マイクロ秒スリープ
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
	}

	// 現在の時刻を記録する
	reference_ = std::chrono::steady_clock::now();

}
