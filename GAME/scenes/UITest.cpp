#include "UITest.h"


UITest::UITest(kEngine* system) {
	/// =========== システム初期化 ============///
	system_ = system;

	light1_ = std::make_unique<Light>();
	light1_->direction = { -0.5f, -1.0f, -0.3f };
	light1_->color = { 1.0f, 1.0f, 1.0f };
	light1_->intensity = 1.0f;
	system_->AddLight(light1_.get());

	light2_ = std::make_unique<Light>();
	light2_->lightingType = LightingType::PointLight;
	light2_->position = { 3.0f, 1.0f, 0.0f };
	light2_->range = 10.0f;
	light2_->color = { 1.0f, 0.2f, 0.2f };
	light2_->intensity = 2.0f;
	system_->AddLight(light2_.get());

	light3_ = std::make_unique<Light>();
	light3_->lightingType = LightingType::SpotLight;
	light3_->position = { 0.0f, 4.0f, 0.0f };   // 在物體正上方
	light3_->direction = { 0.0f, -1.0f, 0.0f };  // 往下照
	light3_->angle = 0.34906585f;
	light3_->range = 12.0f;
	light3_->color = { 0.3f, 0.3f, 1.0f };   // 藍光
	light3_->intensity = 3.0f;
	system_->AddLight(light3_.get());

	areaLight_ = std::make_unique<AreaLight>();
	areaLight_->lightingType = LightingType::AreaLight;
	areaLight_->position = { 0.0f, 2.0f, 0.0f };
	areaLight_->right = { 1.0f, 0.0f, 0.0f };
	areaLight_->up = { 0.0f,  0.0f, 1.0f };
	areaLight_->width = 1.0f;
	areaLight_->height = 1.0f;
	areaLight_->color = { 1.0f, 0.9f, 0.7f }; // 暖色光
	areaLight_->intensity = 5.0f;
	areaLight_->range = 10.0f;
	system_->AddLight(areaLight_.get());

	areaLight2_ = std::make_unique<AreaLight>();
	areaLight2_->ableLight = false;
	areaLight2_->lightingType = LightingType::AreaLight;
	areaLight2_->position = { 0.0f, 2.0f, 0.0f };
	areaLight2_->right = { 1.0f, 0.0f, 0.0f };
	areaLight2_->up = { 0.0f,  0.0f, 1.0f };
	areaLight2_->width = 1.0f;
	areaLight2_->height = 1.0f;
	areaLight2_->color = { 1.0f, 0.9f, 0.7f }; // 暖色光
	areaLight2_->intensity = 5.0f;
	areaLight2_->range = 10.0f;
	system_->AddLight(areaLight2_.get());

	debugCamera_ = system_->CreateDebugCamera();
	camera_ = system_->CreateCamera();
	usingCamera_ = camera_;
	system_->SetCamera(usingCamera_);

	/// =========== リソースロード ============///
	skydomeModelHandle_ = system_->SetModelObj("./kEngine/EngineAssets/TemplateResource/object/skydome/skydome.obj");

	whiteTextureHandle_ = system_->LoadTexture("./kEngine/EngineAssets/TemplateResource/texture/white5x5.png");

	skydome_ = std::make_unique<Object>();
	skydome_->IntObject(system_);
	skydome_->CreateModelData(skydomeModelHandle_);

	ground_ = std::make_unique<Object>();
	ground_->IntObject(system_);
	ground_->CreateDefaultData();
	ground_->modelHandle_ = config::default_Cube_MeshBufferHandle_;
	ground_->objectParts_[0].materialConfig->textureHandle = whiteTextureHandle_;
	ground_->mainPosition.transform.scale = Vector3(50.0f, 0.1f, 50.0f);
	ground_->mainPosition.transform.translate = Vector3(0.0f, -1.0f, 0.0f);

	box_ = std::make_unique<Object>();
	box_->IntObject(system_);
	box_->CreateDefaultData();
	box_->modelHandle_ = config::default_Sphere_MeshBufferHandle_;
	box_->objectParts_[0].materialConfig->textureHandle = whiteTextureHandle_;
	box_->mainPosition.transform.scale = Vector3(0.5f, 0.5f, 0.5f);
	box_->mainPosition.transform.translate = Vector3(0.0f, 0.0f, 0.0f);


	detailButton_ = std::make_unique<DetailButton>(system_);
	detailButton_->SetButton({ 100.0f,100.0f }, 200.0f, 80.0f);

	panel_ = std::make_unique<Panel>(system_);
	panel_->SetPanel({ 720.0f,300.0f }, 500.0f, 500.0f);
}

UITest::~UITest() {
	system_->DestroyCamera(camera_);
	system_->DestroyCamera(debugCamera_);

	light1_.reset();
	light2_.reset();
	light3_.reset();
	areaLight_.reset();
	areaLight2_.reset();

	ground_.reset();
	skydome_.reset();
	box_.reset();
	detailButton_.reset();
}


void UITest::Update() {

	CameraPart();

	/// Skydome更新
	skydome_->Update(usingCamera_);

	ground_->Update(usingCamera_);

	detailButton_->Update();

	panel_->Update();

	Ray mouseRay = usingCamera_->ScreenPointToRay(system_->GetMousePosVector2());

	Sphere target;
	target.center = box_->mainPosition.transform.translate;
	target.radius = 0.5f;

	crashDecision(target, mouseRay) == true ? isHit = true : isHit = false;

	if (system_->GetTriggerOn(DIK_0)) {
		if (useDebugCamera)useDebugCamera = false;
		else useDebugCamera = true;
	}

	if (system_->GetTriggerOn(DIK_SPACE)) {
		outcome_ = SceneOutcome::NEXT;
	}

	if (system_->GetTriggerOn(DIK_Q)) {
		system_->SoundPlaySE(soundHandle_, 0.5f);
	}

	/// Panelのロジック処理
	if (isPress_) {
		if (panel_->GetIsEnd()) {
			panel_->SetOpen();
		}
	} else {
		if (panel_->GetIsNormal()) {
			panel_->SetClose();
		}
	}
	///// Panelのロジック処理
	//if (isHit) {
	//	if (panel_->GetIsEnd()) {
	//		panel_->SetOpen();
	//	}
	//} else {
	//	if (panel_->GetIsNormal()) {
	//		panel_->SetClose();
	//	}
	//}


}

void UITest::Draw() {

	/// 実体処理
	skydome_->Draw();
	ground_->Draw();
	box_->Draw();
	detailButton_->Render();
	panel_->Render();

#ifdef USE_IMGUI
	/// ImGui処理
	ImGuiPart();
#endif
}

void UITest::CameraPart() {
	if (useDebugCamera) {
		usingCamera_ = debugCamera_;
		debugCamera_->MouseControlUpdate();
	} else {
		//Transform cameraTransform = CreateDefaultTransform();
		//cameraTransform.translate.x = player_->mainPosition.transform.translate.x;
		//cameraTransform.translate.y = player_->mainPosition.transform.translate.y + 0.5f;
		//cameraTransform.translate.z = player_->mainPosition.transform.translate.z - 15.0f;
		//camera_->SetCamera(cameraTransform);
		usingCamera_ = camera_;
	}
	//usingCamera_->Update();
	system_->SetCamera(usingCamera_);
}

#ifdef USE_IMGUI
void UITest::ImGuiPart() {
	ImGui::Begin("DebugCamera");
	ImGui::Checkbox("isUse", &useDebugCamera);
	ImGui::End();

	ImGui::Begin("HitCheck");
	ImGui::Checkbox("isMouseHitAABB", &isHit);
	ImGui::End();

	bool isPress = detailButton_->GetIsPress();

	ImGui::Begin("DetailButton");
	ImGui::SliderFloat3("Position", &detailButton_->mainPosition.transform.translate.x, 0.0f, 500.0f);
	ImGui::Checkbox("isClicked", &isPress);
	ImGui::End();
	isPress_ = isPress;

	ImGui::Begin("Panel");
	ImGui::SliderFloat3("Position", &panel_->mainPosition.transform.translate.x, 0.0f, 500.0f);
	ImGui::End();
}
#endif 