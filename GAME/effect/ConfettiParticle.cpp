#include "ConfettiParticle.h"
#include <cmath>

ConfettiParticle::ConfettiParticle(kEngine *system) : Particle(system) {
  createTimer.Init0(defaultParticleInterval_, system_->GetTimeManager());

  // 板ポリゴンと白テクスチャを使用
  defaultModelHandle_ = system_->SetModelObj("GAME/resources/object/Plane/plane.gltf");
  defaultTextureHandle_ = system_->LoadTexture("GAME/resources/texture/white100x100.png");

  commonMaterialConfig->useModelTexture = false;
  commonMaterialConfig->textureHandle = defaultTextureHandle_;
  commonMaterialConfig->textureColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
}

ConfettiParticle::~ConfettiParticle() {}

void ConfettiParticle::Update(Camera *camera) {
  UpdateConfetti();
  DeleteConfetti();
  Particle::Update(camera);
}

void ConfettiParticle::Draw() { Particle::Draw(); }

void ConfettiParticle::Spawn(const Vector3 &pos) {
  SetRootPos(pos);

  int spawnCount = 40; // 紙吹雪の数
  float life = 2.0f; // 寿命（長く残す）
  Vector3 baseScale = {2.5f, 2.5f, 2.5f}; // UIスケールに負けないように大きく

  // 定番のカラフルな色リスト
  Vector4 colors[] = {
      {1.0f, 0.2f, 0.2f, 1.0f}, // 赤
      {0.2f, 1.0f, 0.2f, 1.0f}, // 緑
      {0.2f, 0.5f, 1.0f, 1.0f}, // 青
      {1.0f, 1.0f, 0.2f, 1.0f}, // 黄
      {1.0f, 0.4f, 0.8f, 1.0f}, // ピンク
      {1.0f, 0.6f, 0.2f, 1.0f}  // オレンジ
  };

  for (int i = 0; i < spawnCount; i++) {
    Particle::AddObject();
    auto &p = particleObjectList_.back();

    p->part->CreateModelData(defaultModelHandle_);
    if (!p->part->objectParts_.empty()) {
        *p->part->objectParts_[0].materialConfig = *commonMaterialConfig;
        p->part->objectParts_[0].materialConfig->useModelTexture = false;
        
        // ランダムな色を選択
        int colorIdx = randomMaker_->randomInt(0, 5);
        p->part->objectParts_[0].materialConfig->textureColor = colors[colorIdx];
    }

    p->part->mainPosition.transform = CreateDefaultTransform();
    
    // 初期位置を少し散らす
    float px = pos.x + randomMaker_->randomFloat(-15.0f, 15.0f);
    float py = pos.y + randomMaker_->randomFloat(-5.0f, 5.0f);
    float pz = pos.z + randomMaker_->randomFloat(-5.0f, 5.0f);
    p->part->mainPosition.transform.translate = {px, py, pz};

    // 初期回転
    p->part->mainPosition.transform.rotate = {
        randomMaker_->randomFloat(0.0f, 6.28f),
        randomMaker_->randomFloat(0.0f, 6.28f),
        randomMaker_->randomFloat(0.0f, 6.28f)
    };

    float scaleJitter = randomMaker_->randomFloat(0.8f, 1.5f);
    p->part->mainPosition.transform.scale = {baseScale.x * scaleJitter,
                                             baseScale.y * scaleJitter,
                                             baseScale.z * scaleJitter};

    // 紙吹雪なのでビルボードはオフにし、回転をそのまま見せる
    p->part->isBillboard_ = false;

    // 初速（上方向に吹き飛ぶ）
    float vx = randomMaker_->randomFloat(-10.0f, 10.0f);
    float vy = randomMaker_->randomFloat(15.0f, 35.0f);
    float vz = randomMaker_->randomFloat(-10.0f, 10.0f);
    p->velocity = {vx, vy, vz};

    p->lifeTimeTimer.Init0(life + randomMaker_->randomFloat(-0.5f, 0.5f), system_->GetTimeManager());

    // ConfettiDataの追加
    ConfettiData cdata;
    cdata.baseVelocity = p->velocity;
    cdata.phaseX = randomMaker_->randomFloat(0.0f, 6.28f);
    cdata.phaseZ = randomMaker_->randomFloat(0.0f, 6.28f);
    cdata.freqX = randomMaker_->randomFloat(2.0f, 5.0f);
    cdata.freqZ = randomMaker_->randomFloat(2.0f, 5.0f);
    cdata.rotSpeedX = randomMaker_->randomFloat(-5.0f, 5.0f);
    cdata.rotSpeedY = randomMaker_->randomFloat(-5.0f, 5.0f);
    cdata.rotSpeedZ = randomMaker_->randomFloat(-5.0f, 5.0f);
    confettiDataList_.push_back(cdata);
  }
}

void ConfettiParticle::ClearAll() {
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
  confettiDataList_.clear();
}

void ConfettiParticle::UpdateConfetti() {
  float deltaTime = system_->GetDeltaTime();
  if (deltaTime <= 0.0f) deltaTime = 0.016f;

  for (size_t i = 0; i < particleObjectList_.size(); ++i) {
    auto &p = particleObjectList_[i];
    if (!p->isAlive) continue;

    // リストの不整合防止（念のため）
    if (i >= confettiDataList_.size()) break;
    auto &cdata = confettiDataList_[i];

    // 重力（下方向への加速度）
    p->velocity.y -= 30.0f * deltaTime; // 重力強め
    
    // 空気抵抗（横方向の速度減衰）
    p->velocity.x *= 0.95f;
    p->velocity.z *= 0.95f;

    // ひらひら揺れる動き
    float time = p->lifeTimeTimer.parameter_;
    float swayX = std::sin(cdata.phaseX + time * cdata.freqX) * 0.5f;
    float swayZ = std::cos(cdata.phaseZ + time * cdata.freqZ) * 0.5f;
    
    p->part->mainPosition.transform.translate.x += swayX;
    p->part->mainPosition.transform.translate.z += swayZ;

    // 回転
    p->part->mainPosition.transform.rotate.x += cdata.rotSpeedX * deltaTime;
    p->part->mainPosition.transform.rotate.y += cdata.rotSpeedY * deltaTime;
    p->part->mainPosition.transform.rotate.z += cdata.rotSpeedZ * deltaTime;

    // フェードアウト
    float t = p->lifeTimeTimer.parameter_ / p->lifeTimeTimer.maxTime_;
    if (t > 0.7f && !p->part->objectParts_.empty()) {
        float alpha = 1.0f - ((t - 0.7f) / 0.3f);
        p->part->objectParts_[0].materialConfig->textureColor.w = alpha;
    }
  }
}

void ConfettiParticle::DeleteConfetti() {
  // 削除処理はParticleクラス自体がリストから要素を削除してしまうと
  // confettiDataList_ とのインデックスがずれるため、
  // ここでは isAlive を false にするだけに留めます。
  // （本来的にはイテレータを使って両方同時にeraseするのが正しいですが、
  // ParticleのUpdate内で消される可能性があるためです。
  // ただTD3のParticleは ClearAll でまとめて消す運用が多そうなので、
  // このシーンのワンショット用途なら isAlive を操作するだけで十分です。）
  for (size_t i = 0; i < particleObjectList_.size(); ++i) {
    auto &p = particleObjectList_[i];
    if (p->lifeTimeTimer.parameter_ >= p->lifeTimeTimer.maxTime_) {
      p->isAlive = false;
    }
  }
}
