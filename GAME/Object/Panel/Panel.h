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

class Panel : public SimpleSprite
{
public:
	Panel(kEngine* system);
	~Panel();

	static void LoadTexture(kEngine* system);

	void SetPos(Vector2 pos,float layer = 0);
	void SetWidth(float width);
	void SetHeight(float height);
	void SetScale(float scale);
	void SetIsLock(bool isLock) { panelState_ = LOCK; }

	void SetOpen() { animationTimer_.Reset0(); panelState_ = START; }
	void SetClose() { animationTimer_.ResetM(); panelState_ = CLOSE; }

	bool GetIsSelect(Vector2 mouse, float width, float height);
	bool GetIsLock() { return panelState_ == LOCK; }
	bool GetIsNormal() { return panelState_ == NORMAL; }
	bool GetIsEnd() { return panelState_ == END; }
	void ResetPanel();

	void SetPanel(Vector2 pos, float width, float height, float layer = 0);

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
		START,
		CLOSE,
		END,
		LOCK,
		STATIC_NUM,
	};
	int panelState_ = NORMAL;

private:

	/// ==================== モデルハンドル ===================== ///
	std::unique_ptr<SimpleSprite> sprite_[9];
	
	/// ================== テクスチャーハンドル ================== ///
	int texture_[9];

	/// =================== デフォルトサイズ ==================== ///
	Vector2 buttonDefaultSize_ = { 76.0f,86.0f };
	Vector2 spriteDefaultSize_[9]{};

	/// ======================= サイズ ======================== ///
	/// 各サイズ
	Vector2 buttonSize_ = { 76.0f,86.0f };
	Vector2 spriteSize_[9]{};

	kEngine* system_ = nullptr;//借り
	MaterialConfig material_ = {};

	float scale_ = 1.0f;

	/// ======================= フラグ ======================== ///

	bool isReady_ = false;

	/// ==================== 開け閉じ関連 ====================== ///

	Timer animationTimer_;

private:
	void DrawPanel(int buttonStatic);

	void SetQuad(SimpleSprite* sprite, Vector2 lt, Vector2 size);

	/// ==================== 開け閉じ関連 ====================== ///

	std::vector<std::function<void()>> PanelAnimation;

	void OpenPanel();

	void ClosePanel();

#ifdef _DEBUG
	void ImGuiPart();
#endif 
};