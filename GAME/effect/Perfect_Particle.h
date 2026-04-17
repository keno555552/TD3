#pragma once
#include "Particle/Particle.h"
#include "kEngine.h"

enum class KickEffectType { Perfect, Good, Bad };

class Perfect_Particle : public Particle {
public:
  Perfect_Particle(kEngine *system);
  ~Perfect_Particle();

  void Update(Camera *camera) override;
  void Draw() override;

  void Spawn(const Vector3 &pos, KickEffectType type);

  void ClearAll();

private:
  void UpdatePerfect();
  void DeletePerfect();

private:
  int defaultModelHandle_ = -1;
  int defaultTextureHandle_ = -1;
};