#include "Object/Sprite.h"
#include "Object/Object.h"
#include "GAME/effect/ConfettiParticle.h"
#include <vector>
#include <memory>

class PromptBoard {
public:
  PromptBoard() = default;
  ~PromptBoard();

  void Initialize(kEngine *system, const Vector2 &position);
  void Update(Camera* camera);
  void Draw();

  void SetPromptTexture(int textureHandle);
  void StartStopAnimation();
  void SetThemeTextures(const std::vector<int>& textureHandles);

  bool IsStopAnimationFinished() const { return isStopAnimationFinished_; }

private:
  SimpleSprite *CreateSprite(int textureHandle, const Vector3 &translate,
                             const Vector2 &anchorPoint);
  Object *CreateFlapObject(int modelHandle, int textureHandle, const Vector3 &translate);
  void UpdateRollingAnimation();
  void UpdateStopAnimation();
  void ApplyFlapRotation(float angle);
  void UpdateFlapTextures(int currentTex, int nextTex);
  void SetSpriteAlpha(SimpleSprite *sprite, float alpha);
  void SetObjectAlpha(Object *object, float alpha);
  void UpdateSprites(Camera* camera);

private:
  kEngine *system_ = nullptr;

  Object *topFlapObject_ = nullptr;
  Object *bottomFlapObject_ = nullptr;
  Object *fallingFlapUpperObject_ = nullptr;
  Object *fallingFlapLowerObject_ = nullptr;

  int flapTextureHandle_ = 0;
  int promptTextureHandle_ = 0;

  Object* frameObject_ = nullptr;

  int topFlapModelHandle_ = 0;
  int bottomFlapModelHandle_ = 0;

  std::vector<int> dummyTextureHandles_;
  int currentDummyIndex_ = 0;

  bool isRolling_ = true;
  bool willStopOnNextFlap_ = false;
  bool isStopAnimation_ = false;
  bool isStopAnimationFinished_ = false;
  bool isShowingFinal_ = false;

  bool isBounceAnimation_ = false;
  float bounceAnimationCounter_ = 0.0f;

  int rollingFrameCounter_ = 0;
  int stopAnimationCounter_ = 0;

  int stopAnimationFrame_ = 16;
  int promptRevealFrame_ = 10;

  float fallingFlapAngle_ = 0.0f;
  bool isFlapFallingUpper_ = true;

  bool hasSpawnedParticle_ = false;
  std::unique_ptr<ConfettiParticle> confettiParticle_ = nullptr;
};