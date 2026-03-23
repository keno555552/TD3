#include "Effect1.h"

Effect1::Effect1(kEngine* system) {
	/// =========== システム初期化 ============///
	system_ = system;
	debugCamera_ = system_->CreateDebugCamera();
	camera_ = system_->CreateCamera();
	camera_->Move(Vector3(-2.736f, -4.474f, -27.334f));
	camera_->Rotate(Vector3(-0.324f, 0.228f, 0.0f));

	/// =========== リソースロード ============///
	skydomeModelHandle_ = system_->SetModelObj("resources/TemplateResource/object/skydome/skydome.obj");
	sphereModelHandle_ = system_->SetModelObj("resources/TemplateResource/object/plane/plane.obj");

	boxTextureHandle_ = system_->LoadTexture("resources/texture/testBox.png");

	skydome_ = std::make_unique<Object>();
	skydome_->IntObject(system_);
	skydome_->CreateModelData(skydomeModelHandle_);
	skydome_->objectParts_[0].materialConfig->enableLighting = false;

	int i = 0;
	for (auto& ptr : plane_) {
		ptr = std::make_unique<Object>();
		ptr->IntObject(system_);
		ptr->CreateModelData(sphereModelHandle_);
		ptr->mainPosition.transform.translate = Vector3(0.2f * i, 0.2f * i, 0.2f * i);
		i++;
	}
}

Effect1::~Effect1() {
	system_->DestroyCamera(camera_);
	system_->DestroyCamera(debugCamera_);

	skydome_.reset();
	for (auto& ptr : plane_) {
		ptr.reset();
	}
}

void Effect1::Update() {
	/// カメラ処理
	CameraPart();

	/// Skydome更新
	skydome_->Update(usingCamera_);

	for (auto& ptr : plane_) {
		ptr->Update(usingCamera_);
	}


	if (system_->GetTriggerOn(DIK_0)) {
		if (useDebugCamera)useDebugCamera = false;
		else useDebugCamera = true;
	}
}


void Effect1::Draw() {

	/// 実体処理
	//system_->DrawModel(&skydome_->objectParts[0].transformationMatrix, &(skydome_->objectParts[0].materialConfig.get()), skydomeModelHandle_);
	skydome_->Draw();
	for (auto& ptr : plane_) {
		ptr->Draw();
	}

#ifdef USE_IMGUI
	/// imgui処理
	ImguiPart();
#endif
}

void Effect1::CameraPart() {
	if (useDebugCamera) {
		usingCamera_ = debugCamera_;
	} else {
		usingCamera_ = camera_;
	}
	usingCamera_->Update();
}

#ifdef USE_IMGUI
void Effect1::ImguiPart() {

	Vector3 CameraPosition = usingCamera_->GetTransform().translate;
	Vector3 CameraRotation = usingCamera_->GetTransform().rotate;
	
	ImGui::Begin("DebugCamera");
	ImGui::Checkbox("isUse", &useDebugCamera);
	ImGui::SliderFloat3("CameraPos", &CameraPosition.x , -5.0f, 5.0f);
	ImGui::SliderFloat3("CameraRotate", &CameraRotation.x , -1.0f, 1.0f);
	ImGui::End();

	usingCamera_->SetCamera(Transform{ Vector3(1.0f,1.0f,1.0f),CameraRotation,CameraPosition });

	{
		ImGui::Begin("SpherePos");
		int i = 0;
		for (auto& ptr : plane_) {
			std::string index = std::to_string(i);
			std::string name = "Sphere";
			std::string name1 = "Color";
			std::string name2 = "Pos";
			std::string name3 = "Rotate";
			ImGui::Text((name + index).c_str());
			ImGui::SliderFloat4((name1 + index).c_str(), &ptr->objectParts_[0].materialConfig->textureColor.x, 0.0f, 1.0f);
			ImGui::SliderFloat3((name2 + index).c_str(), &ptr->objectParts_[0].transform.translate.x, 0.0f, 1.0f);
			ImGui::SliderFloat3((name3 + index).c_str(), &ptr->objectParts_[0].transform.rotate.x, -1.0f, 1.0f);
			i++;
		}
		ImGui::End();
	}
}
#endif