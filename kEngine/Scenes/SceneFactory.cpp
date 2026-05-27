#include "SceneFactory.h"
#include "Logger.h"
#include "../../GAME/scenes/TravelScene.h"
#include "../../GAME/scenes/PromptScene.h"
#include "../../GAME/scenes/ContestScene.h"
#include "../../GAME/scenes/ModScene.h"
#include "../../GAME/scenes/SplashScene.h"

SceneFactory::SceneFactory(kEngine* system)
	: system_(system) {
	sceneRegistry_["ANIMATIONEDITOR"] = [this]() { return std::make_unique <AnimationEditor>(system_); };
	sceneRegistry_["EFFECT2"] = [this]() { return std::make_unique <Effect2>(system_); };
	sceneRegistry_["CGHK2"] = [this]() { return	std::make_unique <SceneCGHK2>(system_); };
	sceneRegistry_["TITLE"] = [this]() { return	std::make_unique <TitleScene>(system_); };
	sceneRegistry_["SPLASH"] = [this]() { return std::make_unique <SplashScene>(system_); };
	sceneRegistry_["PROMPT"] = [this]() { return std::make_unique <PromptScene>(system_); };
	sceneRegistry_["MOD"] = [this]() { return std::make_unique <ModScene>(system_); };
	sceneRegistry_["TRAVEL"] = [this]() { return std::make_unique <TravelScene>(system_); };
	sceneRegistry_["CONTEST"] = [this]() { return std::make_unique <ContestScene>(system_); };
}

std::unique_ptr<BaseScene> SceneFactory::CreateScene(const std::string& sceneName) {
	auto it = sceneRegistry_.find(sceneName);
	if (it != sceneRegistry_.end()) {
		return std::move(it->second());
	}
	Logger::Log("[kError] SF ::CreateScene: Scene not found: " + sceneName);
	return nullptr;
}

