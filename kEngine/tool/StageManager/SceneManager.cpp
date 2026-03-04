#include "SceneManager.h"

SceneManager* SceneManager::sceneManager_ = nullptr;

SceneManager::SceneManager(kEngine* system)
	: system_(system),
	sceneFactory_(new SceneFactory(system)) {
	sceneUsingNameHandle_ = "CGHK2";

	helperTextureHandle_ = system_->LoadTexture("resources/texture/helper.png");
	helperSprite_ = new SimpleSprite;
	helperSprite_->IntObject(system_);
	helperSprite_->CreateDefaultData();
	helperSprite_->mainPosition.transform.scale = { 0.5f,0.5f,1.0f };
	helperSprite_->mainPosition.transform.translate = { 0.0f,550.0f,0.0f };
	helperSprite_->objectParts_[0].materialConfig->textureHandle = helperTextureHandle_;

	defaultMenu_ = new DefaultMenu(system_);
}

SceneManager::~SceneManager() {
	if (sceneUsing_ != nullptr)
		delete sceneUsing_, sceneUsing_ = nullptr;
	if (sceneOld_ != nullptr)
		delete sceneOld_, sceneOld_ = nullptr;
}

void SceneManager::Initialize(kEngine* system) {

	if (!sceneManager_) {
		sceneManager_ = new SceneManager(system);
	}

}

SceneManager& SceneManager::GetInstance() {
	return *sceneManager_;
}

void SceneManager::SceneChanger() {

	if (sceneUsing_) {
		bool isSceneChange = false;

		switch (sceneUsing_->GetOutcome()) {

		case SceneOutcome::NEXT:
		{
			auto targetScene = sceneFlow_.find(sceneUsingNameHandle_);
			if (targetScene != sceneFlow_.end()) {
				sceneUsingNameHandle_ = targetScene->second;
				isSceneChange = true;
			} else {
				Logger::Log("[kError] SM :: SceneChanger: Scene not found in sceneFlow_: " + sceneUsingNameHandle_);
			}
		}
		break;

		case SceneOutcome::RETRY:
			isSceneChange = true;
			break;

		case SceneOutcome::RETURN:
			isSceneChange = true;
			sceneUsingNameHandle_ = "TITLE";
			break;
		case SceneOutcome::EXIT:
			kEngine::EndGame();
			break;

		}

		if (defaultMenu_->IsBack()) {
			isSceneChange = true;
			sceneUsingNameHandle_ = "TITLE";
		}

		if (defaultMenu_->IsRetry()) {
			isSceneChange = true;
		}

		if (!isSceneChange)return;

		delete sceneUsing_, sceneUsing_ = nullptr;
	}

	sceneUsing_ = sceneFactory_->CreateScene(sceneUsingNameHandle_);
}


void SceneManager::Update() {


	SceneChanger();

	//defaultMenu_->Update();

	if (!defaultMenu_->GetIsPause()) {
		if (sceneUsing_ != nullptr) {
			sceneUsing_->Update();
		}
	}

}

void SceneManager::Render() {

	if (sceneUsing_ != nullptr) {
		sceneUsing_->Draw();
	} else {
	}

	//defaultMenu_->Draw();

#ifdef USE_IMGUI
	ImGuiPart();
#endif
}

void SceneManager::ClearStage() {
	for (auto& ptr : stage) {
		ptr = false;
	}
	delete sceneUsing_, sceneUsing_ = nullptr;
}

#ifdef USE_IMGUI
void SceneManager::ImGuiPart() {
	{
		float fps = system_->GetFPS();
		float fps1s = system_->GetFPSPerSecond();
		float deltaTime = system_->GetDeltaTime();
		ImGui::Begin("FPS");
		ImGui::InputFloat("FPS", &fps);
		ImGui::InputFloat("FPS_1s", &fps1s);
		ImGui::InputFloat("deltaTime", &deltaTime);
		ImGui::End();
	}
	{
		ImGui::Begin("MenuTest");
		if (defaultMenu_->isClicked()) {
			ImGui::Text("IsClicked: True");
		} else {
			ImGui::Text("IsClicked: False");
		}

		if (defaultMenu_->IsRetry()) {
			ImGui::Text("IsRetry: True");
		} else {
			ImGui::Text("IsRetry: False");
		}

		if (defaultMenu_->IsBack()) {
			ImGui::Text("IsBack: True");
		} else {
			ImGui::Text("IsBack: False");
		}
		ImGui::End();
	}
}
#endif
