#include "SceneManager.h"

std::unique_ptr <SceneManager> SceneManager::sceneManager_ = nullptr;

void SceneManager::Initialize(kEngine* system) {
	system_ = system;

	sceneFactory_ = std::make_unique<SceneFactory>(system);
	//sceneUsingNameHandle_ = "CGHK2";
	sceneUsingNameHandle_ = "TITLE";

	defaultMenu_ = std::make_unique <DefaultMenu>(system_);

	// パーズ案内UIの初期化
	pauseTextureHandle_ = system_->LoadTexture("GAME/resources/pause_UI.png");
	pauseSprite_ = std::make_unique<SimpleSprite>();
	pauseSprite_->IntObject(system_);
	pauseSprite_->CreateDefaultData();
	if (!pauseSprite_->objectParts_.empty()) {
		pauseSprite_->objectParts_[0].materialConfig->textureHandle = pauseTextureHandle_;
	}
	pauseSprite_->mainPosition.transform.scale = { 0.35f, 0.35f, 1.0f };
	pauseSprite_->mainPosition.transform.translate = { 5.0f, 650.0f, 0.0f };
	pauseSprite_->Update(nullptr);
}

void SceneManager::Finalize() {
	sceneUsing_.reset();
	sceneOld_.reset();
	sceneFactory_.reset();
	defaultMenu_.reset();
	pauseSprite_.reset();
}

SceneManager& SceneManager::GetInstance() {
	if (!sceneManager_) {
		sceneManager_ = std::make_unique<SceneManager>(ConstructorKey());
	}
	return *sceneManager_;
}

void SceneManager::SceneChanger() {

	if (sceneUsing_) {

		/// ステージが変わるかどうかのフラグ
		bool isSceneChange = false;

		/// シーンの結果を取得
		SceneOutcome outcome = sceneUsing_->GetOutcome();

		/// 戻りがない場合シーン転移しないのでreturn
		if (outcome == SceneOutcome::NONE)return;

		/// まずはシーンがマップに存在するかを確認する
		auto targetFlow = sceneFlow_.find(sceneUsingNameHandle_);
		if (targetFlow == sceneFlow_.end()) {

			/// もし見つからなかった場合、エラーログを出力する
			Logger::Log("[kError] SM :: SceneChanger: Scene not found in sceneFlow_: " + sceneUsingNameHandle_);

		} else {

			/// もし見つかった場合、シーンの結果がマップに存在するかを確認する
			auto targetScene = targetFlow->second.find(outcome);
			if (targetScene == targetFlow->second.end()) {

				/// 同じく見つからなかった場合
				Logger::Log("[kError] SM :: SceneChanger: Scene not found in sceneFlow_: " + sceneUsingNameHandle_);

			} else {

				/// 見つかった場合、次のシーンに遷移するためのフラグを立てる
				sceneUsingNameHandle_ = targetScene->second;
				isSceneChange = true;
			}
		}

		/// デフォルトメニューによって流れを上書きする
		if (defaultMenu_->IsBack()) {
			isSceneChange = true;
			sceneUsingNameHandle_ = "TITLE";
		}
		if (defaultMenu_->IsRetry()) {
			isSceneChange = true;
		}

		/// EXITが押されたらゲーム終了
		if (outcome == SceneOutcome::EXIT) {
			kEngine::EndGame();
			return;
		}

		if (!isSceneChange)return;

		sceneUsing_.reset();
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

	if (sceneUsingNameHandle_ != "SPLASH") {
		defaultMenu_->Draw();

		if (pauseSprite_) {
			if (sceneUsing_ != nullptr) {
				float alpha = 1.0f - sceneUsing_->GetFadeAlpha();

				if (defaultMenu_ && defaultMenu_->GetIsPause()) {
					alpha = 0.0f;
				}

				pauseSprite_->objectParts_[0].materialConfig->textureColor.w = alpha;

				if (alpha > 0.0f) {
					pauseSprite_->Draw();
				}
			} else {
				pauseSprite_->Draw();
			}
		}
	}
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
		//float fps = system_->GetFPS();
		//float fps1s = system_->GetFPSPerSecond();
		//float deltaTime = system_->GetDeltaTime();
		//ImGui::Begin("FPS");
		//ImGui::InputFloat("FPS", &fps);
		//ImGui::InputFloat("FPS_1s", &fps1s);
		//ImGui::InputFloat("deltaTime", &deltaTime);
		//ImGui::End();
	}
	{
		//ImGui::Begin("MenuTest");
		//if (defaultMenu_->isClicked()) {
		//	ImGui::Text("IsClicked: True");
		//} else {
		//	ImGui::Text("IsClicked: False");
		//}
		//
		//if (defaultMenu_->IsRetry()) {
		//	ImGui::Text("IsRetry: True");
		//} else {
		//	ImGui::Text("IsRetry: False");
		//}
		//
		//if (defaultMenu_->IsBack()) {
		//	ImGui::Text("IsBack: True");
		//} else {
		//	ImGui::Text("IsBack: False");
		//}
		//ImGui::End();
	}
}
#endif
