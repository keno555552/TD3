#pragma once
#include <vector>
#include <functional>
#include <memory>
#include "Object/Sprite.h"
#include "MathsIncluder.h"
#include "Data/Render/CPUData/materialconfig.h"
#include "kEngine.h"


/// このクラスのSimpleSpriteはmainPositionだけ使ってる
/// 中心点として使う

class DetailButton : public SimpleSprite
{
public:
	DetailButton(kEngine* system);
	~DetailButton();

	static void LoadTexture(kEngine* system);

	void SetWidth(float width);
	void SetHeight(float height);
	void SetIsLock(bool isLock) { buttonState_ = LOCK; }

	void SetNormalColor(Vector4 color) { setColor_[NORMAL] = color; }
	void SetPressColor(Vector4 color) { setColor_[PRESS] = color; }
	void SetSelectColor(Vector4 color) { setColor_[SELECT] = color; }
	void SetLockColor(Vector4 color) { setColor_[LOCK] = color; }

	bool GetIsSelect(Vector2 mouse, float width, float height);
	bool GetIsPress() { return buttonState_ == PRESS; }
	bool GetIsLock() { return buttonState_ == LOCK; }
	void ResetButton();

	void SetButton(Vector2 pos, float width, float height, float layer = 0);

	void Update();
	void Render();


private:

	enum NineCorner{
		LT,CT,RT,
		LM,CM,RM,
		LB,CB,RB,
	};

	enum Static{
		NORMAL,
		PRESS,
		SELECT,
		LOCK,
	};
	int buttonState_ = NORMAL;

private:


	/// ==================== モデルハンドル ===================== ///
	std::unique_ptr<SimpleSprite> sprite_[9];
	
	/// ================== テクスチャーハンドル ================== ///
	int texture_[9];

	/// =================== デフォルトサイズ =================== ///
	Vector2 buttonDefaultSize_ = { 76.0f,86.0f };
	Vector2 spriteDefaultSize_[9]{};

	/// =================== サイズ =================== ///
	/// 各サイズ
	Vector2 buttonSize_ = { 76.0f,86.0f };
	Vector2 spriteSize_[9]{};

	/// ==================== 色設定 ===================== ///

	Vector4 currentColor_ = { 0.22f, 0.28f, 0.36f, 1.0f };

	Vector4 setColor_[4]{};

	kEngine* system_ = nullptr;//借り
	MaterialConfig material_ = {};

	bool isReady_ = false;


private:
	void DrawButton(int buttonStatic);

	void SetQuad(SimpleSprite* sprite, Vector2 lt, Vector2 size);

#ifdef _DEBUG
	void ImGuiPart();
#endif // _DEBUG
};