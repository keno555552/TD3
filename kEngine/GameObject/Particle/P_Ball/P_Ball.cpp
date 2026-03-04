#include "P_Ball.h"

P_Ball::P_Ball(kEngine* system) :Particle(system) {

	createTimer.Init0(defaultParticleInterval_, system_->GetTimeManager());

	defaultModelHandle = system_->SetModelObj("resources/TemplateResource/object/plane/plane.obj");
	defaultTextureHandle = system_->LoadTexture("resources/TemplateResource/texture/circle.png");
	commonMaterialConfig->useModelTexture = false;
	commonMaterialConfig->textureHandle = defaultTextureHandle;
	commonMaterialConfig->textureColor.w = 0.5f;

}

P_Ball::~P_Ball() {
}

void P_Ball::Update(Camera* camera) {
	/// パーティクル計算

	CreateBall();
	UpdateBall();
	DeleteBall();

	/// wvp更新
	Particle::Update(camera);
}

void P_Ball::Draw() {

	Particle::Draw();

#ifdef USE_IMGUI
	ImguiPart();
#endif
}

void P_Ball::SetTextureHandle(int handle) {
	commonMaterialConfig->textureHandle = handle;

	for (auto& particle : particleObjectList_) {
		particle->part->objectParts_[0].materialConfig = commonMaterialConfig;
	}
}

void P_Ball::CreateBall() {
	/// 倒數時鐘更新
	createTimer.foreverUp();

	/// 作成判定
	if (createTimer.parameter_ == 0) {

		for (int i = 0; i < (int)createNumber_; i++) {
			// 最大数チェック
			if ((int)particleObjectList_.size() >= maxParticleNum) return;
			Particle::AddObject();

			auto& newPart = particleObjectList_.back();
			newPart->part->CreateModelData(defaultModelHandle);
			*newPart->part->objectParts_[0].materialConfig = *commonMaterialConfig;
			newPart->part->mainPosition.transform = CreateDefaultTransform();
			newPart->part->isBillboard_ = true;

			/// 生成位置セット
			newPart->part->mainPosition.transform.translate = anchorPart_.transform.translate;

			/// 向きセット
			float rx = randomMaker_->randomFloat(-1.0f, 1.0f);
			float ry = randomMaker_->randomFloat(-1.0f, 1.0f);
			float rz = randomMaker_->randomFloat(-1.0f, 1.0f);
			newPart->direction.translate = Vector3(rx, ry, rz);

			/// スビートセット
			newPart->velocity = newPart->direction.translate * speed_;

			/// lifeTimeセット
			newPart->lifeTimeTimer.Init0(lifeTime_, system_->GetTimeManager());

			{
				/// 色セット
				float R = randomMaker_->randomFloat(0, 1.0f);
				float G = randomMaker_->randomFloat(0, 1.0f);
				float B = randomMaker_->randomFloat(0, 1.0f);
				newPart->part->objectParts_[0].materialConfig->textureColor = Vector4(R, G, B, 0.3f);
			}
		}


	}
}

void P_Ball::UpdateBall() {

	for (auto object : particleObjectList_) {
		float T = object->lifeTimeTimer.parameter_ / object->lifeTimeTimer.maxTime_;
		object->part->objectParts_[0].materialConfig->textureColor.w = 1.0f - T;
	}

}

void P_Ball::DeleteBall() {
	for (auto it : particleObjectList_) {
		ParticleData* fd = it;
		bool dead = false;
		if (fd->lifeTimeTimer.parameter_ >= fd->lifeTimeTimer.maxTime_)fd->isAlive = false; continue;
		if (fd->part->mainPosition.transform.translate.x > anchorPart_.transform.translate.x + effectArea_.x / 2.0f)fd->isAlive = false; continue;
		if (fd->part->mainPosition.transform.translate.x < anchorPart_.transform.translate.x - effectArea_.x / 2.0f)fd->isAlive = false; continue;
		if (fd->part->mainPosition.transform.translate.y > anchorPart_.transform.translate.y + effectArea_.y / 2.0f)fd->isAlive = false; continue;
		if (fd->part->mainPosition.transform.translate.y < anchorPart_.transform.translate.y - effectArea_.y / 2.0f)fd->isAlive = false; continue;
		if (fd->part->mainPosition.transform.translate.z > anchorPart_.transform.translate.z + effectArea_.z / 2.0f)fd->isAlive = false; continue;
		if (fd->part->mainPosition.transform.translate.z < anchorPart_.transform.translate.z - effectArea_.z / 2.0f)fd->isAlive = false; continue;
	}
}

#ifdef USE_IMGUI
void P_Ball::ImguiPart() {

	//ImGui::Begin("FireMain");
	//ImGui::SliderFloat3("rootPos", &anchorPart_.transform.translate.x, -20.0f, 20.0f);
	//ImGui::SliderFloat("maxLifeTime", &maxLifeTime_, 0.0f, 10.0f);
	//ImGui::SliderFloat("minLifeTime", &minLifeTime_, 0.0f, 10.0f);
	//ImGui::SliderFloat2("unitMaxSize", &unitMaxSize_.x, 0.0f, 10.0f);
	//ImGui::SliderFloat2("unitMinSize", &unitMinSize_.x, 0.0f, 10.0f);
	//ImGui::SliderFloat3("effectArea", &effectArea_.x, 0.0f, 200.0f);
	//ImGui::SliderFloat3("direction", &direction_.x, 0.0f, 200.0f);
	//ImGui::SliderFloat("speed", &speed_, 0.0f, 100.0f);
	//ImGui::SliderFloat("speedReduce", &speedReduce_, 0.0f, 1.0f);
	//ImGui::SliderFloat("createNumber", &createNumber_, 0.0f, 10.0f);
	//ImGui::End();
	//
	//ImGui::Begin("Part");
	//std::string index = "part";
	//for (size_t i = 0; i < particleObjectList_.size(); ++i) {
	//	ParticleData& fd = *particleObjectList_[i];
	//	ImGui::Text((index + std::to_string(i)).c_str());
	//	ImGui::SliderFloat3(("Pos_" + std::to_string(i)).c_str(), &fd.part->mainPosition.transform.translate.x, -100.0f, 100.0f);
	//	ImGui::SliderFloat3(("Rotate_" + std::to_string(i)).c_str(), &fd.part->mainPosition.transform.rotate.x, -10.0f, 10.0f);
	//	ImGui::SliderFloat(("Size_" + std::to_string(i)).c_str(), &fd.size, 0.0f, 10.0f);
	//}
	//ImGui::End();

}
#endif
