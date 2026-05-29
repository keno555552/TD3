#pragma once
#include "BaseScene.h"
#include "Object\Object.h"
#include "Object\Sprite.h"
#include "Particle\P_Fire\P_Fire.h"
#include "Camera\DebugCamera.h"
#include "Player\Player.h"
#include "Particle\P_Ball\P_Ball.h"
#include <random>

class Effect2 :public BaseScene
{
public:
	Effect2(kEngine* system);
	~Effect2();
	void Update() override;
	void Draw() override;


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
	int planeModelHandle_ = 0;

	/// テキスチャーハンドル
	int boxTextureHandle_ = 0;

	/// =========== ゲームオブジェクト =========///
	std::unique_ptr<Object> skydome_ = nullptr;
	std::unique_ptr<P_Ball> ball_ = nullptr;
	std::unique_ptr<Object> plane_ = nullptr;

	/// ============= ゲーム内データ ===========///
	std::random_device seedGenerator;
	std::mt19937 randomEngine{ seedGenerator() };
	float randomNumber{};


	/// =============== フラグ ===============///
	bool useDebugCamera = false;
	bool isWind_ = false;

	/// ============= シーン内命令 =============///
	void CameraPart();

#ifdef USE_IMGUI
	void ImguiPart();
#endif

};