#pragma once
#include <string>
#include <unordered_map>

#include "BaseSceneFactory.h"

#include "AnimationSystem/AnimationEditor.h"
#include "BaseScene.h"
#include "CG3_HK_2/SceneCGHK2.h"
#include "CG4_HK_1/Effect2.h"
#include "DefaultMenu/DefaultMenu.h"
#include "GAME/scenes/ContestScene.h"
#include "GAME/scenes/ModScene.h"
#include "GAME/scenes/PromptScene.h"
#include "GAME/scenes/TitleScene.h"
#include "GAME/scenes/TravelScene.h"

// このゲーム用のシーン工場
class SceneFactory : public BaseSceneFactory {
public:
  SceneFactory(kEngine *system);
  ~SceneFactory() override = default;

  /// <summary>
  /// シーン生成
  /// </summary>
  /// <param name="sceneName">シーン名</param>
  /// <returns>生成したシーン</returns>
  BaseScene *CreateScene(const std::string &sceneName) override;

private:
  kEngine *system_ = nullptr;
  std::unordered_map<std::string, std::function<BaseScene *()>> sceneRegistry_;
};
