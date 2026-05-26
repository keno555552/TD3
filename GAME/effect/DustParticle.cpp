#include "DustParticle.h"
#include <cmath>

DustParticle::DustParticle(kEngine* system) : Particle(system) {
	defaultModelHandle_ = system_->SetModelObj("GAME/resources/object/Plane/plane.gltf");
	// ※外部生成リソースを削除し、既存のテクスチャのみを使用します
	defaultTextureHandle_ = system_->LoadTexture("GAME/resources/ModScene/particle.png");
	whiteTexHandle_ = system_->LoadTexture("GAME/resources/texture/white100x100.png");
}

void DustParticle::Spawn(const Vector3& pos) {
	// 1. アニメ調の土煙（複数の球が急速に膨らみ、空中で縮んで消えることで、1つの雲のように見せる）
	for (int i = 0; i < 15; ++i) {
		AddObject();
		auto* p = particleObjectList_.back();
		
		p->direction.translate = pos;
		p->scaleSpeed = 1.0f; // Smoke flag
		
		// 着地点から放射状に勢いよく広がる
		float angle = (i / 15.0f) * 3.14159f * 2.0f;
		float speed = 0.08f + ((rand() % 100) / 100.0f) * 0.06f;
		float vx = std::cosf(angle) * speed;
		float vz = std::sinf(angle) * speed;
		float vy = 0.02f + ((rand() % 100) / 100.0f) * 0.04f; 
		
		p->velocity = { vx, vy, vz };
		
		p->lifeTimeTimer.Init0(0.4f + (rand()%100)/200.0f, system_->GetTimeManager());
		
		// トゥーン調の砂色（半透明キューに乗せるためアルファを1.0未満にする）
		float c = 0.85f + ((rand() % 100) / 100.0f - 0.5f) * 0.1f;
		p->color = { c, c - 0.05f, c - 0.15f, 0.8f }; 
		
		p->size = 0.15f + ((rand()%100)/100.0f) * 0.15f; // サイズを少し抑える
		
		p->part->CreateModelData(defaultModelHandle_);
		p->part->isBillboard_ = true; 
		if (!p->part->objectParts_.empty()) {
			p->part->objectParts_[0].materialConfig->useModelTexture = false;
			p->part->objectParts_[0].materialConfig->textureHandle = defaultTextureHandle_; 
			p->part->objectParts_[0].materialConfig->textureColor = p->color;
			p->part->objectParts_[0].materialConfig->enableLighting = false; 
		}
		p->direction.rotate = { 0.0f, 0.0f, (rand()%100)/100.0f * 6.28f }; 
	}

	// 2. 物理デブリ（飛び散る土くれ）
	for (int i = 0; i < 8; ++i) {
		AddObject();
		auto* p = particleObjectList_.back();
		
		p->direction.translate = pos;
		p->scaleSpeed = 2.0f; // Debris flag
		
		float vx = ((rand() % 100) / 50.0f - 1.0f) * 0.12f; 
		float vz = ((rand() % 100) / 50.0f - 1.0f) * 0.08f; 
		float vy = ((rand() % 100) / 100.0f) * 0.15f + 0.05f; // 高く跳ねる
		p->velocity = { vx, vy, vz };
		
		p->lifeTimeTimer.Init0(0.4f + (rand()%100)/200.0f, system_->GetTimeManager());
		
		p->color = { 0.35f, 0.3f, 0.25f, 0.99f }; // デブリも半透明キューに入れて深度クリッピングを防ぐ
		p->size = 0.02f + ((rand()%100)/100.0f) * 0.03f; 
		
		p->part->CreateModelData(defaultModelHandle_);
		p->part->isBillboard_ = true; 
		if (!p->part->objectParts_.empty()) {
			p->part->objectParts_[0].materialConfig->useModelTexture = false;
			p->part->objectParts_[0].materialConfig->textureHandle = whiteTexHandle_;
			p->part->objectParts_[0].materialConfig->textureColor = p->color;
			p->part->objectParts_[0].materialConfig->enableLighting = false; 
		}
		p->direction.rotate = { 0.0f, 0.0f, (rand()%100)/100.0f * 6.28f }; 
	}
}

void DustParticle::Update(Camera* camera) {
	for (auto it = particleObjectList_.begin(); it != particleObjectList_.end();) {
		auto* p = *it;
		p->lifeTimeTimer.ToMix();
		if (p->lifeTimeTimer.GetIsMax()) {
			delete p->part;
			delete p;
			it = particleObjectList_.erase(it);
			continue;
		}
		
		float progress = p->lifeTimeTimer.linearity(); 
		
		if (p->scaleSpeed == 1.0f) {
			// --- アニメ調の煙 ---
			p->velocity.x *= 0.85f; 
			p->velocity.z *= 0.85f;
			p->velocity.y *= 0.92f; 
			
			p->direction.translate.x += p->velocity.x;
			p->direction.translate.y += p->velocity.y;
			p->direction.translate.z += p->velocity.z;
			
			// サイズアニメーション（膨らむ -> 停滞 -> 縮んで消える）
			float currentSize = 0.0f;
			if (progress < 0.2f) {
				currentSize = p->size * (progress / 0.2f);
			} else if (progress < 0.6f) {
				float t = (progress - 0.2f) / 0.4f;
				currentSize = p->size + (p->size * 0.2f) * t;
			} else {
				float t = (progress - 0.6f) / 0.4f;
				currentSize = (p->size * 1.2f) * (1.0f - std::powf(t, 2.0f));
			}
			
			p->direction.rotate.z += 0.02f; 
			
			p->part->mainPosition.transform = p->direction;
			p->part->mainPosition.transform.scale = {currentSize, currentSize, currentSize};
			
			if (!p->part->objectParts_.empty()) {
				// 時間経過でフワッと消える（半透明合成）
				p->color.w = 0.8f * (1.0f - std::powf(progress, 2.0f)); 
				p->part->objectParts_[0].materialConfig->textureColor = p->color;
			}
		} else if (p->scaleSpeed == 2.0f) {
			// --- Debris ---
			p->velocity.x *= 0.97f; 
			p->velocity.z *= 0.97f;
			p->velocity.y -= 0.015f; 
			
			p->direction.translate.x += p->velocity.x;
			p->direction.translate.y += p->velocity.y;
			p->direction.translate.z += p->velocity.z;
			
			if (p->direction.translate.y < -0.35f) {
				p->direction.translate.y = -0.35f;
				p->velocity.y *= -0.5f; 
				p->velocity.x *= 0.7f;  
				p->velocity.z *= 0.7f;
			}
			
			p->direction.rotate.z += (p->velocity.x > 0 ? -0.1f : 0.1f); 
			p->direction.rotate.x += 0.1f;
			p->direction.rotate.y += 0.1f;
			
			float currentSize = p->size * (1.0f - progress);
			
			p->part->mainPosition.transform = p->direction;
			p->part->mainPosition.transform.scale = {currentSize, currentSize, currentSize};
		}
		
		if (!p->part->objectParts_.empty()) {
			p->part->objectParts_[0].materialConfig->textureColor = p->color;
		}
		p->part->Update(camera);
		
		++it;
	}
}

void DustParticle::Draw() {
	for (auto* p : particleObjectList_) {
		p->part->Draw();
	}
}
