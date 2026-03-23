#include "SceneCGHK2.h"

SceneCGHK2::SceneCGHK2(kEngine* system) {
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
	//camera_->Move(Vector3(0.0f, 0.5f, -10.0f));
	usingCamera_ = camera_;
	system_->SetCamera(usingCamera_);

	/// =========== リソースロード ============///
	skydomeModelHandle_ = system_->SetModelObj("./kEngine/EngineAssets/TemplateResource/object/skydome/skydome.obj");
	playerModelHandle_ = system_->SetModelObj("./kEngine/EngineAssets/object/ball/ball.obj");
	//playerModelHandle_ = system_->SetModelObj("./kEngine/EngineAssets/TemplateResource/object/plane/plane.gltf");

	boxTextureHandle_ = system_->LoadTexture("./kEngine/EngineAssets/texture/testBox.png");
	tryTextureHandle_ = system_->LoadTexture("./kEngine/EngineAssets/texture/Tryer.png");
	uvTextureHandle_ = system_->LoadTexture("./kEngine/EngineAssets/TemplateResource/texture/uvChecker.png");
	whiteTextureHandle_ = system_->LoadTexture("./kEngine/EngineAssets/TemplateResource/texture/white5x5.png");

	soundHandle_ = system_->SoundLoadSE("./kEngine/EngineAssets/sound/take.mp3");

	skydome_ = std::make_unique <Object>();
	skydome_->IntObject(system_);
	skydome_->CreateModelData(skydomeModelHandle_);
	//skydome_->objectParts_[0].materialConfig->enableLighting = false;

	player_ = std::make_unique<Player>(system, Vector3(0.0f, 0.0f, 0));
	player_->CreateModelData(playerModelHandle_);
	player_->mainPosition.transform.scale = Vector3(0.5f, 0.5f, 0.5f);
	player_->mainPosition.transform.rotate.y = 3.14f;
	for (auto parts : player_->objectParts_) {
		parts.materialConfig->lightModelType = LightModelType::BlinnPhongReflection;
	}

	//player_->CreateDefaultData();
	//player_->modelHandle_ = playerModelHandle_;
	debugObject_ = std::make_unique<DebugObject>(system_);
	debugObject_->SetFollowObject(&player_->mainPosition);
	//debugObject_->SetShowCenterPoint(true);
	debugObject_->SetShowNumber(true);
	//debugObject_->UpdateShowNumber(92);
	debugObject_->UpdateShowNumber(1234567890);

	ground_ = std::make_unique<Object>();
	ground_->IntObject(system_);
	ground_->CreateDefaultData();
	ground_->modelHandle_ = config::default_Cube_MeshBufferHandle_;
	ground_->objectParts_[0].materialConfig->textureHandle = whiteTextureHandle_;
	ground_->mainPosition.transform.scale = Vector3(50.0f, 0.1f, 50.0f);
	ground_->mainPosition.transform.translate = Vector3(0.0f, -1.0f, 0.0f);

	sprite_ = std::make_unique<SimpleSprite>();
	sprite_->IntObject(system_);
	sprite_->CreateDefaultData();
	sprite_->objectParts_[0].materialConfig->textureHandle = uvTextureHandle_;

	sprite2_ = std::make_unique<SimpleSprite>();
	sprite2_->IntObject(system_);
	sprite2_->CreateDefaultData();
	sprite2_->objectParts_[0].materialConfig->textureHandle = uvTextureHandle_;


}

SceneCGHK2::~SceneCGHK2() {
	system_->DestroyCamera(camera_);
	system_->DestroyCamera(debugCamera_);

	light1_.reset();
	light2_.reset();
	light3_.reset();
	areaLight_.reset();
	areaLight2_.reset();

	ground_.reset();
	debugObject_.reset();
	player_.reset();
	skydome_.reset();
	sprite_.reset();
	sprite2_.reset();
}

void SceneCGHK2::Update() {

	CameraPart();

	/// Skydome更新
	skydome_->Update(usingCamera_);

	/// player更新
	player_->Update(usingCamera_);

	sprite_->Update(usingCamera_);

	debugObject_->SetShowCenterPoint(isShowCenterPoint);
	debugObject_->SetShowNumber(isShowCenterNumber);
	debugObject_->UpdateShowNumber(textNumber);
	debugObject_->Update(usingCamera_);

	ground_->Update(usingCamera_);


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

}


void SceneCGHK2::Draw() {

	/// 実体処理
	//system_->DrawModel(&skydome_->objectParts[0].transformationMatrix, &(skydome_->objectParts[0].materialConfig.get()), skydomeModelHandle_);
	skydome_->Draw();
	player_->Draw();
	debugObject_->Draw();
	ground_->Draw();
	//sprite_->Draw();
	//sprite2_->Draw();
	//system_->Draw3D(skydome_);
	//system_->Draw3D(player_);
	//system_->Draw3D(model_);


#ifdef USE_IMGUI
	/// ImGui処理
	ImGuiPart();
#endif
}

void SceneCGHK2::CameraPart() {
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


void SceneCGHK2::InitNormalLight() {
	light1_->ableLight = true;
	light1_->direction = { -0.5f, -1.0f, -0.3f };
	light1_->color = { 1.0f, 1.0f, 1.0f };
	light1_->intensity = 1.0f;

	light2_->ableLight = true;
	light2_->lightingType = LightingType::PointLight;
	light2_->position = { 3.0f, 1.0f, 0.0f };
	light2_->range = 10.0f;
	light2_->color = { 1.0f, 0.2f, 0.2f };
	light2_->intensity = 2.0f;

	light3_->ableLight = true;
	light3_->lightingType = LightingType::SpotLight;
	light3_->position = { 0.0f, 4.0f, 0.0f };   // 在物體正上方
	light3_->direction = { 0.0f, -1.0f, 0.0f };  // 往下照
	light3_->angle = 0.34906585f;
	light3_->range = 12.0f;
	light3_->color = { 0.3f, 0.3f, 1.0f };   // 藍光
	light3_->intensity = 3.0f;

	areaLight_->ableLight = true;
	areaLight_->lightingType = LightingType::AreaLight;
	areaLight_->position = { 0.0f, 2.0f, 0.0f };
	areaLight_->right = { 1.0f, 0.0f, 0.0f };
	areaLight_->up = { 0.0f,  0.0f, 1.0f };
	areaLight_->width = 1.0f;
	areaLight_->height = 1.0f;
	areaLight_->color = { 1.0f, 0.9f, 0.7f }; // 暖色光
	areaLight_->intensity = 5.0f;
	areaLight_->range = 10.0f;

	areaLight2_->ableLight = false;
}

void SceneCGHK2::InitMultiPointLight() {

	light1_->ableLight = true;
	light1_->lightingType = LightingType::PointLight;
	light1_->position = { -2.0f, 2.0f, 1.5f };
	light1_->range = 12.0f;
	light1_->color = { 1.0f, 0.85f, 0.6f };
	light1_->intensity = 3.0f;

	light2_->ableLight = true;
	light2_->lightingType = LightingType::PointLight;
	light2_->position = { 2.5f, 1.0f, -1.0f };
	light2_->range = 10.0f;
	light2_->color = { 0.6f, 0.7f, 1.0f };
	light2_->intensity = 2.0f;

	light3_->ableLight = true;
	light3_->lightingType = LightingType::PointLight;
	light3_->position = { 0.0f, 1.5f, -3.0f };
	light3_->range = 14.0f;
	light3_->color = { 0.9f, 0.4f, 0.6f };
	light3_->intensity = 2.5f;

	areaLight_->ableLight = false;
	areaLight2_->ableLight = false;
}

void SceneCGHK2::InitMultiSpotLight() {

	light1_->ableLight = true;
	light1_->lightingType = LightingType::SpotLight;
	light1_->position = { -3.0f, 3.0f, 2.0f };
	light1_->direction = Normalize(Vector3{ 1.0f, -1.2f, -0.8f });
	light1_->angle = 0.45f;
	light1_->range = 15.0f;
	light1_->color = { 1.0f, 0.9f, 0.7f };
	light1_->intensity = 4.0f;

	light2_->ableLight = true;
	light2_->lightingType = LightingType::SpotLight;
	light2_->position = { 3.0f, 1.5f, 1.0f };
	light2_->direction = Normalize(Vector3{ -1.0f, -0.5f, -0.3f });
	light2_->angle = 0.55f;
	light2_->range = 12.0f;
	light2_->color = { 0.7f, 0.8f, 1.0f };
	light2_->intensity = 2.5f;

	light3_->ableLight = true;
	light3_->lightingType = LightingType::SpotLight;
	light3_->position = { 0.0f, 2.5f, -3.0f };
	light3_->direction = Normalize(Vector3{ 0.0f, -0.5f, 1.0f });
	light3_->angle = 0.40f;
	light3_->range = 15.0f;
	light3_->color = { 1.0f, 0.5f, 0.8f };
	light3_->intensity = 3.0f;

	areaLight_->ableLight = false;
	areaLight2_->ableLight = false;
}

void SceneCGHK2::InitMultiAreaLight() {

	light1_->ableLight = false;
	light2_->ableLight = false;
	light3_->ableLight = false;

	areaLight_->ableLight = true;
	areaLight_->lightingType = LightingType::AreaLight;

	areaLight_->position = { 0.0f, 3.0f, 0.0f };
	areaLight_->right = { 1.0f, 0.0f, 0.0f };  
	areaLight_->up = { 0.0f, 0.0f, 1.0f };     
	areaLight_->width = 2.5f;
	areaLight_->height = 2.5f;

	areaLight_->color = { 1.0f, 0.9f, 0.75f }; 
	areaLight_->intensity = 6.0f;
	areaLight_->range = 12.0f;

	areaLight2_->ableLight = true;
	areaLight2_->lightingType = LightingType::AreaLight;

	areaLight2_->position = { -3.0f, 1.5f, 1.0f };   
	areaLight2_->right = { 0.0f, 0.0f, 1.0f };    
	areaLight2_->up = { 0.0f, 1.0f, 0.0f };    
	areaLight2_->width = 1.8f;
	areaLight2_->height = 1.8f;

	areaLight2_->color = { 0.7f, 0.8f, 1.0f };   
	areaLight2_->intensity = 4.0f;
	areaLight2_->range = 10.0f;
}

#ifdef USE_IMGUI
void SceneCGHK2::ImGuiPart() {
	ImGui::Begin("DebugCamera");
	ImGui::Checkbox("isUse", &useDebugCamera);
	ImGui::End();

	{
		float shininess = player_->objectParts_[0].materialConfig->shininess;
		float scale = player_->mainPosition.transform.scale.x;
		ImGui::Begin("PlayerPos");
		ImGui::SliderFloat3("Pos", &player_->mainPosition.transform.translate.x, -1.0f, 1.0f);
		ImGui::SliderFloat3("Rotate", &player_->mainPosition.transform.rotate.x, -1.0f, 1.0f);
		ImGui::SliderFloat3("Scale", &player_->mainPosition.transform.scale.x, -1.0f, 1.0f);
		ImGui::SliderFloat("ScaleOnce", &scale, -1.0f, 1.0f);
		ImGui::SliderFloat("shininess", &shininess, 0.0f, 256.0f);

		ImGui::ColorEdit4("Color", &player_->objectParts_[0].materialConfig->textureColor.x);
		ImGui::ColorEdit4("Color2", &player_->objectParts_[0].materialConfig->textureColor2.x);
		ImGui::ColorEdit4("Color3", &player_->objectParts_[0].materialConfig->textureColor3.x);

		ImGui::SliderFloat("intensity", &player_->objectParts_[0].materialConfig->intensity, 0.0f, 5.0f);
		ImGui::SliderFloat("heightScale", &player_->objectParts_[0].materialConfig->heightScale, 0.0f, 5.0f);

		ImGui::Checkbox("isBlinn_Phong", (bool*)&isBlinn_Phong);

		ImGui::SliderInt("textNumber", &textNumber, 0, 1000);
		ImGui::Checkbox("isShowCenterPoint", &isShowCenterPoint);
		ImGui::Checkbox("isShowCenterNumber", &isShowCenterNumber);

		ImGui::End();

		if(isBlinn_Phong) {
			for (auto& parts : player_->objectParts_) {
				parts.materialConfig->lightModelType = LightModelType::BlinnPhongReflection;
			}
		}
		else {
			for (auto& parts : player_->objectParts_) {
				parts.materialConfig->lightModelType = LightModelType::FlameNeonGlow;
			}
		}

		//player_->mainPosition.transform.scale.x = scale;
		//player_->mainPosition.transform.scale.y = scale;
		//player_->mainPosition.transform.scale.z = scale;

		for (auto& parts : player_->objectParts_) {
			parts.materialConfig->shininess = shininess;
		}
	}

	ImGui::Begin("Lightings Setting");
	if (isNormalLight) {
		ImGui::Text("Directional Light");
		ImGui::Checkbox("EnableLight1", &light1_->ableLight);
		ImGui::SliderFloat3("Direction1", &light1_->direction.x, -1.0f, 1.0f);
		ImGui::ColorEdit3("Color1", &light1_->color.x);
		ImGui::SliderFloat("Intensity1", &light1_->intensity, 0.0f, 5.0f);

		ImGui::NewLine();
		ImGui::Text("Point Light");
		ImGui::Checkbox("EnableLight2", &light2_->ableLight);
		ImGui::SliderFloat3("Position2", &light2_->position.x, -10.0f, 10.0f);
		ImGui::SliderFloat("Range2", &light2_->range, 1.0f, 50.0f);
		ImGui::ColorEdit3("Color2", &light2_->color.x);
		ImGui::SliderFloat("Intensity2", &light2_->intensity, 0.0f, 10.0f);

		ImGui::NewLine();
		ImGui::Text("Spot Light");
		ImGui::Checkbox("EnableLight3", &light3_->ableLight);
		ImGui::SliderFloat3("Position3", &light3_->position.x, -10.0f, 10.0f);
		ImGui::SliderFloat3("Direction3", &light3_->direction.x, -1.0f, 1.0f);
		ImGui::SliderFloat("Angle3", &light3_->angle, 0.0f, 1.57f);
		ImGui::SliderFloat("Range3", &light3_->range, 1.0f, 50.0f);
		ImGui::ColorEdit3("Color3", &light3_->color.x);
		ImGui::SliderFloat("Intensity3", &light3_->intensity, 0.0f, 10.0f);

		ImGui::NewLine();
		ImGui::Text("Area Light");
		ImGui::Checkbox("Enable AreaLight", &areaLight_->ableLight);
		ImGui::SliderFloat3("Position", &areaLight_->position.x, -10.0f, 10.0f);
		ImGui::SliderFloat("Width", &areaLight_->width, 0.1f, 10.0f);
		ImGui::SliderFloat("Height", &areaLight_->height, 0.1f, 10.0f);
		ImGui::SliderFloat3("Right", &areaLight_->right.x, -1.0f, 1.0f);
		ImGui::SliderFloat3("Up", &areaLight_->up.x, -1.0f, 1.0f);
		ImGui::SliderFloat("Range", &areaLight_->range, 1.0f, 50.0f);
		ImGui::ColorEdit3("Color", &areaLight_->color.x);
		ImGui::SliderFloat("Intensity", &areaLight_->intensity, 0.0f, 10.0f);


	} else if (isMultiPointLight) {

		ImGui::Text("Spot Light1");
		ImGui::NewLine();
		ImGui::Text("Point Light1");
		ImGui::Checkbox("EnableLight1", &light1_->ableLight);
		ImGui::SliderFloat3("Position1", &light1_->position.x, -10.0f, 10.0f);
		ImGui::SliderFloat("Range1", &light1_->range, 1.0f, 50.0f);
		ImGui::ColorEdit3("Color1", &light1_->color.x);
		ImGui::SliderFloat("Intensity1", &light1_->intensity, 0.0f, 10.0f);

		ImGui::NewLine();
		ImGui::NewLine();
		ImGui::Text("Point Light2");
		ImGui::Checkbox("EnableLight2", &light2_->ableLight);
		ImGui::SliderFloat3("Position2", &light2_->position.x, -10.0f, 10.0f);
		ImGui::SliderFloat("Range2", &light2_->range, 1.0f, 50.0f);
		ImGui::ColorEdit3("Color2", &light2_->color.x);
		ImGui::SliderFloat("Intensity2", &light2_->intensity, 0.0f, 10.0f);

		ImGui::NewLine();
		ImGui::NewLine();
		ImGui::Text("Point Light3");
		ImGui::Checkbox("EnableLight3", &light3_->ableLight);
		ImGui::SliderFloat3("Position3", &light3_->position.x, -10.0f, 10.0f);
		ImGui::SliderFloat("Range3", &light3_->range, 1.0f, 50.0f);
		ImGui::ColorEdit3("Color3", &light3_->color.x);
		ImGui::SliderFloat("Intensity3", &light3_->intensity, 0.0f, 10.0f);

	} else if (isMultiSpotLight) {

		ImGui::Text("Spot Light1");
		ImGui::Checkbox("EnableLight1", &light1_->ableLight);
		ImGui::SliderFloat3("Position1", &light1_->position.x, -10.0f, 10.0f);
		ImGui::SliderFloat3("Direction1", &light1_->direction.x, -1.0f, 1.0f);
		ImGui::SliderFloat("Angle1", &light1_->angle, 0.0f, 1.57f);
		ImGui::SliderFloat("Range1", &light1_->range, 1.0f, 50.0f);
		ImGui::ColorEdit3("Color1", &light1_->color.x);
		ImGui::SliderFloat("Intensity1", &light1_->intensity, 0.0f, 10.0f);

		ImGui::NewLine();
		ImGui::Text("Spot Light2");
		ImGui::Checkbox("EnableLight2", &light2_->ableLight);
		ImGui::SliderFloat3("Position2", &light2_->position.x, -10.0f, 10.0f);
		ImGui::SliderFloat3("Direction2", &light2_->direction.x, -1.0f, 1.0f);
		ImGui::SliderFloat("Angle2", &light2_->angle, 0.0f, 1.57f);
		ImGui::SliderFloat("Range2", &light2_->range, 1.0f, 50.0f);
		ImGui::ColorEdit3("Color2", &light2_->color.x);
		ImGui::SliderFloat("Intensity2", &light2_->intensity, 0.0f, 10.0f);

		ImGui::NewLine();
		ImGui::Text("Spot Light");
		ImGui::Checkbox("EnableLight3", &light3_->ableLight);
		ImGui::SliderFloat3("Position3", &light3_->position.x, -10.0f, 10.0f);
		ImGui::SliderFloat3("Direction3", &light3_->direction.x, -1.0f, 1.0f);
		ImGui::SliderFloat("Angle3", &light3_->angle, 0.0f, 1.57f);
		ImGui::SliderFloat("Range3", &light3_->range, 1.0f, 50.0f);
		ImGui::ColorEdit3("Color3", &light3_->color.x);
		ImGui::SliderFloat("Intensity3", &light3_->intensity, 0.0f, 10.0f);
	} else if (isMultiAreaLight) {
		ImGui::NewLine();
		ImGui::Text("Area Light1");
		ImGui::Checkbox("Enable AreaLight1", &areaLight_->ableLight);
		ImGui::SliderFloat3("Position1", &areaLight_->position.x, -10.0f, 10.0f);
		ImGui::SliderFloat("Width1", &areaLight_->width, 0.1f, 10.0f);
		ImGui::SliderFloat("Height1", &areaLight_->height, 0.1f, 10.0f);
		ImGui::SliderFloat3("Right1", &areaLight_->right.x, -1.0f, 1.0f);
		ImGui::SliderFloat3("Up1", &areaLight_->up.x, -1.0f, 1.0f);
		ImGui::SliderFloat("Range1", &areaLight_->range, 1.0f, 50.0f);
		ImGui::ColorEdit3("Color1", &areaLight_->color.x);
		ImGui::SliderFloat("Intensity1", &areaLight_->intensity, 0.0f, 10.0f);

		ImGui::NewLine();
		ImGui::Text("Area Light2");
		ImGui::Checkbox("Enable AreaLight2", &areaLight2_->ableLight);
		ImGui::SliderFloat3("Position2", &areaLight2_->position.x, -10.0f, 10.0f);
		ImGui::SliderFloat("Width2", &areaLight2_->width, 0.1f, 10.0f);
		ImGui::SliderFloat("Height2", &areaLight2_->height, 0.1f, 10.0f);
		ImGui::SliderFloat3("Right2", &areaLight2_->right.x, -1.0f, 1.0f);
		ImGui::SliderFloat3("Up2", &areaLight2_->up.x, -1.0f, 1.0f);
		ImGui::SliderFloat("Range2", &areaLight2_->range, 1.0f, 50.0f);
		ImGui::ColorEdit3("Color2", &areaLight2_->color.x);
		ImGui::SliderFloat("Intensity2", &areaLight2_->intensity, 0.0f, 10.0f);
	}

	ImGui::End();

	SelectLightMode();
}

void SceneCGHK2::SelectLightMode() {
	bool N = isNormalLight;
	bool P = isMultiPointLight;
	bool S = isMultiSpotLight;
	bool A = isMultiAreaLight;

	ImGui::Begin("Lightings Scene");
	ImGui::Checkbox("Normal Light", &N);
	ImGui::Checkbox("Multi Point Light", &P);
	ImGui::Checkbox("Multi Spot Light", &S);
	ImGui::Checkbox("Multi Area Light", &A);
	ImGui::End();

	isNormalLight = false;
	isMultiPointLight = false;
	isMultiSpotLight = false;
	isMultiAreaLight = false;

	if (N) {
		isNormalLight = true;
		if (lightMode != 0) {
			InitNormalLight();
			lightMode = 0;
			return;
		}
	}
	if (P) {
		isMultiPointLight = true;
		if (lightMode != 1) {
			InitMultiPointLight();
			lightMode = 1;
			return;
		}
	}
	if (S) {
		isMultiSpotLight = true;
		if (lightMode != 2) {
			InitMultiSpotLight();
			lightMode = 2;
			return;
		}
	}
	if (A) {
		isMultiAreaLight = true;
		if (lightMode != 3) {
			InitMultiAreaLight();
			lightMode = 3;
			return;
		}
	}
}

#endif