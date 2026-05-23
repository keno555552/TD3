#pragma once
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
  delete frameObject_;
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
  Vector3 basePos = {0.0f, 0.0f, 110.0f}; // 壁(Z=130)から少し手前に戻す

  int frameTex = system_->LoadTexture("GAME/resources/texture/white100x100.png");
  int frameModelHandle = system_->SetModelObj("GAME/resources/model/flap_frame.obj"); // ご用意いただいた枠モデル
  frameObject_ = CreateFlapObject(frameModelHandle, frameTex, basePos);
  
  // パネルのいっこ後ろ（奥）に配置
  frameObject_->mainPosition.transform.translate.z = basePos.z + 1.0f;
  
  // サイズを0.9倍に縮小
  frameObject_->mainPosition.transform.scale.x *= 0.9f;
  frameObject_->mainPosition.transform.scale.y *= 0.9f;
  frameObject_->mainPosition.transform.scale.z *= 0.9f;
  
  // 色をさらにもうちょっと黒っぽく（暗い紺色）にする (RGBA)
  frameObject_->objectParts_[0].materialConfig->textureColor = { 0.05f, 0.08f, 0.15f, 1.0f };

  topFlapObject_ = CreateFlapObject(topFlapModelHandle_, initialTex, basePos);
  bottomFlapObject_ = CreateFlapObject(bottomFlapModelHandle_, initialTex, basePos);
  
  fallingFlapUpperObject_ = CreateFlapObject(topFlapModelHandle_, initialTex, basePos);
  fallingFlapLowerObject_ = CreateFlapObject(bottomFlapModelHandle_, initialTex, basePos);

  isRolling_ = true;
  isStopAnimation_ = false;
  isStopAnimationFinished_ = false;
  isShowingFinal_ = false;

  isBounceAnimation_ = false;
  bounceAnimationCounter_ = 0.0f;
  hasSpawnedParticle_ = false;
  
  if (!confettiParticle_) {
      confettiParticle_ = std::make_unique<ConfettiParticle>(system_);
  } else {
      confettiParticle_->ClearAll();
  }

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

  if (isBounceAnimation_) {
    bounceAnimationCounter_ += 0.12f; // アニメーション速度
    if (bounceAnimationCounter_ >= kPi) {
        bounceAnimationCounter_ = 0.0f;
        isBounceAnimation_ = false;
        hasSpawnedParticle_ = false;
        
        // スケールと色と回転を元に戻す
        if (topFlapObject_) {
            topFlapObject_->mainPosition.transform.scale = { 4.17f, 5.0f, 1.0f };
            topFlapObject_->objectParts_[0].materialConfig->textureColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            topFlapObject_->mainPosition.transform.rotate.y = 0.0f;
            topFlapObject_->mainPosition.transform.rotate.z = 0.0f;
        }
        if (bottomFlapObject_) {
            bottomFlapObject_->mainPosition.transform.scale = { 4.17f, 5.0f, 1.0f };
            bottomFlapObject_->objectParts_[0].materialConfig->textureColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            bottomFlapObject_->mainPosition.transform.rotate.y = 0.0f;
            bottomFlapObject_->mainPosition.transform.rotate.z = 0.0f;
        }
    } else {
        // sin関数を使ってスケールをバウンス (最大1.3倍)
        float bounce = std::sin(bounceAnimationCounter_);
        float scaleMultiplier = 1.0f + bounce * 0.3f;
        
        // --- ここから演出の切り替え ---
        // 以下の「パターン1」と「パターン2」のうち、使いたい方のコメントを外して試してください。

        // パターン1：ブルブル震えて光る演出（パーティクルなし）
        //float rotationZ = std::sin(bounceAnimationCounter_ * 20.0f) * 0.1f;
        //float rotationY = std::cos(bounceAnimationCounter_ * 25.0f) * 0.15f;

        // パターン2：ちょっと傾いて飛び出してきて、紙吹雪パーティクルを出す演出
        
        float rotationZ = 0.0f; // 斜めにならないように変更
        float rotationY = 0.0f;
        if (!hasSpawnedParticle_) {
            if (confettiParticle_) {
                // UIパネルの中央から発生させる
                confettiParticle_->Spawn({0.0f, 0.0f, 95.0f});
            }
            hasSpawnedParticle_ = true;
        }
        
        // --- 演出切り替えここまで ---

        // 色をピカッとゴールドに光らせる
        float colorR = 1.0f + bounce * 1.5f; 
        float colorG = 1.0f + bounce * 1.0f; 
        float colorB = 1.0f - bounce * 0.5f; 

        if (topFlapObject_) {
            topFlapObject_->mainPosition.transform.scale = { 4.17f * scaleMultiplier, 5.0f * scaleMultiplier, 1.0f };
            topFlapObject_->objectParts_[0].materialConfig->textureColor = { colorR, colorG, colorB, 1.0f };
            topFlapObject_->mainPosition.transform.rotate.y = rotationY;
            topFlapObject_->mainPosition.transform.rotate.z = rotationZ;
        }
        if (bottomFlapObject_) {
            bottomFlapObject_->mainPosition.transform.scale = { 4.17f * scaleMultiplier, 5.0f * scaleMultiplier, 1.0f };
            bottomFlapObject_->objectParts_[0].materialConfig->textureColor = { colorR, colorG, colorB, 1.0f };
            bottomFlapObject_->mainPosition.transform.rotate.y = rotationY;
            bottomFlapObject_->mainPosition.transform.rotate.z = rotationZ;
        }
    }
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
      
      isBounceAnimation_ = true;
      bounceAnimationCounter_ = 0.0f;
      
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
  
  if (confettiParticle_) {
      confettiParticle_->Update(camera);
  }

  if (frameObject_) {
      frameObject_->Update(camera);
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
  
  if (confettiParticle_) {
      confettiParticle_->Draw();
  }

  if (frameObject_) {
      system_->Draw3D(frameObject_);
  }
}