#pragma once
#include "BaseScene.h"
#include "Object\Object.h"
#include "Object\Sprite.h"
#include "Camera\DebugCamera.h"
#include "Player\Player.h"

class Effect1 :public BaseScene
{
public:
	Effect1(kEngine* system);
	~Effect1();
	void Update() override;
	void Draw() override;

	void CameraPart();
	void ImguiPart();

private:
	kEngine* system_ = nullptr;				// 借り

	/// ============== カメラ関連 ==============///
	/// ステージカメラ(mainカメラ)
	std::weak_ptr<Camera> camera_;
	/// デバックカメラ
	std::weak_ptr <DebugCamera> debugCamera_;
	/// 今使っているカメラ
	std::weak_ptr<Camera> usingCamera_;

	/// =========== リソースハンドル ============///
	/// モデルハンドル
	int skydomeModelHandle_ = 0;
	int sphereModelHandle_ = 0;

	/// テキスチャーハンドル
	int boxTextureHandle_ = 0;

	/// =========== ゲームオブジェクト ==========///
	std::unique_ptr<Object> skydome_ = nullptr;
	std::unique_ptr<Object> plane_[200];


	/// =============== フラグ ===============///
	bool useDebugCamera = false;
};

