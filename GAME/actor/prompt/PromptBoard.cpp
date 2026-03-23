#include "PromptBoard.h"

namespace {

const float kPi = 3.14159265f;

} // namespace

PromptBoard::~PromptBoard() {
  delete frameSprite_;
  delete hingeSprite_;
  delete topFlapSprite_;
  delete bottomFlapSprite_;
  delete promptSprite_;
}

void PromptBoard::Initialize(kEngine *system, const Vector2 &position) {
  system_ = system;

  frameTextureHandle_ = system_->LoadTexture("GAME/resources/texture/frame.png");
  hingeTextureHandle_ = system_->LoadTexture("GAME/resources/texture/hinge.png");
  flapTextureHandle_ =
      system_->LoadTexture("GAME/resources/texture/ReversibleFlap.png");
  promptTextureHandle_ =
      system_->LoadTexture("GAME/resources/texture/prompt.png");

  frameSprite_ = CreateSprite(frameTextureHandle_,
                              {position.x, position.y, 20.0f},
                              {0.0f, 0.0f});

  promptSprite_ =
      CreateSprite(promptTextureHandle_, {0.0f, 0.0f, 10.0f}, {0.0f, 0.0f});
  promptSprite_->followObject_ = frameSprite_;

  topFlapSprite_ =
      CreateSprite(flapTextureHandle_, {250.0f, 75.0f, 14.0f}, {250.0f, 50.0f});
  topFlapSprite_->followObject_ = frameSprite_;

  bottomFlapSprite_ =
      CreateSprite(flapTextureHandle_, {250.0f, 75.0f, 15.0f}, {250.0f, 0.0f});
  bottomFlapSprite_->followObject_ = frameSprite_;

  hingeSprite_ =
      CreateSprite(hingeTextureHandle_, {0.0f, 50.0f, 18.0f}, {0.0f, 0.0f});
  hingeSprite_->followObject_ = frameSprite_;

  SetSpriteAlpha(promptSprite_, 0.0f);
  SetSpriteAlpha(topFlapSprite_, 1.0f);
  SetSpriteAlpha(bottomFlapSprite_, 1.0f);

  ApplyFlapRotation(0.0f, 0.0f);
  UpdateSprites();
}

SimpleSprite *PromptBoard::CreateSprite(int textureHandle,
                                        const Vector3 &translate,
                                        const Vector2 &anchorPoint) {
  SimpleSprite *sprite = new SimpleSprite;
  sprite->IntObject(system_);
  sprite->CreateDefaultData();
  sprite->mainPosition.transform = CreateDefaultTransform();
  sprite->mainPosition.transform.translate = translate;
  sprite->objectParts_[0].anchorPoint = anchorPoint;
  sprite->objectParts_[0].materialConfig->textureHandle = textureHandle;
  return sprite;
}

void PromptBoard::Update() {
  if (isStopAnimation_) {
    UpdateStopAnimation();
  } else if (isRolling_) {
    UpdateRollingAnimation();
  }

  UpdateSprites();
}

void PromptBoard::Draw() {
  if (hingeSprite_ != nullptr) {
    hingeSprite_->Draw();
  }

  if (topFlapSprite_ != nullptr) {
    topFlapSprite_->Draw();
  }

  if (bottomFlapSprite_ != nullptr) {
    bottomFlapSprite_->Draw();
  }

  if (promptSprite_ != nullptr) {
    promptSprite_->Draw();
  }

  if (frameSprite_ != nullptr) {
    frameSprite_->Draw();
  }
}

void PromptBoard::SetPromptTexture(int textureHandle) {
  promptTextureHandle_ = textureHandle;

  if (promptSprite_ != nullptr && !promptSprite_->objectParts_.empty()) {
    promptSprite_->objectParts_[0].materialConfig->textureHandle =
        promptTextureHandle_;
  }
}

void PromptBoard::StartStopAnimation() {
  isRolling_ = false;
  isStopAnimation_ = true;
  isStopAnimationFinished_ = false;
  stopAnimationCounter_ = 0;

  SetSpriteAlpha(promptSprite_, 0.0f);
  SetSpriteAlpha(topFlapSprite_, 1.0f);
  SetSpriteAlpha(bottomFlapSprite_, 1.0f);
}

void PromptBoard::UpdateRollingAnimation() {
  ++rollingFrameCounter_;

  const float wave = sinf(static_cast<float>(rollingFrameCounter_) * 0.75f);
  const float angle = flapMaxAngle_ * wave;

  ApplyFlapRotation(angle, -angle);

  SetSpriteAlpha(promptSprite_, 0.0f);
  SetSpriteAlpha(topFlapSprite_, 1.0f);
  SetSpriteAlpha(bottomFlapSprite_, 1.0f);
}

void PromptBoard::UpdateStopAnimation() {
  ++stopAnimationCounter_;

  float t = static_cast<float>(stopAnimationCounter_) /
            static_cast<float>(stopAnimationFrame_);
  if (t > 1.0f) {
    t = 1.0f;
  }

  const float angle = flapMaxAngle_ * (1.0f - t) * sinf(t * kPi * 4.0f);
  ApplyFlapRotation(angle, -angle);

  if (stopAnimationCounter_ >= promptRevealFrame_) {
    const int revealFrame = stopAnimationFrame_ - promptRevealFrame_;
    float promptAlpha = 1.0f;

    if (revealFrame > 0) {
      promptAlpha =
          static_cast<float>(stopAnimationCounter_ - promptRevealFrame_) /
          static_cast<float>(revealFrame);
    }

    if (promptAlpha < 0.0f) {
      promptAlpha = 0.0f;
    }
    if (promptAlpha > 1.0f) {
      promptAlpha = 1.0f;
    }

    SetSpriteAlpha(promptSprite_, promptAlpha);
    SetSpriteAlpha(topFlapSprite_, 1.0f - promptAlpha);
    SetSpriteAlpha(bottomFlapSprite_, 1.0f - promptAlpha);
  }

  if (stopAnimationCounter_ >= stopAnimationFrame_) {
    ApplyFlapRotation(0.0f, 0.0f);
    SetSpriteAlpha(promptSprite_, 1.0f);
    SetSpriteAlpha(topFlapSprite_, 0.0f);
    SetSpriteAlpha(bottomFlapSprite_, 0.0f);

    isStopAnimation_ = false;
    isStopAnimationFinished_ = true;
  }
}

void PromptBoard::ApplyFlapRotation(float topAngle, float bottomAngle) {
  if (topFlapSprite_ != nullptr) {
    topFlapSprite_->mainPosition.transform.rotate = {topAngle, 0.0f, 0.0f};
  }

  if (bottomFlapSprite_ != nullptr) {
    bottomFlapSprite_->mainPosition.transform.rotate = {bottomAngle, 0.0f,
                                                        0.0f};
  }
}

void PromptBoard::SetSpriteAlpha(SimpleSprite *sprite, float alpha) {
  if (sprite == nullptr || sprite->objectParts_.empty()) {
    return;
  }

  sprite->objectParts_[0].materialConfig->textureColor.w = alpha;
}

void PromptBoard::UpdateSprites() {
  if (frameSprite_ != nullptr) {
    frameSprite_->Update(nullptr);
  }

  if (promptSprite_ != nullptr) {
    promptSprite_->Update(nullptr);
  }

  if (topFlapSprite_ != nullptr) {
    topFlapSprite_->Update(nullptr);
  }

  if (bottomFlapSprite_ != nullptr) {
    bottomFlapSprite_->Update(nullptr);
  }

  if (hingeSprite_ != nullptr) {
    hingeSprite_->Update(nullptr);
  }
}