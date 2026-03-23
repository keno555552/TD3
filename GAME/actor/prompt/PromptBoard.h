#pragma once
#include "Object/Sprite.h"

class PromptBoard {
public:
  PromptBoard() = default;
  ~PromptBoard();

  void Initialize(kEngine *system, const Vector2 &position);
  void Update();
  void Draw();

  void SetPromptTexture(int textureHandle);
  void StartStopAnimation();

  bool IsStopAnimationFinished() const { return isStopAnimationFinished_; }

private:
  SimpleSprite *CreateSprite(int textureHandle, const Vector3 &translate,
                             const Vector2 &anchorPoint);
  void UpdateRollingAnimation();
  void UpdateStopAnimation();
  void ApplyFlapRotation(float topAngle, float bottomAngle);
  void SetSpriteAlpha(SimpleSprite *sprite, float alpha);
  void UpdateSprites();

private:
  kEngine *system_ = nullptr;

  SimpleSprite *frameSprite_ = nullptr;
  SimpleSprite *hingeSprite_ = nullptr;
  SimpleSprite *topFlapSprite_ = nullptr;
  SimpleSprite *bottomFlapSprite_ = nullptr;
  SimpleSprite *promptSprite_ = nullptr;

  int frameTextureHandle_ = 0;
  int hingeTextureHandle_ = 0;
  int flapTextureHandle_ = 0;
  int promptTextureHandle_ = 0;

  bool isRolling_ = true;
  bool isStopAnimation_ = false;
  bool isStopAnimationFinished_ = false;

  int rollingFrameCounter_ = 0;
  int stopAnimationCounter_ = 0;

  int stopAnimationFrame_ = 16;
  int promptRevealFrame_ = 10;

  float flapMaxAngle_ = 1.10f;
};