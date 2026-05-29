#include "EffectManager.h"
#include "kEngine.h"

void EffectManager::Initialize(kEngine* system) {
		system_ = system;
		particleManager_ = std::make_unique<ParticleManager>(system);
}

void EffectManager::Finalize() {}


void EffectManager::Update() {

	particleManager_->Update();

}

void EffectManager::Draw() {
	particleManager_->Draw();
}
