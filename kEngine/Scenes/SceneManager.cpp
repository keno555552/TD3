#include "SceneManager.h"

std::unique_ptr <SceneManager> SceneManager::sceneManager_ = nullptr;


void SceneManager::Initialize(kEngine* system) {
	system_ = system;
	sceneFactory_ = std::make_unique<SceneFactory>(system);
	//sceneUsingNameHandle_ = "CGHK2";
	sceneUsingNameHandle_ = "TITLE";

	defaultMenu_ = std::make_unique <DefaultMenu>(system_);
}

void SceneManager::Finalize() {
	sceneUsing_.reset();
	sceneOld_.reset();
	sceneFactory_.reset();
	defaultMenu_.reset();
}

SceneManager& SceneManager::GetInstance() {
	if (!sceneManager_) {
		sceneManager_.reset(new SceneManager);
	}
	return *sceneManager_;
}

void SceneManager::SceneChanger() {

	if (sceneUsing_) {
		bool isSceneChange = false;

		switch (sceneUsing_->GetOutcome()) {

		case SceneOutcome::NEXT: {
			auto targetScene = sceneFlow_.find(sceneUsingNameHandle_);
			if (targetScene != sceneFlow_.end()) {
				sceneUsingNameHandle_ = targetScene->second;
				isSceneChange = true;
			} else {
				Logger::Log(
					"[kError] SM :: SceneChanger: Scene not found in sceneFlow_: " +
					sceneUsingNameHandle_);
			}
		} break;

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

		if (!isSceneChange)
			return;
	}

	sceneUsing_ = sceneFactory_->CreateScene(sceneUsingNameHandle_);
}

void SceneManager::Update() {

	SceneChanger();

	// defaultMenu_->Update();

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

	// defaultMenu_->Draw();

#ifdef USE_IMGUI
	ImGuiPart();
#endif
}

void SceneManager::ClearStage() {
	for (auto& ptr : stage) {
		ptr = false;
	}
	sceneUsing_.reset();
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
