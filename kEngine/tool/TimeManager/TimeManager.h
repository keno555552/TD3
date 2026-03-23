#pragma once
#include <chrono>
#include <vector>

class TimeManager
{
public:

	//void Initialize();
	//void Finalize();
	void Update();

public:
	float getDeltaTime() const { return deltaTime_; }
	float getInstantFPS() const { return instantFps_; }
	float getFPSPerSecond() const { return fpsPerSecond_; }

	/// 倍率付きの時間関数
	void setTimeScale(float timeScale) { timeScale_ = timeScale; }
	float getTimeScale() const { return timeScale_; }
	float getScaledDeltaTime() const { return scaledDeltaTime_; }
	/// Timer用の倍率付きの時間関数
	void setTimerTimeScale(float timerTimeScale) { timerTimeScale_ = timerTimeScale; }
	float getTimerTimeScale() const { return timerTimeScale_; }
	float getTimerScaledDeltaTime() const { return timerScaledDeltaTime_; }


private:
	std::chrono::steady_clock::time_point lastUpdateTime;
	float deltaTime_ = 0.0f;
	float oneScondCounter_ = 0.0f;
	std::vector<float>fpsHistory_;
	float instantFps_ = 0.0f;
	float fpsPerSecond_ = 0.0f;

private:
	float timeScale_ = 1.0f;
	float scaledDeltaTime_ = 0.0f;
	float timerTimeScale_ = 1.0f;
	float timerScaledDeltaTime_ = 0.0f;
};

class Timer {
public:
	Timer();

	/// <summary>
	/// timeを0にする初期化
	/// </summary>
	/// <param name="time">Timerのマクス時間</param>
	void Init0(float time, TimeManager* timeManager);

	/// <summary>
	/// timeをMixにする初期化
	/// </summary>
	/// <param name="time">Timerのマクス時間</param>
	void InitM(float time, TimeManager* timeManager);

	/// <summary>
	/// timeを0に戻る
	/// </summary>
	/// <param name="time">Timerのマクス時間</param>
	void Reset0();

	/// <summary>
	/// timeをMixに戻る
	/// </summary>
	/// <param name="time">Timerのマクス時間</param>
	void ResetM();

	/// <summary>
	/// Timerをマクスにすると止まる
	/// </summary>
	void ToMix();

	/// <summary>
	/// Timerをマクスにすると0になって、止まる。1以上になると動く
	/// </summary>
	void ToMixZero();

	/// <summary>
	/// Timerを0にすると止まる
	/// </summary>
	void ToZero();

	/// <summary>
	/// Timerを0にするとマクスになって、止まる。Mix - 1以下になると動く
	/// </summary>
	void ToZeroMix();

	/// <summary>
	/// 無休Upタイマー
	/// </summary>
	void foreverUp();

	/// <summary>
	/// 無休Downタイマー
	/// </summary>
	void foreverDown();

	/// <summary>
	/// アニメーション用の無休タイマー、1秒12フレームで int形数字を出力
	/// </summary>
	/// <returns></returns>
	void AnimationF();

	/// <summary>
	/// アニメーション用のタイマー、秒を12フレームの形で int形数字を出力
	/// </summary>
	/// <returns></returns>
	int FrameChange();

	/// <summary>
	/// 線形移動
	/// </summary>
	/// <param name="a">始まりの数値</param>
	/// <param name="b">最後の数値</param>
	/// <returns>aからbまで今どれくらい進んでる値</returns>
	float linearity(float a, float b);

	/// <summary>
	/// 線形移動
	/// </summary>
	/// <returns>0から1までの変化値</returns>
	float linearity();

	/// <summary>
	/// Timerに連れてAからBまで変動、段々遅くなーる
	/// </summary>
	/// <param name="a">始まりの数値</param>
	/// <param name="b">最後の数値</param>
	/// <param name="c">clockの変数</param>
	/// <param name="t">clockの最大時間</param>
	/// <param name="r">変動率[1.0f以上]</param>
	/// <returns>aからbまで今どれくらい進んでる値</returns>
	float easyIn(int a, int b, float r);

	/// <summary>
	/// Timerに連れてAからBまで変動、段々遅くなーる
	/// </summary>
	/// <param name="r">変動率[1.0f以上]</param>
	/// <returns>0から1までの変化値</returns>
	float easyIn(float r);

	/// <summary>
	/// Timerに連れてAからBまで変動、段々早くなーる
	/// </summary>
	/// <param name="a">始まりの数値</param>
	/// <param name="b">最後の数値</param>
	/// <param name="c">clockの変数</param>
	/// <param name="t">clockの最大時間</param>
	/// <param name="r">変動率[1.0f以上]</param>
	/// <returns>aからbまで今どれくらい進んでる値</returns>
	float easyOut(float a, float b, float r);

	/// <summary>
	/// Timerに連れてAからBまで変動、段々早くなーる
	/// </summary>
	/// <param name="r">変動率[1.0f以上]</param>
	/// <returns>0から1までの変化値</returns>
	float easyOut(float r);

	/// <summary>
	/// Timerに連れてAからBまで変動、段々早くなって、終わるの際段々遅くなる
	/// </summary>
	/// <param name="a">始まりの数値</param>
	/// <param name="b">最後の数値</param>
	/// <param name="c">clockの変数</param>
	/// <param name="t">clockの最大時間</param>
	/// <param name="r">変動率[1.0f以上]</param>
	/// <returns>aからbまで今どれくらい進んでる値</returns>
	float easyInOut(int a, int b, float r);

	/// <summary>
	/// Timerに連れてAからBまで変動、段々早くなって、終わるの際段々遅くなる
	/// </summary>
	/// <param name="r">変動率[1.0f以上]</param>
	/// <returns>0から1までの変化値</returns>
	float easyInOut(float r);

	/// <summary>
	/// Timerに連れてAからBまで変動、一回後ろに行って、早く最後のどころに行く
	/// </summary>
	/// <param name="a">始まりの数値</param>
	/// <param name="b">最後の数値</param>
	/// <param name="c">clockの変数</param>
	/// <param name="t">clockの最大時間</param>
	/// <param name="r">変動率[1.0f以上]</param>
	/// <returns>aからbまで今どれくらい進んでる値</returns>
	float easyInBack(int a, int b, float r);

	/// <summary>
	/// Timerに連れてAからBまで変動、一回後ろに行って、早く最後のどころに行く
	/// </summary>
	/// <param name="r">変動率[1.0f以上]</param>
	/// <returns>0から1までの変化値</returns>
	float easyInBack(float r);

	/// <summary>
	/// Timerに連れてAからBまで変動、最後のどころに着いた後止めず、一定距離後戻る
	/// </summary>
	/// <param name="a">始まりの数値</param>
	/// <param name="b">最後の数値</param>
	/// <param name="c">clockの変数</param>
	/// <param name="t">clockの最大時間</param>
	/// <param name="r">変動率[1.0f以上]</param>
	/// <returns>aからbまで今どれくらい進んでる値</returns>
	float easyOutBack(int a, int b, float r);

	/// <summary>
	/// Timerに連れてAからBまで変動、最後のどころに着いた後止めず、一定距離後戻る
	/// </summary>
	/// <param name="r">変動率[1.0f以上]</param>
	/// <returns>0から1までの変化値</returns>
	float easyOutBack(float r);

	bool GetIsMax() const;

	bool GetIsZero() const;

	/// 時計をTimeScaleの影響を受けるかどうか取得する。trueならTimeScaleの影響を受ける、falseなら受けない
	bool GetInfluenceByTimeScale() const { return isInfluenceByTimeScale_; }

	/// 時計をTimeScaleの影響を受けるかどうか設定する。trueならTimeScaleの影響を受ける、falseなら受けない
	void SetInfluenceByTimeScale(bool isInfluence) { isInfluenceByTimeScale_ = isInfluence; }

public:
	/// 経過時間
	float parameter_;
	/// 最大時間
	float maxTime_;
	/// scaleに影響されるか
	bool isInfluenceByTimeScale_ = false;

private:

	TimeManager* timeManager_ = nullptr;

private:
	float TimerSpeed();
};
#pragma endregion 

#pragma region Easing
/// <summary>
/// 線形移動
/// </summary>
/// <param name="a">始まりの数値</param>
/// <param name="b">最後の数値</param>
/// <param name="c">clockの変数</param>
/// <param name="t">clockの最大時間</param>
/// <param name="r">変動率[1.0f以上]</param>
/// <returns>aからbまで今どれくらい進んでる値</returns>
float linearity(float a, float b, float t);

/// <summary>
/// Timerに連れてAからBまで変動、段々遅くなーる
/// </summary>
/// <param name="a">始まりの数値</param>
/// <param name="b">最後の数値</param>
/// <param name="c">clockの変数</param>
/// <param name="t">clockの最大時間</param>
/// <param name="r">変動率[1.0f以上]</param>
/// <returns>aからbまで今どれくらい進んでる値</returns>
float easyIn(float a, float b, float t, float r);

/// <summary>
/// Timerに連れてAからBまで変動、段々早くなーる
/// </summary>
/// <param name="a">始まりの数値</param>
/// <param name="b">最後の数値</param>
/// <param name="c">clockの変数</param>
/// <param name="t">clockの最大時間</param>
/// <param name="r">変動率[1.0f以上]</param>
/// <returns>aからbまで今どれくらい進んでる値</returns>
float easyOut(float a, float b, float t, float r);

/// <summary>
/// Timerに連れてAからBまで変動、段々早くなって、終わるの際段々遅くなる
/// </summary>
/// <param name="a">始まりの数値</param>
/// <param name="b">最後の数値</param>
/// <param name="c">clockの変数</param>
/// <param name="t">clockの最大時間</param>
/// <param name="r">変動率[1.0f以上]</param>
/// <returns>aからbまで今どれくらい進んでる値</returns>
float easyInOut(int a, int b, int c, int t, float r);

/// <summary>
/// Timerに連れてAからBまで変動、一回後ろに行って、早く最後のどころに行く
/// </summary>
/// <param name="a">始まりの数値</param>
/// <param name="b">最後の数値</param>
/// <param name="c">clockの変数</param>
/// <param name="t">clockの最大時間</param>
/// <param name="r">変動率[1.0f以上]</param>
/// <returns>aからbまで今どれくらい進んでる値</returns>
float easyInBack(int a, int b, int c, int t, float r);

/// <summary>
/// Timerに連れてAからBまで変動、最後のどころに着いた後止めず、一定距離後戻る
/// </summary>
/// <param name="a">始まりの数値</param>
/// <param name="b">最後の数値</param>
/// <param name="c">clockの変数</param>
/// <param name="t">clockの最大時間</param>
/// <param name="r">変動率[1.0f以上]</param>
/// <returns>aからbまで今どれくらい進んでる値</returns>
float easyOutBack(int a, int b, int c, int t, float r);

float smootherstep(float t);

#pragma endregion 