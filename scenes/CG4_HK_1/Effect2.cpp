#include "Effect2.h"

Effect2::Effect2(kEngine* system) {
	/// =========== システム初期化 ============///
	system_ = system;
	debugCamera_ = system_->CreateDebugCamera();
	camera_ = system_->CreateCamera();
	camera_->Move(Vector3(0.0f, 0.5f, -10.0f));

	/// =========== リソースロード ============///
	skydomeModelHandle_ = system_->SetModelObj("resources/TemplateResource/object/skydome/skydome.obj");
	planeModelHandle_ = system_->SetModelObj("resources/TemplateResource/object/plane/plane.obj");

	boxTextureHandle_ = system_->LoadTexture("resources/texture/testBox.png");

	skydome_ = new Object;
	skydome_->IntObject(system_);
	skydome_->CreateModelData(skydomeModelHandle_);
	skydome_->objectParts_[0].materialConfig->enableLighting = false;

	ball_ = new P_Ball(system_);

	plane_ = new Object;
	plane_->IntObject(system_);
	plane_->CreateModelData(planeModelHandle_);
}

Effect2::~Effect2() {
	system_->DestroyCamera(camera_);
	system_->DestroyCamera(debugCamera_);

	delete skydome_;
	delete ball_;
	delete plane_;
	//delete fire_;
}

void Effect2::Update() {
	/// カメラ処理
	CameraPart();

	/// Skydome更新
	skydome_->Update(usingCamera_);

	plane_->Update(usingCamera_);


	if (isWind_) {
		std::vector<Object*> list = ball_->GetObjectList();
		for (auto& obj : list) {
			obj->mainPosition.transform.translate.x -= 3 * system_->GetDeltaTime();
		}
	}
	ball_->Update(usingCamera_);



	if (system_->GetTriggerOn(DIK_SPACE)) {
		std::uniform_real_distribution<float> distribution(10.0f, 20.0f);
		randomNumber = distribution(randomEngine);
	}

	if (system_->GetTriggerOn(DIK_0)) {
		if (useDebugCamera)useDebugCamera = false;
		else useDebugCamera = true;
	}
}


void Effect2::Draw() {

	/// 実体処理
	skydome_->Draw();
	//plane_->Draw();

	ball_->Draw();

#ifdef USE_IMGUI
	/// imgui処理
	ImguiPart();
#endif
}

void Effect2::CameraPart() {
	if (useDebugCamera) {
		usingCamera_ = debugCamera_;
		debugCamera_->MouseControlUpdate();
	} else {
		usingCamera_ = camera_;
	}
	system_->SetCamera(usingCamera_);
}

#ifdef USE_IMGUI
void Effect2::ImguiPart() {
	ImGui::Begin("DebugCamera");
	ImGui::Checkbox("isUse", &useDebugCamera);
	ImGui::Checkbox("isWind", &isWind_);
	ImGui::End();

	//int num = fire_->GetParticleNum();
	//ImGui::Begin("RandomTest");
	//ImGui::Text("ParticleNumber = %d", num);
	//ImGui::End();

}
#endif