#pragma once
#include "MathsIncluder.h"
#include "Data/Render/CPUData/materialconfig.h"
#include "kEngine.h"

//enum{
//	CANT,
//	NORMAL,
//	SELECT,
//	PRESS
//};

class Button
{
public:
	Button(kEngine* system);

	bool GetPress() { return isPress_; }
	bool GetCanPress() { return canPress_; }
	bool GetOnOff() { return on_; }
	bool GetResetOnOff();
	void SetPos(Vector2 pos) { pos_ = pos; }
	void SetWidth(float width) { width_ = width; }
	void SetHeight(float height) { height_ = height; }
	void SetCanPress() { canPress_ = true; }
	void SetCannotPress() { canPress_ = false; }

	bool CheckIsSelect(Vector2 mouse, float width, float height);
	void ResetButton();

	void Init(Vector2 pos, float width, float height, int textureN, int textureS, int textureP, int textureC, int click1, int click2);
	void Update();
	void Render();

private:
	Vector2 pos_;
	float width_;
	float height_;

	int textureN_;
	int textureS_;
	int textureP_;
	int textureC_;

	int texture1_;
	int texture2_;
	int texture3_;
	int texture4_;
	int texture5_;
	int texture6_;
	int texture7_;
	int texture8_;
	int texture9_;

	int click1_;
	int click2_;

	bool on_;
	bool isPress_;
	bool isSelect_;
	bool canPress_;

	kEngine* system_ = nullptr;//借り
	MaterialConfig material_ = {};

private:

};

