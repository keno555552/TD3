#pragma once
#include "BaseScene.h"
#include "Object\Object.h"
#include "Object\Sprite.h"
#include "Lighting\Light.h"
#include "Camera\DebugCamera.h"
#include "GAME\Object\DetailButton\DetailButton.h"
#include "GAME\Object\Panel\Panel.h"

class UITest : public BaseScene
{
public:
	UITest(kEngine* system);
	~UITest();

	void Update() override;
	void Draw() override;

private:
	kEngine* system_ = nullptr;				// 借り

	/// ============= ライト関連 ==============///
	std::unique_ptr<Light> light1_ = nullptr;
	std::unique_ptr<Light> light2_ = nullptr;
	std::unique_ptr<Light> light3_ = nullptr;
	std::unique_ptr<AreaLight> areaLight_ = nullptr;
	std::unique_ptr<AreaLight> areaLight2_ = nullptr;


	/// ============== カメラ関連 ==============///
	/// ステージカメラ(mainカメラ)
	Camera* camera_ = nullptr;
	/// デバックカメラ
	DebugCamera* debugCamera_ = nullptr;
	/// 今使っているカメラ
	Camera* usingCamera_ = nullptr;			// 借り用

	/// =========== リソースハンドル ============///
	/// モデルハンドル
	int skydomeModelHandle_ = 0;

	/// テキスチャーハンドル
	int whiteTextureHandle_ = 0;

	/// サントラハンドル
	int soundHandle_ = 0;

	/// =========== ゲームオブジェクト ==========///
	std::unique_ptr<Object> skydome_ = nullptr;
	std::unique_ptr<Object> ground_ = nullptr;
	std::unique_ptr<Object> box_ = nullptr;

	std::unique_ptr<DetailButton> detailButton_ = nullptr;
	std::unique_ptr<Panel> panel_ = nullptr;

	/// =============== フラグ ================///
	bool useDebugCamera = false;

	bool isHit = false;

	bool isPress_ = false;

private:
	/// ============= シーン内命令 =============///
	void CameraPart();
#ifdef USE_IMGUI
	void ImGuiPart();
#endif



};

