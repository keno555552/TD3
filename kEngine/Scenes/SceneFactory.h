#pragma once
#include <string>
#include <unordered_map>

#include "BaseSceneFactory.h"

#include "BaseScene.h"
#include "AnimationSystem/AnimationEditor.h"
#include "DefaultMenu/DefaultMenu.h"
#include "CG4_HK_1/Effect2.h"
#include "CG3_HK_2/SceneCGHK2.h"

// このゲーム用のシーン工場
class SceneFactory : public BaseSceneFactory
{
public:

    SceneFactory(kEngine* system);
	~SceneFactory() override = default;

    /// <summary>
    /// シーン生成
    /// </summary>
    /// <param name="sceneName">シーン名</param>
    /// <returns>生成したシーン</returns>
    std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) override;


private:
	kEngine* system_ = nullptr;
	std::unordered_map<std::string, std::function<std::unique_ptr<BaseScene>()>> sceneRegistry_;
};
