#pragma once
#include "Camera.h"

class kEngine;
class DebugCamera :public Camera {
public:

	/// CameraManagerからのみ生成・破棄可能
	friend class CameraManager;
	friend class kEngine;

	/// 更新
	void Update() override;

	/// Mouse移動
	void MouseControlUpdate();

	/// 操作設定
	void isKeyAvailable(bool available);

	void isKeyQAvailable(bool available) { isQ_ = available; }
	void isKeyEAvailable(bool available) { isE_ = available; }
	void isKeyAAvailable(bool available) { isA_ = available; }
	void isKeyDAvailable(bool available) { isD_ = available; }
	void isKeyWAvailable(bool available) { isW_ = available; }
	void isKeySAvailable(bool available) { isS_ = available; }
	void isKeyZAvailable(bool available) { isZ_ = available; }
	void isKeyRAvailable(bool available) { isR_ = available; }
	void isMouseMiddleAvailable(bool available) { isQ_ = available; }
	void isMouseRightvailable(bool available) { isQ_ = available; }
	void isGamePadAvailable(bool available) { isQ_ = available; }

private:
	/// 初期化
	DebugCamera(kEngine* system);
	~DebugCamera() = default;
private:
	kEngine* system_ = nullptr;

private:
	/// 操作可能フラグ
	bool isQ_ = true;
	bool isE_ = true;
	bool isA_ = true;
	bool isD_ = true;
	bool isW_ = true;
	bool isS_ = true;
	bool isZ_ = true;
	bool isC_ = true;
	bool isR_ = true;
	bool isMouseM_ = true;
	bool isMouseR_ = true;
	bool isGamePad_ = true;
};
