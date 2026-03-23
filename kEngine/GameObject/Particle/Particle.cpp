#include "Particle.h"


Particle::Particle(kEngine* system) {
	system_ = system;
	commonMaterialConfig = std::make_shared<MaterialConfig>();
	InitMaterialConfig(commonMaterialConfig.get());

	createTimer.Init0(defaultParticleInterval_, system_->GetTimeManager());

	anchorPart_.transform = CreateDefaultTransform();
	anchorPart_.materialConfig = std::make_shared<MaterialConfig>();
	InitMaterialConfig(anchorPart_.materialConfig.get());
}

Particle::~Particle() {
	commonMaterialConfig.reset();

	for (auto& object : particleObjectList_) {
		delete object, object = nullptr;
	}

	randomMaker_.reset();

}

void Particle::Update(Camera* camera) {

	for (auto object = particleObjectList_.begin(); object != particleObjectList_.end();) {
		if ((*object)->isAlive == false) {
			delete* object, * object = nullptr;
			object = particleObjectList_.erase(object);
		} else {
			auto obj = (*object);

			float deltaTime = system_->GetDeltaTime();

			obj->lifeTimeTimer.ToMix();
			//obj->part->mainPosition.transform.scale *= obj->direction.scale * obj->scaleSpeed * deltaTime;
			obj->part->mainPosition.transform.rotate += obj->direction.rotate * obj->rotateSpeed * deltaTime;
			obj->part->mainPosition.transform.translate += obj->velocity * deltaTime;
			obj->part->Update(camera);
			object++;
		}
	}

}

void Particle::Draw() {
	for (auto& obj : particleObjectList_) {
		obj->part->Draw();
	}
}

void Particle::SetCommonMaterialConfig(const MaterialConfig& material) {
	*commonMaterialConfig = material;
}

void Particle::SetRootPos(Vector3 pos) {
	anchorPart_.transform.translate = pos;
}

void Particle::SetRootRotate(Vector3 pos) {
	anchorPart_.transform.rotate = pos;
}

void Particle::SetRootScale(Vector3 pos) {
	anchorPart_.transform.scale = pos;
}

void Particle::AddObject() {
	ParticleData* newParticle = new ParticleData();
	particleObjectList_.insert(particleObjectList_.end(), newParticle);
	particleObjectList_.back()->part = new Object();
	particleObjectList_.back()->part->IntObject(system_);
}

std::vector<Object*> Particle::GetObjectList() {
	std::vector<Object*> outList{};
	for (auto& obj : particleObjectList_) {
		outList.push_back(obj->part);
	}
	return outList;
}

void Particle::SetSeed(unsigned int seed) {
	randomMaker_->SetSeed(seed);
}

void Particle::SetTimer(float maxTime) {
	createTimer.maxTime_ = maxTime;
	if (createTimer.parameter_ > maxTime)createTimer.parameter_ = maxTime;
}
