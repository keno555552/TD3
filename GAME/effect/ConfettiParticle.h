#pragma once
#include "Particle/Particle.h"
#include "kEngine.h"
#include <vector>

// 紙吹雪一つ一つの追加データ（ひらひら舞うため）
struct ConfettiData {
    float phaseX = 0.0f;
    float phaseZ = 0.0f;
    float freqX = 1.0f;
    float freqZ = 1.0f;
    float rotSpeedX = 0.0f;
    float rotSpeedY = 0.0f;
    float rotSpeedZ = 0.0f;
    Vector3 baseVelocity = {0,0,0};
};

class ConfettiParticle : public Particle {
public:
  ConfettiParticle(kEngine *system);
  ~ConfettiParticle();

  void Update(Camera *camera) override;
  void Draw() override;

  // 紙吹雪を発生させる
  void Spawn(const Vector3 &pos);

  void ClearAll();

private:
  void UpdateConfetti();
  void DeleteConfetti();

private:
  int defaultModelHandle_ = -1;
  int defaultTextureHandle_ = -1;
  std::vector<ConfettiData> confettiDataList_; // particleObjectList_ と同期して管理
};
