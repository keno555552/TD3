#include "Perfect_Particle.h"

Perfect_Particle::Perfect_Particle(kEngine *system) : Particle(system) {
  createTimer.Init0(defaultParticleInterval_, system_->GetTimeManager());

  defaultModelHandle_ =
      system_->SetModelObj("GAME/resources/object/Plane/plane.gltf");
  defaultTextureHandle_ =
      system_->LoadTexture("GAME/resources/texture/white100x100.png");

  commonMaterialConfig->useModelTexture = false;
  commonMaterialConfig->textureHandle = defaultTextureHandle_;
  commonMaterialConfig->textureColor = Vector4(1.0f, 1.0f, 0.2f, 1.0f);
}

Perfect_Particle::~Perfect_Particle() {}

void Perfect_Particle::Update(Camera *camera) {
  UpdatePerfect();
  DeletePerfect();
  Particle::Update(camera);
}

void Perfect_Particle::Draw() { Particle::Draw(); }

void Perfect_Particle::Spawn(const Vector3 &pos, KickEffectType type) {
  SetRootPos(pos);

  int spawnCount = 5;
  float speed = 5.0f;
  float life = 0.2f;
  Vector3 baseScale = {0.15f, 0.15f, 0.15f};
  Vector4 color = {1.0f, 1.0f, 0.2f, 1.0f};

  switch (type) {
  case KickEffectType::Perfect:
    spawnCount = 18;
    speed = 9.0f;
    life = 0.35f;
    baseScale = {0.35f, 0.35f, 0.35f};
    color = {6.0f, 5.5f, 1.5f, 1.0f};
    break;

  case KickEffectType::Good:
    spawnCount = 10;
    speed = 6.0f;
    life = 0.25f;
    baseScale = {0.22f, 0.22f, 0.22f};
    color = {2.0f, 4.0f, 1.5f, 1.0f};
    break;

  case KickEffectType::Bad:
    spawnCount = 6;
    speed = 3.5f;
    life = 0.18f;
    baseScale = {0.20f, 0.20f, 0.20f};
    color = {1.0f, 0.2f, 0.2f, 1.0f};
    break;
  }

  for (int i = 0; i < spawnCount; i++) {
    Particle::AddObject();
    auto &p = particleObjectList_.back();

    p->part->CreateModelData(defaultModelHandle_);
    *p->part->objectParts_[0].materialConfig = *commonMaterialConfig;

    p->part->objectParts_[0].materialConfig->useModelTexture = false;
    p->part->objectParts_[0].materialConfig->textureColor = color;

    p->part->mainPosition.transform = CreateDefaultTransform();
    p->part->mainPosition.transform.translate = pos;

    float scaleJitter = randomMaker_->randomFloat(0.8f, 1.3f);
    p->part->mainPosition.transform.scale = {baseScale.x * scaleJitter,
                                             baseScale.y * scaleJitter,
                                             baseScale.z * scaleJitter};

    p->part->isBillboard_ = true;

    float rx = randomMaker_->randomFloat(-1.4f, 1.4f);
    float ry = randomMaker_->randomFloat(0.6f, 1.6f);
    float rz = randomMaker_->randomFloat(-1.4f, 1.4f);

    Vector3 dir = {rx, ry, rz};
    p->velocity = dir * speed;

    p->lifeTimeTimer.Init0(life, system_->GetTimeManager());
  }
}

void Perfect_Particle::ClearAll() {
  for (auto &p : particleObjectList_) {
    if (p != nullptr) {
      if (p->part != nullptr) {
        delete p->part;
        p->part = nullptr;
      }

      delete p;
      p = nullptr;
    }
  }

  particleObjectList_.clear();
}

void Perfect_Particle::UpdatePerfect() {
  for (auto &object : particleObjectList_) {
    float t = object->lifeTimeTimer.parameter_ / object->lifeTimeTimer.maxTime_;
    object->part->objectParts_[0].materialConfig->textureColor.w = 1.0f - t;
  }
}

void Perfect_Particle::DeletePerfect() {
  for (auto &fd : particleObjectList_) {
    if (fd->lifeTimeTimer.parameter_ >= fd->lifeTimeTimer.maxTime_) {
      fd->isAlive = false;
    }
  }
}