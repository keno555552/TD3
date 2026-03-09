#include "TravelScene.h"

namespace {
/*    enum class を配列添字に変換するための補助関数   */
size_t ToIndex(ModBodyPart part) { return static_cast<size_t>(part); }
} // namespace

TravelScene::TravelScene(kEngine *system) {
  system_ = system;

  //===============================
  // ライト
  //===============================
  light1_ = new Light;
  light1_->direction = {-0.5f, -1.0f, -0.3f};
  light1_->color = {1.0f, 1.0f, 1.0f};
  light1_->intensity = 1.0f;
  system_->AddLight(light1_);

  //===============================
  // カメラ
  //===============================
  debugCamera_ = system_->CreateDebugCamera();
  camera_ = system_->CreateCamera();

  // デバッグカメラ初期位置
  debugCamera_->SetTranslate({0.0f, 0.0f, -18.0f});
  debugCamera_->SetDefaultTransform(debugCamera_->GetTransform());

  // 通常カメラ初期位置
  camera_->SetTranslate({0.0f, 0.0f, -18.0f});
  camera_->SetDefaultTransform(camera_->GetTransform());

  usingCamera_ = debugCamera_;
  system_->SetCamera(usingCamera_);

  //===============================
  // 3Dオブジェクト
  //===============================

  // 各部位のハンドルとオブジェクト配列を初期化
  modModelHandles_.fill(0);
  modObjects_.fill(nullptr);

  // 改造用の各部位オブジェクトをセットアップ
  SetupModObjects();

  //===============================
  // 2D
  //===============================
  fade_.Initialize(system_);
  fade_.StartFadeIn();
}

TravelScene::~TravelScene() {
  system_->DestroyCamera(camera_);
  system_->DestroyCamera(debugCamera_);

  // 各部位オブジェクトを解放
  for (auto &object : modObjects_) {
    delete object;
    object = nullptr;
  }

  system_->RemoveLight(light1_);

  delete light1_;
}

void TravelScene::Update() {
  CameraPart();

  //===============================
  // プレイヤー移動
  //===============================
  if (system_->GetTriggerOn(DIK_D)) {
    float power = rightStepPower_;

    if (!isFirstStep_) {
      if (!lastStepLeft_) {
        power *= sameSidePenalty_;
      } else {
        power *= alternateBonus_;
      }
    }

    velocityX_ += power;
    rightLegSwing_ = legSwingAngle_;

    isFirstStep_ = false;
    lastStepLeft_ = false;
  } else if (system_->GetTriggerOn(DIK_A)) {
    float power = leftStepPower_;

    if (!isFirstStep_) {
      if (lastStepLeft_) {
        power *= sameSidePenalty_;
      } else {
        power *= alternateBonus_;
      }
    }

    velocityX_ += power;
    leftLegSwing_ = legSwingAngle_;

    isFirstStep_ = false;
    lastStepLeft_ = true;
  }

  // 速度に変換
  velocityX_ *= speedDamping_;

  moveX_ += velocityX_;

  // 移動
  modObjects_[ToIndex(ModBodyPart::Body)]->mainPosition.transform.translate.x =
      moveX_;

  leftLegSwing_ *= legSwingDamping_;
  rightLegSwing_ *= legSwingDamping_;

  // 見た目に反映
  modObjects_[ToIndex(ModBodyPart::LeftLeg)]->mainPosition.transform.rotate.x =
      leftLegSwing_;
  modObjects_[ToIndex(ModBodyPart::RightLeg)]->mainPosition.transform.rotate.x =
      rightLegSwing_;
  modObjects_[ToIndex(ModBodyPart::LeftArm)]->mainPosition.transform.rotate.x =
      -rightLegSwing_;
  modObjects_[ToIndex(ModBodyPart::RightArm)]->mainPosition.transform.rotate.x =
      -leftLegSwing_;

  // 各部位オブジェクトを更新
  UpdateModObjects();

  // スペースキーで評価シーンへ
  if (!fade_.IsBusy() && system_->GetTriggerOn(DIK_SPACE)) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
    nextOutcome_ = SceneOutcome::NEXT;
  }

  // リトライ
  if (!fade_.IsBusy() && system_->GetTriggerOn(DIK_RETURN)) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
    nextOutcome_ = SceneOutcome::RETRY;
  }

  // フェード更新
  fade_.Update(usingCamera_);

  // フェード終了後にシーン移行
  if (isStartTransition_ && fade_.IsFinished()) {
    outcome_ = nextOutcome_;
  }
}

void TravelScene::Draw() {

  DrawModObjects();

#ifdef USE_IMGUI
  // 現在シーン表示
  ImGui::Begin("Scene");
  ImGui::Text("TravelScene");
  ImGui::End();
#endif

  // フェード描画
  fade_.Draw();
}

void TravelScene::CameraPart() {
  if (useDebugCamera_) {
    usingCamera_ = debugCamera_;
    debugCamera_->MouseControlUpdate();
  } else {
    usingCamera_ = camera_;
  }

  system_->SetCamera(usingCamera_);
}

void TravelScene::SetupModObjects() {
  SetupPartObject(ModBodyPart::Body, "GAME/resources/modBody/body/body.obj");
  SetupPartObject(ModBodyPart::Head, "GAME/resources/modBody/head/head.obj");
  SetupPartObject(ModBodyPart::LeftArm,
                  "GAME/resources/modBody/leftArm/leftArm.obj");
  SetupPartObject(ModBodyPart::RightArm,
                  "GAME/resources/modBody/rightArm/rightArm.obj");
  SetupPartObject(ModBodyPart::LeftLeg,
                  "GAME/resources/modBody/leftLeg/leftLeg.obj");
  SetupPartObject(ModBodyPart::RightLeg,
                  "GAME/resources/modBody/rightLeg/rightLeg.obj");
  // 親子関係設定
  SetupHierarchy();

  // 初期配置設定
  SetupInitialLayout();
}

/*   指定した部位のObjectを1つ生成する   */
void TravelScene::SetupPartObject(ModBodyPart part, const std::string &path) {
  size_t index = ToIndex(part);

  // モデル読み込み
  modModelHandles_[index] = system_->SetModelObj(path);

  // Object生成
  modObjects_[index] = new Object;
  modObjects_[index]->IntObject(system_);
  modObjects_[index]->CreateModelData(modModelHandles_[index]);

  // 初期Transform設定
  modObjects_[index]->mainPosition.transform = CreateDefaultTransform();
}

/*   部位同士の親子関係を設定する   */
void TravelScene::SetupHierarchy() {
  Object *body = modObjects_[ToIndex(ModBodyPart::Body)];
  if (body == nullptr) {
    return;
  }

  // BodyのmainPositionを親として使う
  ObjectPart *bodyRoot = &body->mainPosition;

  modObjects_[ToIndex(ModBodyPart::Head)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::LeftArm)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::RightArm)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::LeftLeg)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::RightLeg)]->followObject_ = bodyRoot;
}

/*   各部位の初期配置を設定する   */
void TravelScene::SetupInitialLayout() {
  Object *body = modObjects_[ToIndex(ModBodyPart::Body)];
  Object *head = modObjects_[ToIndex(ModBodyPart::Head)];
  Object *leftArm = modObjects_[ToIndex(ModBodyPart::LeftArm)];
  Object *rightArm = modObjects_[ToIndex(ModBodyPart::RightArm)];
  Object *leftLeg = modObjects_[ToIndex(ModBodyPart::LeftLeg)];
  Object *rightLeg = modObjects_[ToIndex(ModBodyPart::RightLeg)];

  // 胴体を基準位置へ配置
  body->mainPosition.transform.translate = {0.0f, 0.0f, 0.0f};
  body->mainPosition.transform.rotate = {0.0f, 1.5f, 0.0f};

  // 頭を胴体の上に配置
  head->mainPosition.transform.translate = {0.0f, 1.5f, 0.0f};

  // 腕を左右に配置
  leftArm->mainPosition.transform.translate = {-1.25f, 0.5f, 0.0f};
  rightArm->mainPosition.transform.translate = {1.25f, 0.5f, 0.0f};

  // 脚を下に配置
  leftLeg->mainPosition.transform.translate = {-0.5f, -1.75f, 0.0f};
  rightLeg->mainPosition.transform.translate = {0.5f, -1.75f, 0.0f};
}

/*   各部位Objectの更新をまとめて行う   */
void TravelScene::UpdateModObjects() {
  for (auto &object : modObjects_) {
    if (object != nullptr) {
      object->Update(usingCamera_);
    }
  }
}

void TravelScene::DrawModObjects() {
  for (auto &object : modObjects_) {
    if (object != nullptr) {
      object->Draw();
    }
  }
}
