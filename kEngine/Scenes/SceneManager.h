#pragma once
#include "Data/Render/CPUData/MaterialConfig.h"
#include "AnimationSystem/AnimationEditor.h"
#include "BaseScene.h"
#include "CG3_HK_2/SceneCGHK2.h"
#include "CG4_HK_1/Effect2.h"
#include "DefaultMenu/DefaultMenu.h"
#include "SceneFactory.h"

class SceneManager {
public:
  static void Initialize(kEngine *system);
  static SceneManager &GetInstance();

  /// 複製禁止
  SceneManager(const SceneManager &) = delete;
  SceneManager &operator=(const SceneManager &) = delete;
  SceneManager(SceneManager &&) = delete;
  SceneManager &operator=(SceneManager &&) = delete;

  void Update();
  void Render();
  ;

public:
  std::string sceneUsingNameHandle_ = "NONE";

  void SceneChanger();

private:
  std::map<std::string, std::string> sceneFlow_ = {
      {"TITLE", "PROMPT"},   {"PROMPT", "MOD"},     {"MOD", "TRAVEL"},
      {"TRAVEL", "CONTEST"}, {"CONTEST", "PROMPT"},
  };

private:
  static SceneManager *sceneManager_;

  SceneManager(kEngine *system);
  ~SceneManager();

private:
	std::unique_ptr <BaseScene> sceneUsing_ = nullptr;
	std::unique_ptr <BaseScene> sceneOld_ = nullptr;

private:
  kEngine *system_ = nullptr; // 借り

	std::unique_ptr <SceneFactory> sceneFactory_ = nullptr;

	std::unique_ptr <DefaultMenu> defaultMenu_ = nullptr;

  /// ========= リソースハンドル ========= ///

  // int helperTextureHandle_ = 0;

  /// ============ オブジェクト =========== ///

  // SimpleSprite *helperSprite_ = nullptr;
  // Vector2 helperSpriteScale_ = {1.0f, 1.0f};
  // Vector2 helperSpritePos_ = {50.0f, 50.0f};

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
