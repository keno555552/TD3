#pragma once
#include "BaseScene.h"
#include "Object\Object.h"
#include "Object\Sprite.h"
#include "Lighting\Light.h"
#include "Camera\DebugCamera.h"
#include "GAME\Object\DetailButton\DetailButton.h"
#include "GAME\Object\Panel\Panel.h"
#include "GameObject\button\button.h"
#include "Tool\DefaultMenu\DefaultMenu.h"

class UITest : public BaseScene
{
public:
	UITest(kEngine* system);
	~UITest();

	void Update() override;
	void Draw() override;

private:
	kEngine* system_ = nullptr;							// 借り

	/// ============= ライト関連 ==============///
	std::unique_ptr<Light> light1_ = nullptr;
	std::unique_ptr<Light> light2_ = nullptr;
	std::unique_ptr<Light> light3_ = nullptr;
	std::unique_ptr<AreaLight> areaLight_ = nullptr;
	std::unique_ptr<AreaLight> areaLight2_ = nullptr;


	/// ============== カメラ関連 ==============///
	/// ステージカメラ(mainカメラ)
	std::weak_ptr<Camera> camera_;
	/// デバックカメラ
	std::weak_ptr <DebugCamera> debugCamera_ ;
	/// 今使っているカメラ
	std::weak_ptr<Camera> usingCamera_;			

	/// =========== リソースハンドル ============///
	/// モデルハンドル
	int skydomeModelHandle_ = 0;
	int smallStageHandel_ = 0;
	int objectHandle_ = 0;
	int BGObjectHandle_ = 0;

	/// テキスチャーハンドル
	int whiteTextureHandle_ = 0;
	int clicleTextureHandle_ = 0;
	int effectTextureHandle_ = 0;
	int uvCheckerTextureHandle_ = 0;

	/// サントラハンドル
	int soundHandle_ = 0;

	/// パーティクルハンドル
	int particleHandle_ = 0;
	int particleHandle2_ = 0;

	int ddsTest = 0;

	/// =========== ゲームオブジェクト ==========///
	std::unique_ptr<Object> skydome_ = nullptr;
	std::unique_ptr<Object> skybox_ = nullptr;
	std::unique_ptr<Object> ground_ = nullptr;
	std::unique_ptr<Object> box_ = nullptr;
	std::unique_ptr<Object>	BGObject_ = nullptr;

	std::unique_ptr<DetailButton> detailButton_ = nullptr;
	std::unique_ptr<Button> button_ = nullptr;
	std::unique_ptr<Panel> panel_ = nullptr;
	std::unique_ptr<DefaultMenu> defaultMenu_ = nullptr;

	/// =============== フラグ ================///
	bool useDebugCamera = false;

	bool isHit = false;

	bool isPress_ = false;

	/// =============== その他 ================///

	SphereBuildMaterial sphereBuildMaterial_ = {};
	CylinderBuildMaterial cylinderBuildMaterial_ = {};
	RenderCommand renderCommand_ = {};

private:
	/// ============= シーン内命令 =============///
	void CameraPart();
	void MouseLogic();
#ifdef USE_IMGUI
	void ImGuiPart();
#endif

};

