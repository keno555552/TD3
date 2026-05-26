#pragma once
#include "kEngine/core/kEngine.h"
#include "kEngine/GameObject/Object/Object.h"
#include "kEngine/GameObject/Particle/Particle.h"

class DustParticle : public Particle {
public:
	DustParticle(kEngine* system);
	void Spawn(const Vector3& pos);
	void Update(Camera* camera) override;
	void Draw() override;

private:
	int defaultModelHandle_ = -1;
	int defaultTextureHandle_ = -1;
	int whiteTexHandle_ = -1;
};
