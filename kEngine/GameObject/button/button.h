#pragma once
#include "MathsIncluder.h"
#include "materialconfig.h"
#include "kEngine.h"

//enum {
//	CANT,
//	NORMAL,
//	SELECT,
//	PRESS
//};

class Button
{
public:
	Button(kEngine* system);

	bool GetPress();
	bool GetCanPress();
	bool GetOnOff();
	bool GetResetOnOff();
	void SetPos(Vector2 pos);
	void SetWidth(float width);
	void SetHeight(float height);
	void SetCanPress();
	void SetCantPress();
	
	bool CheckIsSelect(Vector2 mouse, float width, float height);
	void ResetButton();

	void Init(Vector2 pos, float width,float height, int textureN, int textureS, int textureP, int textureC,int click1,int click2);
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

	int click1_;
	int click2_;

	bool on_;
	bool isPress_;
	bool isSelect_;
	bool canPress_;

	kEngine* system_ = nullptr;//借り
	MaterialConfig material_ = {};
};

