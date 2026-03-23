#include "Fade.h"
#include "kEngine.h"

Fade::~Fade() { delete sprite_; }

void Fade::Initialize(kEngine *system) {
  system_ = system;

  textureHandle_ =
      system_->LoadTexture("./kEngine/EngineAssets/TemplateResource/texture/white5x5.png");

  sprite_ = new SimpleSprite;
  sprite_->IntObject(system_);
  sprite_->CreateDefaultData();
  sprite_->objectParts_[0].materialConfig->textureHandle = textureHandle_;

  sprite_->mainPosition.transform.scale = {1280.0f, 720.0f, 1.0f};
  sprite_->mainPosition.transform.translate = {0.0f, 0.0f, 0.0f};
  sprite_->objectParts_[0].materialConfig->textureColor = {0.0f, 0.0f, 0.0f,
                                                           alpha_};
}

void Fade::Update(Camera *camera) {
  if (state_ == State::FadeIn) {
    alpha_ -= speed_;
    if (alpha_ <= 0.0f) {
      alpha_ = 0.0f;
      state_ = State::None;
      isFinished_ = true;
    }
  } else if (state_ == State::FadeOut) {
    alpha_ += speed_;
    if (alpha_ >= 1.0f) {
      alpha_ = 1.0f;
      state_ = State::None;
      isFinished_ = true;
    }
  }

  sprite_->objectParts_[0].materialConfig->textureColor.w = alpha_;
  sprite_->Update(camera);
}

void Fade::Draw() { sprite_->Draw(); }

void Fade::StartFadeIn(float speed) {
  speed_ = speed;
  state_ = State::FadeIn;
  isFinished_ = false;
}

void Fade::StartFadeOut(float speed) {
  speed_ = speed;
  state_ = State::FadeOut;
  isFinished_ = false;
}

bool Fade::IsFinished() const { return isFinished_; }

bool Fade::IsBusy() const { return state_ != State::None; }

float Fade::GetAlpha() const { return alpha_; }
