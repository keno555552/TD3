#include "ModPartParticle.h"

ModPartParticle::ModPartParticle(kEngine *system) : Particle(system) {
  createTimer.Init0(defaultParticleInterval_, system_->GetTimeManager());

  defaultModelHandle_ =
      system_->SetModelObj("GAME/resources/object/Plane/plane.gltf");
  defaultTextureHandle_ =
      system_->LoadTexture("GAME/resources/ModScene/particle.png");

  commonMaterialConfig->useModelTexture = false;
  commonMaterialConfig->textureHandle = defaultTextureHandle_;
  commonMaterialConfig->textureColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
}

ModPartParticle::~ModPartParticle() { ClearAll(); }

void ModPartParticle::Update(Camera *camera) {
  UpdateParticles();
  DeleteDeadParticles();
  Particle::Update(camera);
}

void ModPartParticle::Draw() { Particle::Draw(); }

void ModPartParticle::Spawn(const Vector3 &pos, ModPartParticleType type) {
  SetRootPos(pos);

  int spawnCount = 12;
  float speed = 3.0f;
  float life = 0.5f;
  Vector3 baseScale = {0.5f, 0.5f, 0.5f};
  Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};

  if (type == ModPartParticleType::Add) {
    // 追加時は勢いよく弾ける
    spawnCount = 20;
    speed = 4.5f;
    baseScale = {0.6f, 0.6f, 0.6f};
  } else if (type == ModPartParticleType::Remove) {
    // 削除時
    spawnCount = 15;
    speed = 3.0f;
    baseScale = {0.4f, 0.4f, 0.4f};
  }

  for (int i = 0; i < spawnCount; i++) {
    Particle::AddObject();
    auto &p = particleObjectList_.back();

    // 彩度と明度を上げるため、HSVのようなアプローチで色を生成
    // 明度(Value)を高く、彩度(Saturation)を高く設定する
    float min_val = randomMaker_->randomFloat(0.4f, 0.6f); // 最小値を上げて少し白み（パステル感）を混ぜ、彩度を抑える
    float max_val = randomMaker_->randomFloat(0.9f, 1.0f); // 高い値 (高明度)
    float mid_val = randomMaker_->randomFloat(min_val, max_val);

    float r = 0.0f, g = 0.0f, b = 0.0f;
    int randType = randomMaker_->randomInt(0, 5);
    if (randType == 0) { r = max_val; g = mid_val; b = min_val; }
    else if (randType == 1) { r = max_val; g = min_val; b = mid_val; }
    else if (randType == 2) { r = mid_val; g = max_val; b = min_val; }
    else if (randType == 3) { r = min_val; g = max_val; b = mid_val; }
    else if (randType == 4) { r = mid_val; g = min_val; b = max_val; }
    else { r = min_val; g = mid_val; b = max_val; }

    Vector4 particleColor = {r, g, b, 1.0f};
    
    // 追加時はより明るく（発光っぽく）
    if (type == ModPartParticleType::Add) {
        particleColor.x *= 1.5f;
        particleColor.y *= 1.5f;
        particleColor.z *= 1.5f;
    }

    p->part->CreateModelData(defaultModelHandle_);
    if (!p->part->objectParts_.empty()) {
        *p->part->objectParts_[0].materialConfig = *commonMaterialConfig;
        p->part->objectParts_[0].materialConfig->useModelTexture = false;
        p->part->objectParts_[0].materialConfig->textureColor = particleColor;
        p->part->objectParts_[0].materialConfig->enableLighting = false;
        p->part->objectParts_[0].materialConfig->lightModelType = LightModelType::Sprite2D;
        
        // エンジン側を変更せずに透過バケット（透明オブジェクトとしての描画順）に入れるためのハック
        p->part->objectParts_[0].materialConfig->textureColor.w = 0.99f;
    }

    p->part->mainPosition.transform = CreateDefaultTransform();
    p->part->mainPosition.transform.translate = pos;

    float scaleJitter = randomMaker_->randomFloat(0.5f, 1.2f);
    p->size = baseScale.x * scaleJitter;
    p->part->mainPosition.transform.scale = {p->size, p->size, p->size};

    p->part->isBillboard_ = true;

    // 全方位に散らばるベクトル
    float rx = randomMaker_->randomFloat(-1.0f, 1.0f);
    float ry = randomMaker_->randomFloat(-1.0f, 1.0f);
    float rz = randomMaker_->randomFloat(-1.0f, 1.0f);

    Vector3 dir = {rx, ry, rz};
    // 長さが0に近い場合の安全対策
    float length = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (length > 0.001f) {
        dir.x /= length;
        dir.y /= length;
        dir.z /= length;
    }

    p->velocity = dir * (speed * randomMaker_->randomFloat(0.5f, 1.5f));

    // 「方向変えずにそのまま」なのでバイアスも削除
    
    p->lifeTimeTimer.Init0(life * randomMaker_->randomFloat(0.8f, 1.2f), system_->GetTimeManager());
  }
}

void ModPartParticle::ClearAll() {
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

void ModPartParticle::UpdateParticles() {
  for (auto &object : particleObjectList_) {
    // 直進させるため、重力（velocity.y の減算）は行わない
    // object->velocity.y -= 3.0f * system_->GetDeltaTime();
    
    // スケールを徐々に小さくする
    float t = object->lifeTimeTimer.parameter_ / object->lifeTimeTimer.maxTime_;
    float scaleProgress = 1.0f - t;
    
    float currentScale = object->size * scaleProgress;
    object->part->mainPosition.transform.scale = {currentScale, currentScale, currentScale};

    // アルファ値も減衰させる
    if (!object->part->objectParts_.empty()) {
        object->part->objectParts_[0].materialConfig->textureColor.w = 1.0f - t;
    }
  }
}

void ModPartParticle::DeleteDeadParticles() {
  for (auto &fd : particleObjectList_) {
    if (fd->lifeTimeTimer.parameter_ >= fd->lifeTimeTimer.maxTime_) {
      fd->isAlive = false;
    }
  }
}
