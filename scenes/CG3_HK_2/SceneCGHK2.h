#pragma once
#include "BaseScene.h"
#include "Object\Object.h"
#include "Object\Sprite.h"
#include "Light.h"
#include "DebugCamera.h"
#include "Player\Player.h"
#include "tool\MapChip\MapChipField.h"
#include "tool\DebugTool\DebugObject.h"


class SceneCGHK2 : public BaseScene
{
public:
	SceneCGHK2(kEngine* system);
	~SceneCGHK2();

	void Update() override;
	void Draw() override;
	
private:
	kEngine* system_ = nullptr;				// 借り

	/// ============= ライト関連 ==============///
	Light* light1_ = nullptr;
	Light* light2_ = nullptr;
	Light* light3_ = nullptr;
	AreaLight* areaLight_ = nullptr;
	AreaLight* areaLight2_ = nullptr;


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
	int playerModelHandle_ = 0;

	/// テキスチャーハンドル
	int boxTextureHandle_ = 0;
	int tryTextureHandle_ = 0;
	int uvTextureHandle_ = 0;
	int whiteTextureHandle_ = 0;

	/// サントラハンドル
	int soundHandle_ = 0;

	/// =========== ゲームオブジェクト ==========///
	Object* skydome_ = nullptr;
	Object* ground_ = nullptr;
	Player* player_ = nullptr;
	DebugObject* debugObject_ = nullptr;
	SimpleSprite* sprite_ = nullptr;
	SimpleSprite* sprite2_ = nullptr;


	/// =============== フラグ ================///
	bool useDebugCamera = false;

	int isBlinn_Phong = true;

	int textNumber = 0;
	bool isShowCenterPoint = false;
	bool isShowCenterNumber = false;

	int lightMode = 0;
	bool isNormalLight = true;
	bool isMultiPointLight = false;
	bool isMultiSpotLight = false;
	bool isMultiAreaLight = false;

	void InitNormalLight();
	void InitMultiPointLight();
	void InitMultiSpotLight();
	void InitMultiAreaLight();
private:
	/// ============= シーン内命令 =============///
	void CameraPart();
#ifdef USE_IMGUI
	void ImGuiPart();
	void SelectLightMode();
#endif
};

