#include "SceneFactory.h"
#include "Logger.h"

SceneFactory::SceneFactory(kEngine* system)
	: system_(system) {
	sceneRegistry_["ANIMATIONEDITOR"] = [this]() { return new AnimationEditor(system_); };
	sceneRegistry_["EFFECT2"] = [this]() { return new Effect2(system_); };
	sceneRegistry_["CGHK2"] = [this]() { return new SceneCGHK2(system_); };
	sceneRegistry_["TITLE"] = [this]() { return new TitleScene(system_); };
	sceneRegistry_["PROMPT"] = [this]() { return new PromptScene(system_); };
	sceneRegistry_["MOD"] = [this]() { return new ModScene(system_); };
	sceneRegistry_["TRAVEL"] = [this]() { return new TravelScene(system_); };
	sceneRegistry_["CONTEST"] = [this]() { return new ContestScene(system_); };
}

BaseScene* SceneFactory::CreateScene(const std::string& sceneName) {
	auto it = sceneRegistry_.find(sceneName);
	if (it != sceneRegistry_.end()) {
		return it->second();
	}
	Logger::Log("[kError] SF ::CreateScene: Scene not found: " + sceneName);
	return nullptr;
}
