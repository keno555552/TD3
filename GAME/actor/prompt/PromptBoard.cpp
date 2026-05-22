#include "PromptBoard.h"
#include <cmath>
#include "kEngine/Scenes/SceneManager.h"

namespace {
const float kPi = 3.14159265f;
}

PromptBoard::~PromptBoard() {
  delete topFlapObject_;
  delete bottomFlapObject_;
  delete fallingFlapUpperObject_;
  delete fallingFlapLowerObject_;
}

void PromptBoard::Initialize(kEngine *system, const Vector2 &position) {
  system_ = system;

  flapTextureHandle_ = system_->LoadTexture("GAME/resources/texture/ReversibleFlap.png");
  promptTextureHandle_ = system_->LoadTexture("GAME/resources/texture/themes/prompt.png");
  
  if (dummyTextureHandles_.empty()) {
      dummyTextureHandles_.push_back(flapTextureHandle_);
  }

  topFlapModelHandle_ = system_->SetModelObj("GAME/resources/model/top_flap.obj");
  bottomFlapModelHandle_ = system_->SetModelObj("GAME/resources/model/bottom_flap.obj");

  int initialTex = dummyTextureHandles_[0];

  // The center of the 3D projection, used to align perfectly with the UI
  Vector3 basePos = {0.0f, 0.0f, 100.0f};

  topFlapObject_ = CreateFlapObject(topFlapModelHandle_, initialTex, basePos);
  bottomFlapObject_ = CreateFlapObject(bottomFlapModelHandle_, initialTex, basePos);
  
  fallingFlapUpperObject_ = CreateFlapObject(topFlapModelHandle_, initialTex, basePos);
  fallingFlapLowerObject_ = CreateFlapObject(bottomFlapModelHandle_, initialTex, basePos);

  isRolling_ = true;
  isStopAnimation_ = false;
  isStopAnimationFinished_ = false;
  isShowingFinal_ = false;

  rollingFrameCounter_ = 0;
  stopAnimationCounter_ = 0;
  fallingFlapAngle_ = 0.0f;
  isFlapFallingUpper_ = true;

  ApplyFlapRotation(0.0f);
  UpdateSprites(nullptr);
}

Object *PromptBoard::CreateFlapObject(int modelHandle, int textureHandle, const Vector3 &translate) {
    Object *obj = new Object;
    obj->IntObject(system_);
    obj->CreateModelData(modelHandle);
    
    obj->mainPosition.transform = CreateDefaultTransform();
    obj->mainPosition.transform.translate = translate;
    
    // FOV and distance calculated scaling to match 500x150 pixel UI
    obj->mainPosition.transform.scale = { 4.17f, 5.0f, 1.0f };
    
    obj->objectParts_[0].materialConfig->textureHandle = textureHandle;
    
    return obj;
  }

SimpleSprite *PromptBoard::CreateSprite(int textureHandle,
                                        const Vector3 &translate,
                                        const Vector2 &anchorPoint) {
  SimpleSprite *sprite = new SimpleSprite();
  sprite->IntObject(system_);
  sprite->CreateDefaultData();
  sprite->objectParts_[0].materialConfig->textureHandle = textureHandle;
  sprite->mainPosition.transform.translate = translate;
  sprite->objectParts_[0].anchorPoint = anchorPoint;
  return sprite;
}

void PromptBoard::SetPromptTexture(int textureHandle) {
  promptTextureHandle_ = textureHandle;
}

void PromptBoard::SetThemeTextures(const std::vector<int>& textureHandles) {
  if (!textureHandles.empty()) {
      dummyTextureHandles_ = textureHandles;
      Logger::Log("[PromptBoard] SetThemeTextures called. Handles: ");
      for (size_t i = 0; i < dummyTextureHandles_.size(); ++i) {
          Logger::Log("  [%zu] = %d", i, dummyTextureHandles_[i]);
      }
  }
}

void PromptBoard::StartStopAnimation() {
  if (isRolling_) {
    willStopOnNextFlap_ = true;
  }
}

void PromptBoard::Update(Camera* camera) {
  if (isRolling_) {
    UpdateRollingAnimation();
  } else if (isStopAnimation_) {
    UpdateStopAnimation();
  }
    if (!isShowingFinal_) {
        int currentTex = dummyTextureHandles_[currentDummyIndex_ % dummyTextureHandles_.size()];
        int nextTex = dummyTextureHandles_[(currentDummyIndex_ + 1) % dummyTextureHandles_.size()];
        
        if (isStopAnimation_) {
            nextTex = promptTextureHandle_;
        }
        
        UpdateFlapTextures(currentTex, nextTex);
    }
  
  UpdateSprites(camera);
}

void PromptBoard::UpdateRollingAnimation() {
  rollingFrameCounter_++;
  
  // Speed of the rolling animation
  fallingFlapAngle_ -= 0.3f;
    if (fallingFlapAngle_ <= -kPi) {
        fallingFlapAngle_ += kPi;
        currentDummyIndex_++;
        isFlapFallingUpper_ = true;
        
        if (willStopOnNextFlap_) {
            isRolling_ = false;
            isStopAnimation_ = true;
            stopAnimationCounter_ = 0;
            willStopOnNextFlap_ = false;
        }
    } else if (fallingFlapAngle_ <= -kPi / 2.0f && isFlapFallingUpper_) {
        isFlapFallingUpper_ = false;
    }
  
  ApplyFlapRotation(fallingFlapAngle_);
}

void PromptBoard::UpdateStopAnimation() {
  stopAnimationCounter_++;

  // Calculate speed based on how far the flap has fallen (progress from 0.0 to 1.0)
  // Starts at 0.3f and smoothly slows down to a minimum of 0.02f for a dramatic reveal
  float progress = fallingFlapAngle_ / -kPi;
  float speed = 0.3f * (1.0f - progress);
  if (speed < 0.03f) {
      speed = 0.03f;
  }
  
  fallingFlapAngle_ -= speed;
  
  if (fallingFlapAngle_ <= -kPi) {
      fallingFlapAngle_ = -kPi;
      isFlapFallingUpper_ = false;
      isStopAnimation_ = false;
      isStopAnimationFinished_ = true;
      isShowingFinal_ = true;
      
      // Stop exactly at the prompt texture
      UpdateFlapTextures(promptTextureHandle_, promptTextureHandle_);
  } else if (fallingFlapAngle_ <= -kPi / 2.0f && isFlapFallingUpper_) {
      isFlapFallingUpper_ = false;
  }
  
  ApplyFlapRotation(fallingFlapAngle_);
}

void PromptBoard::ApplyFlapRotation(float angle) {
  if (SceneManager::GetInstance().IsPause()) {
    return;
  }
  
  if (fallingFlapUpperObject_) {
      fallingFlapUpperObject_->mainPosition.transform.rotate.x = angle;
      fallingFlapUpperObject_->mainPosition.transform.scale.y = isFlapFallingUpper_ ? 5.0f : 0.0f;
  }
  if (fallingFlapLowerObject_) {
      fallingFlapLowerObject_->mainPosition.transform.rotate.x = angle + kPi;
      fallingFlapLowerObject_->mainPosition.transform.scale.y = (!isFlapFallingUpper_ && !isShowingFinal_) ? 5.0f : 0.0f;
  }
}

void PromptBoard::UpdateFlapTextures(int currentTex, int nextTex) {
    // Check if the textures are already correct to avoid massive memory leaks
    if (topFlapObject_ && topFlapObject_->objectParts_[0].materialConfig->textureHandle == nextTex &&
        bottomFlapObject_ && bottomFlapObject_->objectParts_[0].materialConfig->textureHandle == currentTex) {
        return;
    }

    static int lastCurrent = -1, lastNext = -1;
    if (lastCurrent != currentTex || lastNext != nextTex) {
        Logger::Log("[PromptBoard] Flap Update: currentTex=%d, nextTex=%d, promptTex=%d, index=%d", 
            currentTex, nextTex, promptTextureHandle_, currentDummyIndex_);
        lastCurrent = currentTex; lastNext = nextTex;
    }
    
    if (topFlapObject_) {
        auto newMat = std::make_shared<MaterialConfig>(*topFlapObject_->objectParts_[0].materialConfig);
        newMat->textureHandle = nextTex;
        topFlapObject_->objectParts_[0].materialConfig = newMat;
    }
    if (bottomFlapObject_) {
        auto newMat = std::make_shared<MaterialConfig>(*bottomFlapObject_->objectParts_[0].materialConfig);
        newMat->textureHandle = currentTex;
        bottomFlapObject_->objectParts_[0].materialConfig = newMat;
    }
    if (fallingFlapUpperObject_) {
        auto newMat = std::make_shared<MaterialConfig>(*fallingFlapUpperObject_->objectParts_[0].materialConfig);
        newMat->textureHandle = currentTex;
        fallingFlapUpperObject_->objectParts_[0].materialConfig = newMat;
    }
    if (fallingFlapLowerObject_) {
        auto newMat = std::make_shared<MaterialConfig>(*fallingFlapLowerObject_->objectParts_[0].materialConfig);
        newMat->textureHandle = nextTex;
        fallingFlapLowerObject_->objectParts_[0].materialConfig = newMat;
    }
}

void PromptBoard::SetSpriteAlpha(SimpleSprite *sprite, float alpha) {
  if (sprite && sprite->objectParts_[0].materialConfig) {
    sprite->objectParts_[0].materialConfig->textureColor.w = alpha;
  }
}

void PromptBoard::SetObjectAlpha(Object *object, float alpha) {
  if (object && object->objectParts_[0].materialConfig) {
    object->objectParts_[0].materialConfig->textureColor.w = alpha;
  }
}

void PromptBoard::UpdateSprites(Camera* camera) {
  if (topFlapObject_) {
    topFlapObject_->Update(camera);
  }
  if (bottomFlapObject_) {
    bottomFlapObject_->Update(camera);
  }
  if (fallingFlapUpperObject_) {
    fallingFlapUpperObject_->Update(camera);
  }
  if (fallingFlapLowerObject_) {
    fallingFlapLowerObject_->Update(camera);
  }
}

void PromptBoard::Draw() {
  if (isShowingFinal_) {
      if (topFlapObject_) topFlapObject_->objectParts_[0].materialConfig->textureHandle = promptTextureHandle_;
      if (bottomFlapObject_) bottomFlapObject_->objectParts_[0].materialConfig->textureHandle = promptTextureHandle_;
  }

  if (topFlapObject_ != nullptr) {
    system_->Draw3D(topFlapObject_);
  }
  if (bottomFlapObject_ != nullptr) {
    system_->Draw3D(bottomFlapObject_);
  }
  
  if (isFlapFallingUpper_ && fallingFlapUpperObject_ != nullptr) {
    system_->Draw3D(fallingFlapUpperObject_);
  } else if (!isFlapFallingUpper_ && !isShowingFinal_ && fallingFlapLowerObject_ != nullptr) {
    system_->Draw3D(fallingFlapLowerObject_);
  }
}