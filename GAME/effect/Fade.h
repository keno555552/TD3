#pragma once
#include "Object/Sprite.h"

class kEngine;
class Camera;

class Fade {

public:
  Fade() = default;
  ~Fade();
  Fade(const Fade &) = delete;
  Fade &operator=(const Fade &) = delete;

  void Initialize(kEngine *system);
  void Update(Camera *camera);
  void Draw();

  void StartFadeIn(float speed = 0.03f);
  void StartFadeOut(float speed = 0.03f);

  bool IsFinished() const;
  bool IsBusy() const;
  float GetAlpha() const;

private:
  enum class State {
    None,
    FadeIn,
    FadeOut,
  };

private:
  kEngine *system_ = nullptr;

  int textureHandle_ = 0;
  SimpleSprite *sprite_ = nullptr;

  float alpha_ = 1.0f;
  float speed_ = 0.03f;
  bool isFinished_ = false;

  State state_ = State::None;
};
