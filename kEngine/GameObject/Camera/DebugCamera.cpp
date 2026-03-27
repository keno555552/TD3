#include "DebugCamera.h"
#include "kEngine.h"

DebugCamera::DebugCamera(kEngine *system) { system_ = system; }

void DebugCamera::Update() { Camera::Update(); }

void DebugCamera::MouseControlUpdate() {
  bool isMoved = false;

  float deltaTime = system_->GetDeltaTime();

  float scale = deltaTime * 1;

  /// keyboard移動
  Transform cameraTransform = CreateDefaultTransform();
  if (system_->GetIsPush(DIK_Q) && isQ_) {
    cameraTransform.translate.z += 0.01f;
    isMoved = true;
  }
  if (system_->GetIsPush(DIK_E) && isE_) {
    cameraTransform.translate.z -= 0.01f;
    isMoved = true;
  }
  if (system_->GetIsPush(DIK_A) && isA_) {
    cameraTransform.translate.x -= 0.01f;
    isMoved = true;
  }
  if (system_->GetIsPush(DIK_D) && isD_) {
    cameraTransform.translate.x += 0.01f;
    isMoved = true;
  }
  if (system_->GetIsPush(DIK_W) && isW_) {
    cameraTransform.translate.y += 0.01f;
    isMoved = true;
  }
  if (system_->GetIsPush(DIK_S) && isS_) {
    cameraTransform.translate.y -= 0.01f;
    isMoved = true;
  }
  if (system_->GetIsPush(DIK_Z) && isZ_) {
    cameraTransform.rotate.y += 0.01f;
    isMoved = true;
  }
  if (system_->GetIsPush(DIK_C) && isC_) {
    cameraTransform.rotate.y -= 0.01f;
    isMoved = true;
  }

  /// マウスホイール移動
  float before = cameraTransform.translate.z;
  if (before != cameraTransform.translate.z) {
    isMoved = true;
  }

  /// マウス操作
  // 中鍵平行移動
  if (system_->GetMouseIsPush(2) && isMouseM_) {
    cameraTransform.translate.x += (float)system_->GetMousePosXIns() * -0.005f;
    cameraTransform.translate.y += (float)system_->GetMousePosYIns() * 0.005f;
    cameraTransform.translate.z += system_->GetMouseScroll();
    isMoved = true;
  }
  // 右鍵視覚移動
  if (system_->GetMouseIsPush(1) && isMouseR_) {
    {
      cameraTransform.rotate.x = ((float)system_->GetMousePosYIns() * scale);
      cameraTransform.rotate.y = ((float)system_->GetMousePosXIns() * scale);
    }
    isMoved = true;
  }

  /// GamePad操作
  if (config::GetKeyboardState() && isGamePad_) {
    if (system_->GetGamepadLStick01X() != 0) {
      cameraTransform.translate.x += system_->GetGamepadLStick01X() / 50.0f;
      isMoved = true;
    }
    if (system_->GetGamepadLStick01Y() != 0) {
      cameraTransform.translate.z += system_->GetGamepadLStick01Y() / 50.0f;
      isMoved = true;
    }
    if (system_->GetGamepadIsPush(VK_PAD_LSHOULDER) != 0) {
      cameraTransform.translate.y += 0.01f;
      isMoved = true;
    }
    if (system_->GetGamepadIsPush(VK_PAD_RSHOULDER) != 0) {
      cameraTransform.translate.y -= 0.01f;
      isMoved = true;
    }
    if (system_->GetGamepadRStick01Y() != 0) {
      cameraTransform.rotate.x -= system_->GetGamepadRStick01Y() / 100.0f;
      isMoved = true;
    }
    if (system_->GetGamepadRStick01X() != 0) {
      cameraTransform.rotate.y += system_->GetGamepadRStick01X() / 100.0f;
      isMoved = true;
    }
  }

  /// 元の位置に戻る
  if (system_->GetIsPush(DIK_R) && isR_) {
    ResetCamera();
    isMoved = true;
  }
  if (system_->GetGamepadIsPush(VK_PAD_BACK) && isGamePad_) {
    ResetCamera();
    isMoved = true;
  }

  if (isMoved) {
    Matrix4x4 handle = MakeTranslateMatrix4x4(cameraTransform.translate) *
                       MakeRotateMatrix4x4(cameraTransform_.rotate);
    cameraTransform_.translate =
        cameraTransform_.translate + MakeTranslateVector3(handle);
    cameraTransform_.rotate = cameraTransform_.rotate + cameraTransform.rotate;
    dirty_ = true;
  }
}

void DebugCamera::isKeyAvailable(bool available) {
  isQ_ = available;
  isE_ = available;
  isA_ = available;
  isD_ = available;
  isW_ = available;
  isS_ = available;
  isZ_ = available;
  isC_ = available;
  isR_ = available;
  isMouseM_ = available;
  isMouseR_ = available;
}
