#pragma once
#include "MaterialConfig.h"
#include "SceneFactory.h"
#include "BaseScene.h"
#include "AnimationSystem/AnimationEditor.h"
#include "DefaultMenu/DefaultMenu.h"
#include "CG4_HK_1/Effect2.h"
#include "CG3_HK_2/SceneCGHK2.h"

class SceneManager {
public:

	static void Initialize(kEngine* system);
	static SceneManager& GetInstance();

	/// 複製禁止
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;
	SceneManager(SceneManager&&) = delete;
	SceneManager& operator=(SceneManager&&) = delete;

	void Update();
	void Render();
;

public:
	std::string sceneUsingNameHandle_ = "NONE";

	void SceneChanger();

private:
	std::map<std::string,std::string> sceneFlow_ = {
		//{"TITLE","STAGE_01"},
	};


private:
	static SceneManager* sceneManager_;

	SceneManager(kEngine* system);
	~SceneManager();
private:
	BaseScene* sceneUsing_ = nullptr;
	BaseScene* sceneOld_ = nullptr;

private:
	kEngine* system_ = nullptr; // 借り

	SceneFactory* sceneFactory_ = nullptr;

	DefaultMenu* defaultMenu_ = nullptr;

	/// ========= リソースハンドル ========= ///

	int helperTextureHandle_ = 0;

	/// ============ オブジェクト =========== ///

	SimpleSprite* helperSprite_ = nullptr;
	Vector2 helperSpriteScale_ = { 1.0f,1.0f };
	Vector2 helperSpritePos_ = { 50.0f,50.0f };

	/// =============== ステージ管理 ===============///
	
	bool stage[10]{};

	bool isFirst_ = true;

	bool isFromTitle_ = true;

	bool isReset_ = false;

	bool isEnd_ = false;

private:
	/// ============= ステージマネージメント ============///
	void ClearStage();

#ifdef USE_IMGUI
	void ImGuiPart();
#endif
};
