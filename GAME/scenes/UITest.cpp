#include "UITest.h"
#include "DebugDraw.h"
#include "EngineAssets/Particle/HitSpark.h"

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

	/// PostEffectを設定
	std::vector<PostProcessType> postProcessList = {
		PostProcessType::Vignette,
	};

	system_->SetPostProcessChain(postProcessList);

	/// =========== リソースロード ============///
	skydomeModelHandle_ = system_->SetModelObj("./kEngine/EngineAssets/TemplateResource/object/skydome/skydome.obj");
	smallStageHandel_ = system_->SetModelObj("./GAME/resources/Object/smallStage/smallStage.obj");
	BGObjectHandle_ = system_->SetModelObj("./GAME/resources/Object/titleBG/titleBG.obj");

	whiteTextureHandle_ = system_->LoadTexture("./kEngine/EngineAssets/TemplateResource/texture/white5x5.png");
	clicleTextureHandle_ = system_->LoadTexture("./kEngine/EngineAssets/TemplateResource/texture/circle_withAlpha.png");
	effectTextureHandle_ = system_->LoadTexture("./GAME/resources/texture/gradationLine.png");
	uvCheckerTextureHandle_ = system_->LoadTexture("./kEngine/EngineAssets/TemplateResource/texture/uvChecker.png");


	//BGObject.obj
	//objectHandle_ = system_->SetModelObj("./GAME/Object/Goal/Goal.obj");
	//objectHandle_ = system_->SetModelObj("./kEngine/EngineAssets/Object/charater/charater.obj");
	objectHandle_ = system_->CreateEngineModel(cylinderBuildMaterial_);

	//ddsTest = system_->LoadTexture("./GAME/resources/texture/skyCube/rostock_laage_airport_4k.dds");
	ddsTest = system_->LoadTexture("./GAME/resources/texture/skyCube/output_skybox.dds");

	system_->GetDrawEngine()->SetEnviromentReflectionTexture(ddsTest);

	skydome_ = std::make_unique<Object>();
	skydome_->IntObject(system_);
	skydome_->CreateModelData(skydomeModelHandle_);

	ground_ = std::make_unique<Object>();
	ground_->IntObject(system_);
	//ground_->CreateModelData(objectHandle_);
	ground_->mainPosition.transform.translate = Vector3(1.0f, 1.0f, 1.0f);
	ground_->CreateDefaultData();
	ground_->modelHandle_ = config::default_Cube_MeshBufferHandle_;
	ground_->objectParts_[0].materialConfig->textureHandle = whiteTextureHandle_;
	ground_->mainPosition.transform.scale = Vector3(50.0f, 0.1f, 50.0f);
	ground_->mainPosition.transform.translate = Vector3(0.0f, -1.0f, 0.0f);

	box_ = std::make_unique<Object>();
	box_->IntObject(system_);
	//box_->CreateModelData(objectHandle_);
	box_->CreateDefaultData();
	//box_->modelHandle_ = config::default_Cube_MeshBufferHandle_;
	//box_->modelHandle_ = config::default_Sphere_MeshBufferHandle_;
	//box_->modelHandle_ = config::default_Cylinder_MeshBufferHandle_;
	box_->modelHandle_ = objectHandle_;
	//box_->objectParts_[0].materialConfig->textureHandle = whiteTextureHandle_;
	box_->objectParts_[0].materialConfig->textureHandle = uvCheckerTextureHandle_;
	//box_->objectParts_[0].materialConfig->textureHandle = clicleTextureHandle_;
	//box_->objectParts_[0].materialConfig->textureHandle = effectTextureHandle_;
	//box_->isBillboard_ = true;
	//box_->objectParts_[0].materialConfig->isReflective = true;
	box_->objectParts_[0].materialConfig->enableLighting = false;
	box_->objectParts_[0].materialConfig->lightModelType = LightModelType::Lambert;
	box_->objectParts_[0].materialConfig->rasterizerMode = RasterizerMode::CullNone;
	box_->objectParts_[0].materialConfig->textureColor = { 1.0f,1.0f,1.0f,0.999f };
	//box_->mainPosition.transform.scale = Vector3(0.5f, 0.5f, 0.5f);
	//box_->mainPosition.transform.translate = Vector3(0.0f, 0.0f, 0.0f);

	skybox_ = std::make_unique<Object>();
	skybox_->IntObject(system_);
	skybox_->CreateDefaultData();
	skybox_->modelHandle_ = config::default_SkyCube_MeshBufferHandle_;
	skybox_->objectParts_[0].materialConfig->textureHandle = ddsTest;
	skybox_->objectParts_[0].materialConfig->MakePSOEnvironment();
	skybox_->mainPosition.transform.scale = Vector3(200.0f, 200.0f, 200.0f);

	detailButton_ = std::make_unique<DetailButton>(system_);
	detailButton_->SetButton({ 100.0f,100.0f }, 200.0f, 80.0f);

	button_ = std::make_unique<Button>(system_);
	button_->Init(
		{ 100.0f,350.0f },
		200.0f, 80.0f,
		whiteTextureHandle_,
		whiteTextureHandle_,
		whiteTextureHandle_,
		whiteTextureHandle_,
		soundHandle_, soundHandle_);
	button_->ChangeTextureCColor({ 1.0f,1.0f,1.0f,1.0f });
	button_->ChangeTextureSColor({ 1.0f,0.8f,0.8f,1.0f });
	button_->ChangeTexturePColor({ 0.5f,0.5f,1.0f,1.0f });
	button_->ChangeTextureNColor({ 0.5f,0.5f,0.5f,1.0f });

	defaultMenu_ = std::make_unique<DefaultMenu>(system_);

	panel_ = std::make_unique<Panel>(system_);
	panel_->SetPanel({ 720.0f,300.0f }, 500.0f, 500.0f);

	BGObject_ = std::make_unique<Object>();
	BGObject_->IntObject(system_);
	BGObject_->CreateModelData(BGObjectHandle_);
	BGObject_->objectParts_[0].materialConfig->enableLighting = false;
	Vector4 color{ 255.0f,224.0f,136.0f,255.0f };
	color.ColorBy1();
	BGObject_->objectParts_[0].materialConfig->textureColor = color;
	BGObject_->objectParts_[1].materialConfig->enableLighting = false;
	BGObject_->objectParts_[1].materialConfig->textureColor = {1,1,1,1};
	BGObject_->mainPosition.transform.scale = { 100.0f,100.0f,100.0f };

	/// =========== パーティクル作る ============///

	HitSpark hitSpark;
	hitSpark.startPosition = { 0.0f, 0.0f, 0.0f };
	hitSpark.objectList[0].objectParts_[0].materialConfig->textureHandle = clicleTextureHandle_;
	particleHandle_ = system_->GetEffectManager()->GetParticleManager()->CreateEmitter(hitSpark, 0);

	HitImpact hitImpact;
	hitImpact.startPosition = { 0.0f, 0.0f, 0.0f };
	hitImpact.objectList[0].objectParts_[0].materialConfig->textureHandle = effectTextureHandle_;
	particleHandle2_ = system_->GetEffectManager()->GetParticleManager()->CreateEmitter(hitImpact, 1);

	HitSpackImpactLink linkData;
	linkData.sourceId = particleHandle_;
	linkData.targetId = particleHandle2_;
	system_->GetEffectManager()->GetParticleManager()->LinkEmitterToEmitter(linkData);

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

	system_->ChangeRenderCommand(renderCommand_);

	CameraPart();

	/// Skydome更新
	skydome_->Update();

	ground_->Update();

	detailButton_->Update();

	button_->Update();

	panel_->Update();

	defaultMenu_->Updata();

	MouseLogic();

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

	//if (detailButton_->GetIsPress()) {
	//	for(auto& object : box_->objectParts_){ object.materialConfig->reflectiveStrength = 1.0f; }
	//} else {
	//	for(auto& object : box_->objectParts_){ object.materialConfig->reflectiveStrength = 0.0f; }
	//}

	if (detailButton_->GetIsClicked()) {
		system_->GetEffectManager()->GetParticleManager()->ShootEmitter(particleHandle_, 1);
	}

	//if (button_->GetIsClick()) {
	//	system_->ChangeEngineModel(sphereBuildMaterial_, objectHandle_);
	//}
	if (button_->GetIsClick()) {
		system_->ChangeEngineModel(cylinderBuildMaterial_, objectHandle_);
	}


	//if (system_->GetMouseTriggerOn(0)) {
	//	box_->objectParts_[0].materialConfig->reflectiveStrength += 0.05f;
	//}
	//
	//if (system_->GetMouseTriggerOn(1)) {
	//	box_->objectParts_[0].materialConfig->reflectiveStrength -= 0.05f;
	//}

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

	// BGObject_->objectParts_[1].transform.rotate.z -= 0.001f;

}

void UITest::MouseLogic() {

	auto cam = usingCamera_.lock();
	if (!cam) return;

	Ray mouseRay = usingCamera_.lock()->ScreenPointToRay(system_->GetMousePosVector2());

	//Sphere target;
	//target.center = box_->mainPosition.transform.translate;
	//target.radius = 0.5f;
	//isHit = crashDecision(target, mouseRay);
	//
	//AABB aabb;
	//aabb.min = box_->mainPosition.transform.translate - Vector3(0.5f, 0.5f, 0.5f);
	//aabb.max = box_->mainPosition.transform.translate + Vector3(0.5f, 0.5f, 0.5f);
	//bool aabbHit = crashDecision(aabb, mouseRay);
	//
	//if (isHit) {
	//	DebugDraw::AddSphere(target, { 1.0f,0.0f,0.0f,1.0f }, cam.get());
	//} else {	
	//	DebugDraw::AddSphere(target, { 1.0f,1.0f,0.0f,1.0f }, cam.get());
	//}
	//
	//if (aabbHit) {
	//	DebugDraw::AddAABB(aabb, { 1.0f,0.0f,0.0f,1.0f });
	//} else {	
	//	DebugDraw::AddAABB(aabb, { 0.0f,1.0f,0.0f,1.0f });
	//}

}

void UITest::Draw() {

	/// 実体処理
	//skydome_->Draw();
	//skybox_->Draw();
	ground_->Draw();
	//box_->Draw();
	detailButton_->Render();
	button_->Render();
	//panel_->Render();
	defaultMenu_->Draw();
	BGObject_->Draw();

#ifdef USE_IMGUI
	/// ImGui処理
	ImGuiPart();
#endif
}

void UITest::CameraPart() {
	if (useDebugCamera) {
		usingCamera_ = debugCamera_;
		if (auto sp = debugCamera_.lock()) {
			sp->MouseControlUpdate();
		}
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

	ImGui::Begin("BGObject");
	ImGui::ColorEdit4("object0.color", &BGObject_->objectParts_[0].materialConfig->textureColor.x);
	ImGui::SliderFloat("object0.rotate", &BGObject_->objectParts_[0].transform.rotate.z, -10.0f, 10.0f);
	ImGui::SliderFloat("object1.rotate", &BGObject_->objectParts_[1].transform.rotate.z, -10.0f, 10.0f);
	float scale = BGObject_->mainPosition.transform.scale.x;
	ImGui::SliderFloat("object.scale", &scale, 0.1f, 500.0f);
	BGObject_->mainPosition.transform.scale = { scale, scale, scale };
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
	{

		ImGui::Begin("SphereBuildMaterial");
		ImGui::Text("SphereBuildMaterial");
		ImGui::SliderInt("Latitude", &sphereBuildMaterial_.LatitudeSegments, 0, 50);
		ImGui::SliderInt("Longitude", &sphereBuildMaterial_.LongitudeSegments, 0, 50);

		ImGui::Text("CylinderBuildMaterial");
		ImGui::SliderInt("Division", &cylinderBuildMaterial_.Division, 0, 50);
		ImGui::SliderFloat("Height", &cylinderBuildMaterial_.Height, 0.0f, 8.0f);
		ImGui::SliderFloat("TopRadius", &cylinderBuildMaterial_.TopRadius, 0.0f, 2.0f);
		ImGui::SliderFloat("BottomRadius", &cylinderBuildMaterial_.BottomRadius, 0.0f, 2.0f);
		ImGui::Checkbox("ReverseY", &cylinderBuildMaterial_.isReverseY);
		ImGui::End();
	}
	{

		ImGui::Begin("RenderCommand");
		ImGui::Text("ColorGuard");
		ImGui::SliderFloat3("Guard Color", &renderCommand_.guardColor[0], 0, 1);
		ImGui::SliderFloat("Guard Amount", &renderCommand_.guardAmount, 0, 1);
		ImGui::Text("Vignette");
		ImGui::SliderFloat2("Vignette Center", &renderCommand_.vignetteCenter.x, 0, 1);
		ImGui::SliderFloat("Vignette Radius", &renderCommand_.vignetteRadius, 0, 1);
		ImGui::SliderFloat("Vignette Softness", &renderCommand_.vignetteSoftness, 0, 1);
		ImGui::SliderFloat("Vignette Intensity", &renderCommand_.vignetteIntensity, 0, 100);
		ImGui::ColorEdit4("Vignette Color", &renderCommand_.vignetteColor.x);
		ImGui::End();
	}

	{
		ImGui::Begin("Box");
		// BlendModeType 対応する文字列の配列
		static const char* blendModeNames[] = {
		"Opaque",
		"Normal",
		"Add",
		"Subtract",
		"Multiply",
		"Screen"
		};

		// 今の BlendModeTypeをとる
		int currentBlend = static_cast<int>(box_->objectParts_[0].materialConfig->blendModeType);

		// 選択肢
		if (ImGui::Combo("Blend Mode", &currentBlend, blendModeNames, IM_ARRAYSIZE(blendModeNames))) {
			box_->objectParts_[0].materialConfig->blendModeType =
				static_cast<BlendModeType>(currentBlend);
		}
		static const char* lightModeNames[] = {
		"Sprite2D",
		"Lambert",
		"HalfLambert",
		"PhongReflection",
		"BlinnPhongReflection",
		"FlameNeonGlow"
		};

		// 今の LightModeTypeをとる
		int currentLight = static_cast<int>(box_->objectParts_[0].materialConfig->lightModelType);

		// 選択肢
		if (ImGui::Combo("Light Mode", &currentLight, lightModeNames, IM_ARRAYSIZE(lightModeNames))) {
			box_->objectParts_[0].materialConfig->lightModelType =
				static_cast<LightModelType>(currentLight);
		}

		ImGui::SliderFloat3("Rotation", &box_->mainPosition.transform.rotate.x, 0.0f, 10.0f);
		ImGui::SliderFloat3("Position", &box_->mainPosition.transform.translate.x, 0.0f, 10.0f);

		ImGui::SliderFloat3("UVTrans", &box_->objectParts_[0].materialConfig->uvTranslate.x, -20.0f, 20.0f);
		ImGui::End();
	}

}
#endif 