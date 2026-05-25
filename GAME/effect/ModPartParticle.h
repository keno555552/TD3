#pragma once
#include "Particle/Particle.h"
#include "kEngine.h"

enum class ModPartParticleType {
  Add,
  Remove
};

class ModPartParticle : public Particle {
public:
  ModPartParticle(kEngine *system);
  ~ModPartParticle();

  void Update(Camera *camera) override;
  void Draw() override;

  void Spawn(const Vector3 &pos, ModPartParticleType type);

  void ClearAll();

private:
  void UpdateParticles();
  void DeleteDeadParticles();

private:
  int defaultModelHandle_ = -1;
  int defaultTextureHandle_ = -1;
};
