#pragma once
#include "MathsIncluder.h"
#include "Object/Sprite.h"
#include "kEngine.h"

//enum{
//	CANT,
//	NORMAL,
//	SELECT,
//	PRESS
//};

class Button : public SimpleSprite
{
public:
	Button(kEngine* system);

	/// ================= 初期化、更新、描画関連 ================= ///
	void Init(Vector3 pos, float width, float height, int textureN, int textureS, int textureP, int textureC, int click1, int click2);
	void Update();
	void Render();

	/// ================= 設定変更関連 ================= ///
	void SetPos(Vector2 pos);
	void SetWidth(float width);
	void SetHeight(float height);
	void SetCanPress() { canPress_ = true; }
	void SetCannotPress() { canPress_ = false; }
	void SetFollowObject(SpriteData* followObject = nullptr);
	void ResetButton();

	/// ================= 状態取得関連 ================= ///
	bool GetPress() { return isPress_; }
	bool GetCanPress() { return canPress_; }
	bool GetIsClick() { return isClick_; }
	bool GetIsRelease() { return isRelease_; }
	bool GetOnOff() { return on_; }
	bool GetResetOnOff();
	bool GetIsSelect(Vector2 mouse, float width, float height);

	/// ================= texture関連 ================= ///
	void ClearAllTexture();
	/// textureNをセット、設定しないバケットtextureがないと見る
	void ChangeTextureNHandle(int i = -1) { textureN_ = textureN_; }
	void ChangeTextureNColor(Vector4 color) { textureColorN_ = color; }
	/// textureNをセット、設定しないバケットtextureがないと見る
	void ChangeTextureSHandle(int i = -1) { textureS_ = textureN_; }
	void ChangeTextureSColor(Vector4 color) { textureColorS_ = color; }
	/// textureNをセット、設定しないバケットtextureがないと見る
	void ChangeTexturePHandle(int i = -1) { textureP_ = textureN_; }
	void ChangeTexturePColor(Vector4 color) { textureColorP_ = color; }
	/// textureNをセット、設定しないバケットtextureがないと見る
	void ChangeTextureCHandle(int i = -1) { textureC_ = textureN_; }
	void ChangeTextureCColor(Vector4 color) { textureColorC_ = color; }

private:

	/// Sprite関連
	float width_{0};
	float height_{0};

	/// textureHandle
	int textureN_ = -1;
	int textureS_ = -1;
	int textureP_ = -1;
	int textureC_ = -1;

	/// textureColor
	Vector4 textureColorN_ = { 1.0f,1.0f,1.0f,1.0f };
	Vector4 textureColorS_ = { 1.0f,1.0f,1.0f,1.0f };
	Vector4 textureColorP_ = { 1.0f,1.0f,1.0f,1.0f };
	Vector4 textureColorC_ = { 1.0f,1.0f,1.0f,1.0f };

	/// SEHandle
	int click1_ = -1;
	int click2_ = -1;

	/// キーフラグ
	bool on_ = false;
	bool isPress_ = false;
	bool isClick_ = false;
	bool isRelease_ = false;
	bool isSelect_ = false;
	bool canPress_ = true;
	bool wasPressed_ = false;

	/// システム関連
	kEngine* system_ = nullptr;//借り
	MaterialConfig material_ = {};

private:
	void SetCorner(float width, float height);
	void SetCornerWidth(float width);
	void SetCornerHeight(float height);

	Vector2 GetFollowPosition();
};