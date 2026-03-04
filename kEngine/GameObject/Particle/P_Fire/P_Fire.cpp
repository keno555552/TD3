#include "P_Fire.h"

//void P_Fire::IntFire(kEngine* system) {
//	Particle::IntParticele(system);
//}
//
//void P_Fire::Update(Camera* camera) {
//
//	/// パーティクル計算
//
//	CreateFire();
//	UpdateFire();
//	DeleteFire();
//
//	/// wvp更新
//	Particle::Update(camera);
//
//}
//
//void P_Fire::Draw() {
//
//	if(objectParts_.size() != 0){
//		Particle::Draw();
//	}
//
//#ifdef USE_IMGUI
//	ImguiPart();
//#endif
//}
//
//void P_Fire::CreateFire() {
//
//	/// 倒數時鐘更新
//	createTimer.foreverUp();
//
//	/// 作成判定
//	if (createTimer.parameter_ == 0) {
//
//
//		// 最大数チェック
//		if ((int)fireDataList_.size() >= maxParticleNum) return;
//
//		Object::CreateDefaultData();
//		objectParts_.back().transform.translate = mainPosition.transform.translate;
//		objectParts_.back().materialConfig = commonMaterialConfig;
//
//		/// 旋轉ランダム
//		float rx = randomMaker_->randomFloat(0.0f, (float)M_PI);
//		float ry = randomMaker_->randomFloat(0.0f, (float)M_PI);
//		float rz = randomMaker_->randomFloat(0.0f, (float)M_PI);
//		objectParts_.back().transform.rotate = Vector3(rx, ry, rz);
//
//		/// スケールランダム
//		float size = randomMaker_->randomFloat(unitMinSize.x, unitMaxSize.x);
//		objectParts_.back().transform.scale = Vector3(size, size, size);
//
//		/// FireData追加
//		fireDataList_.insert(fireDataList_.end(), FireData{});
//
//		/// 向きセット
//		rx = randomMaker_->randomFloat(-1.0f, 1.0f);
//		ry = randomMaker_->randomFloat(-1.0f, 1.0f);
//		rz = randomMaker_->randomFloat(-1.0f, 1.0f);
//		fireDataList_.back().direction = Vector3(rx, ry, rz);
//
//		/// sizeセット
//		fireDataList_.back().size = size;
//
//		/// lifeTimeセット
//		float lifeTime = randomMaker_->randomFloat(minLifeTime, maxLifeTime);
//		fireDataList_.back().lifeTimeTimer.Init0(lifeTime, system_->GetTimeManager());
//
//		/// partセット
//		fireDataList_.back().part = &objectParts_.back();
//
//	}
//}
//
//void P_Fire::UpdateFire() {
//
//	for (auto& ptr : fireDataList_) {
//
//		float deltaTime = system_->GetDeltaTime();
//
//		/// FireData更新
//		ptr.lifeTimeTimer.ToMix();
//
//		/// part更新
//		ptr.part->transform.rotate.x += ptr.direction.x * speed * deltaTime;
//		ptr.part->transform.rotate.y += ptr.direction.y * speed * deltaTime;
//		ptr.part->transform.rotate.z += ptr.direction.z * speed * deltaTime;
//
//		ptr.part->transform.translate.x += ptr.direction.x * speed * deltaTime;
//		ptr.part->transform.translate.y += ptr.direction.y * speed * deltaTime;
//		ptr.part->transform.translate.z += ptr.direction.z * speed * deltaTime;
//
//		ptr.part->transform.scale;
//	}
//}
//
//void P_Fire::DeleteFire() {
//
//	for (auto it = fireDataList_.begin(); it != fireDataList_.end(); ) {
//		FireData& fd = *it;
//		bool dead = false;
//		if (it->lifeTimeTimer.parameter_ >= it->lifeTimeTimer.maxTime_)dead = true;
//		if (it->part->transform.translate.x > mainPosition.transform.translate.x + effectArea.x / 2.0f)dead = true;
//		if (it->part->transform.translate.x < mainPosition.transform.translate.x - effectArea.x / 2.0f)dead = true;
//		if (it->part->transform.translate.y > mainPosition.transform.translate.y + effectArea.y / 2.0f)dead = true;
//		if (it->part->transform.translate.y < mainPosition.transform.translate.y - effectArea.y / 2.0f)dead = true;
//		if (it->part->transform.translate.z > mainPosition.transform.translate.z + effectArea.z / 2.0f)dead = true;
//		if (it->part->transform.translate.z < mainPosition.transform.translate.z - effectArea.z / 2.0f)dead = true;
//
//		if (dead) {
//			auto partIt = std::find_if(objectParts_.begin(), objectParts_.end(),
//				[&](auto& obj) { return &obj == fd.part; });
//
//			if (partIt != objectParts_.end()) {
//				objectParts_.erase(partIt);
//			}
//
//			it = fireDataList_.erase(it);
//		} else {
//			++it;
//		}
//	}
//
//
//}
//
//
//#ifdef USE_IMGUI
//void P_Fire::ImguiPart() {
//
//	ImGui::Begin("FireMain");
//	ImGui::SliderFloat3("rootPos", &mainPosition.transform.translate.x, -20.0f, 20.0f);
//	ImGui::SliderFloat("maxLifeTime", &maxLifeTime, 0.0f, 10.0f);
//	ImGui::SliderFloat("minLifeTime", &minLifeTime, 0.0f, 10.0f);
//	ImGui::SliderFloat2("unitMaxSize", &unitMaxSize.x, 0.0f, 10.0f);
//	ImGui::SliderFloat2("unitMinSize", &unitMinSize.x, 0.0f, 10.0f);
//	ImGui::SliderFloat3("effectArea", &effectArea.x, 0.0f, 200.0f);
//	ImGui::SliderFloat3("direction", &direction.x, 0.0f, 200.0f);
//	ImGui::SliderFloat("speed", &speed, 0.0f, 100.0f);
//	ImGui::SliderFloat("speedReduce", &speedReduce, 0.0f, 1.0f);
//	ImGui::SliderFloat("createNumber", &createNumber, 0.0f, 10.0f);
//
//	ImGui::SliderInt("maxParticleNum", &maxParticleNum, 0, 5000);
//	ImGui::End();
//
//	ImGui::Begin("Part");
//	std::string index = "part";
//	for (size_t i = 0; i < fireDataList_.size(); ++i) {
//		FireData& fd = fireDataList_[i];
//		ImGui::Text((index + std::to_string(i)).c_str());
//		ImGui::SliderFloat3(("Pos_" + std::to_string(i)).c_str(), &fd.part->transform.translate.x, -100.0f, 100.0f);
//		ImGui::SliderFloat3(("Rotate_" + std::to_string(i)).c_str(), &fd.part->transform.rotate.x, -10.0f, 10.0f);
//		ImGui::SliderFloat(("Size_" + std::to_string(i)).c_str(), &fd.size, 0.0f, 10.0f);
//	}
//	ImGui::End();
//}
//#endif