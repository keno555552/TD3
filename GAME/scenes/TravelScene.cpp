#include "TravelScene.h"
#include <cmath>

namespace {
/*    enum class を配列添字に変換するための補助関数   */
size_t ToIndex(ModBodyPart part) { return static_cast<size_t>(part); }

/*   成分ごとのスケール適用   */
Vector3 ScaleByRatio(const Vector3 &base, const Vector3 &ratio) {
  return {base.x * ratio.x, base.y * ratio.y, base.z * ratio.z};
}

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
  debugCamera_->SetTranslate({0.0f, 0.0f, -48.0f});
  debugCamera_->SetDefaultTransform(debugCamera_->GetTransform());

  // 通常カメラ初期位置
  camera_->SetTranslate({0.0f, 0.0f, -48.0f});
  camera_->SetDefaultTransform(camera_->GetTransform());

  usingCamera_ = camera_;
  system_->SetCamera(usingCamera_);

  //===============================
  // 3Dオブジェクト
  //===============================

  // 各部位のハンドルとオブジェクト配列を初期化
  modModelHandles_.fill(0);
  modObjects_.fill(nullptr);

  // 改造用の各部位オブジェクトをセットアップ
  SetupModObjects();

  customizeData_ = ModBody::CopySharedCustomizeData();
  if (customizeData_ == nullptr) {
    customizeData_ = ModBody::CreateDefaultCustomizeData();
  }

  LoadCustomizeData();

  BuildFeaturesFromCustomizeData();

  BuildExtraVisualParts();

  ApplyCustomizeToMovementParam();

  // UpdateChildRootsFromBody();

  leftLegBend_ = legRecoverAngle_;
  rightLegBend_ = legRecoverAngle_;

  leftLegPrevBend_ = leftLegBend_;
  rightLegPrevBend_ = rightLegBend_;

  leftLegBendSpeed_ = 0.0f;
  rightLegBendSpeed_ = 0.0f;
  leftLegPrevBendSpeed_ = 0.0f;
  rightLegPrevBendSpeed_ = 0.0f;

  bodyTilt_ = 0.0f;
  bodyTiltVelocity_ = 0.0f;

  leftDriveAccum_ = 0.0f;
  rightDriveAccum_ = 0.0f;
  leftHoldTime_ = 0.0f;
  rightHoldTime_ = 0.0f;
  lastKickSide_ = 0;

  moveX_ = -18.0f;

  // bodyで代用中
  groundModelHandle_ =
      system_->SetModelObj("GAME/resources/modBody/body/body.obj");

  ground_ = std::make_unique<Object>();
  ground_->IntObject(system_);
  ground_->CreateModelData(groundModelHandle_);
  ground_->mainPosition.transform = CreateDefaultTransform();

  ground_->mainPosition.transform.translate = {0.0f, groundY_ - 3.5f, 0.0f};
  ground_->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  ground_->mainPosition.transform.scale = {50.0f, 0.3f, 10.0f};

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

  ClearExtraVisualParts();

  system_->RemoveLight(light1_);

  delete light1_;
}

void TravelScene::Update() {

  //===============================
  // カメラ更新
  //===============================

  CameraPart();

  //===============================
  // プレイヤー移動
  //===============================

  if (!HasRequiredParts()) {
    return;
  }

  //--------------------------------
  // 入力：押すと蹴り出す、離すと回収する
  //--------------------------------
  bool leftNowInput = system_->GetIsPush(DIK_A);
  bool rightNowInput = system_->GetIsPush(DIK_D);

  const float deltaTime = system_->GetDeltaTime();

  SavePreviousFrameState();

  UpdateTimeLimit(deltaTime);

  UpdateHoldState(leftNowInput, rightNowInput, deltaTime);

  UpdateLegBendState(leftNowInput, rightNowInput);

  UpdateMovementState(leftNowInput, rightNowInput);

  ApplyVisualState();

  UpdateSceneTransition();

  const ModControlPointData *cp = GetControlPoints();
  if (cp != nullptr) {
    // Logger::Log("RECV LeftShoulderPos : %.2f %.2f %.2f\n",
    //             cp->leftShoulderPos.x, cp->leftShoulderPos.y,
    //             cp->leftShoulderPos.z);
    // Logger::Log("RECV LeftElbowPos : %.2f %.2f %.2f\n", cp->leftElbowPos.x,
    //             cp->leftElbowPos.y, cp->leftElbowPos.z);
    // Logger::Log("RECV LeftWristPos : %.2f %.2f %.2f\n", cp->leftWristPos.x,
    //             cp->leftWristPos.y, cp->leftWristPos.z);

    // Logger::Log("RECV RightShoulderPos : %.2f %.2f %.2f\n",
    //             cp->rightShoulderPos.x, cp->rightShoulderPos.y,
    //             cp->rightShoulderPos.z);
    // Logger::Log("RECV RightElbowPos : %.2f %.2f %.2f\n", cp->rightElbowPos.x,
    //             cp->rightElbowPos.y, cp->rightElbowPos.z);
    // Logger::Log("RECV RightWristPos : %.2f %.2f %.2f\n", cp->rightWristPos.x,
    //             cp->rightWristPos.y, cp->rightWristPos.z);

    // Logger::Log("RECV LeftHipPos : %.2f %.2f %.2f\n", cp->leftHipPos.x,
    //             cp->leftHipPos.y, cp->leftHipPos.z);
    // Logger::Log("RECV LeftKneePos : %.2f %.2f %.2f\n", cp->leftKneePos.x,
    //             cp->leftKneePos.y, cp->leftKneePos.z);
    // Logger::Log("RECV LeftAnklePos : %.2f %.2f %.2f\n", cp->leftAnklePos.x,
    //             cp->leftAnklePos.y, cp->leftAnklePos.z);

    // Logger::Log("RECV RightHipPos : %.2f %.2f %.2f\n", cp->rightHipPos.x,
    //             cp->rightHipPos.y, cp->rightHipPos.z);
    // Logger::Log("RECV RightKneePos : %.2f %.2f %.2f\n", cp->rightKneePos.x,
    //             cp->rightKneePos.y, cp->rightKneePos.z);
    // Logger::Log("RECV RightAnklePos : %.2f %.2f %.2f\n", cp->rightAnklePos.x,
    //             cp->rightAnklePos.y, cp->rightAnklePos.z);

    // Logger::Log("RECV ChestPos : %.2f %.2f %.2f\n", cp->chestPos.x,
    //             cp->chestPos.y, cp->chestPos.z);
    // Logger::Log("RECV BellyPos : %.2f %.2f %.2f\n", cp->bellyPos.x,
    //             cp->bellyPos.y, cp->bellyPos.z);
    // Logger::Log("RECV WaistPos : %.2f %.2f %.2f\n", cp->waistPos.x,
    //             cp->waistPos.y, cp->waistPos.z);

    // Logger::Log("RECV LowerNeckPos : %.2f %.2f %.2f\n", cp->lowerNeckPos.x,
    //             cp->lowerNeckPos.y, cp->lowerNeckPos.z);
    // Logger::Log("RECV UpperNeckPos : %.2f %.2f %.2f\n", cp->upperNeckPos.x,
    //             cp->upperNeckPos.y, cp->upperNeckPos.z);
    // Logger::Log("RECV HeadCenterPos : %.2f %.2f %.2f\n", cp->headCenterPos.x,
    //             cp->headCenterPos.y, cp->headCenterPos.z);
  }
}

void TravelScene::Draw() {

  DrawModObjects();

  DrawExtraVisualParts();

  ground_->Draw();

#ifdef USE_IMGUI
  // 現在シーン表示
  ImGui::Begin("Scene");
  ImGui::Text("TravelScene");
  ImGui::End();
#endif

#ifdef USE_IMGUI
  ImGui::Begin("TravelDebug");

  //==============================
  // 位置・速度
  //==============================
  ImGui::Text("MoveX : %.3f", moveX_);
  ImGui::Text("MoveY : %.3f", moveY_);
  ImGui::Text("VelocityX : %.3f", velocityX_);
  ImGui::Text("VelocityY : %.3f", velocityY_);

  //==============================
  // 姿勢確認
  //==============================
  float legDiffTilt = (leftLegBend_ - rightLegBend_) * legDiffTiltPower_;

  float postureError = std::abs(bodyTilt_ - idealRunTilt_);
  float badPosture = std::clamp(postureError / postureTolerance_, 0.0f, 1.0f);
  float forwardRate = 1.0f - badPosture;
  float upwardRate = 0.30f + badPosture * 0.70f;

  ImGui::Separator();
  ImGui::Text("BodyTilt : %.4f", bodyTilt_);
  ImGui::Text("LegDiffTilt : %.4f", legDiffTilt);

  ImGui::Text("LeftLegBend : %.4f", leftLegBend_);
  ImGui::Text("RightLegBend : %.4f", rightLegBend_);

  ImGui::Text("ForwardRate : %.4f", forwardRate);
  ImGui::Text("UpwardRate : %.4f", upwardRate);

  ImGui::Text("LeftDriveAccum : %.4f", leftDriveAccum_);
  ImGui::Text("RightDriveAccum : %.4f", rightDriveAccum_);

  ImGui::Text("LeftHoldTime : %.4f", leftHoldTime_);
  ImGui::Text("RightHoldTime : %.4f", rightHoldTime_);

  ImGui::Text("BodyTiltVelocity : %.4f", bodyTiltVelocity_);
  ImGui::Text("LeftLegBendSpeed : %.4f", leftLegBendSpeed_);
  ImGui::Text("RightLegBendSpeed : %.4f", rightLegBendSpeed_);

  ImGui::Checkbox("Force Tilt", &debugForceTilt_);
  ImGui::SliderFloat("Tilt Value", &debugTiltValue_, -0.4f, 0.4f);

  ImGui::Checkbox("Use Customize Move", &useCustomizeMove_);
  ImGui::Text("runPower: %.2f", tuning_.runPower);
  ImGui::Text("lift: %.2f", tuning_.lift);
  ImGui::Text("maxSpeed: %.2f", tuning_.maxSpeed);
  ImGui::Text("stability: %.2f", tuning_.stability);
  ImGui::Text("bodyTilt: %.4f", bodyTilt_);
  ImGui::Text("turnResponse: %.2f", tuning_.turnResponse);

  ImGui::End();
#endif

#ifdef USE_IMGUI
  ImGui::Begin("Time");

  ImGui::Text("Remaining Time : %.2f", timeLimit_);
  ImGui::Text("Time Up : %s", isTimeUp_ ? "YES" : "NO");

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
  SetupPartObject(ModBodyPart::Neck, "GAME/resources/modBody/neck/neck.obj");
  SetupPartObject(ModBodyPart::Head, "GAME/resources/modBody/head/head.obj");

  SetupPartObject(ModBodyPart::LeftUpperArm,
                  "GAME/resources/modBody/leftUpperArm/leftUpperArm.obj");
  SetupPartObject(ModBodyPart::LeftForeArm,
                  "GAME/resources/modBody/leftForeArm/leftForeArm.obj");
  SetupPartObject(ModBodyPart::RightUpperArm,
                  "GAME/resources/modBody/rightUpperArm/rightUpperArm.obj");
  SetupPartObject(ModBodyPart::RightForeArm,
                  "GAME/resources/modBody/rightForeArm/rightForeArm.obj");

  SetupPartObject(ModBodyPart::LeftThigh,
                  "GAME/resources/modBody/leftThighs/leftThighs.obj");
  SetupPartObject(ModBodyPart::LeftShin,
                  "GAME/resources/modBody/leftShin/leftShin.obj");
  SetupPartObject(ModBodyPart::RightThigh,
                  "GAME/resources/modBody/rightThighs/rightThighs.obj");
  SetupPartObject(ModBodyPart::RightShin,
                  "GAME/resources/modBody/rightShin/rightShin.obj");

  SetupHierarchy();
  // SetupInitialLayout();
  SetupBodyJointOffsets();

  fixedPartIdToPart_.clear();
  fixedPartIdToPart_[1] =
      &modObjects_[ToIndex(ModBodyPart::Body)]->mainPosition;
  fixedPartIdToPart_[2] =
      &modObjects_[ToIndex(ModBodyPart::Neck)]->mainPosition;
  fixedPartIdToPart_[3] =
      &modObjects_[ToIndex(ModBodyPart::Head)]->mainPosition;
  fixedPartIdToPart_[4] =
      &modObjects_[ToIndex(ModBodyPart::LeftUpperArm)]->mainPosition;
  fixedPartIdToPart_[5] =
      &modObjects_[ToIndex(ModBodyPart::LeftForeArm)]->mainPosition;
  fixedPartIdToPart_[6] =
      &modObjects_[ToIndex(ModBodyPart::RightUpperArm)]->mainPosition;
  fixedPartIdToPart_[7] =
      &modObjects_[ToIndex(ModBodyPart::RightForeArm)]->mainPosition;
  fixedPartIdToPart_[8] =
      &modObjects_[ToIndex(ModBodyPart::LeftThigh)]->mainPosition;
  fixedPartIdToPart_[9] =
      &modObjects_[ToIndex(ModBodyPart::LeftShin)]->mainPosition;
  fixedPartIdToPart_[10] =
      &modObjects_[ToIndex(ModBodyPart::RightThigh)]->mainPosition;
  fixedPartIdToPart_[11] =
      &modObjects_[ToIndex(ModBodyPart::RightShin)]->mainPosition;
}

/*   指定した部位のObjectを1つ生成する   */
void TravelScene::SetupPartObject(ModBodyPart part, const std::string &path) {
  const size_t index = ToIndex(part);

  modModelHandles_[index] = system_->SetModelObj(path);

  modObjects_[index] = new Object;
  modObjects_[index]->IntObject(system_);
  modObjects_[index]->CreateModelData(modModelHandles_[index]);
  modObjects_[index]->mainPosition.transform = CreateDefaultTransform();

  modBodies_[index].Initialize(modObjects_[index], part);
}

/*   部位同士の親子関係を設定する   */
void TravelScene::SetupHierarchy() {
  Object *body = modObjects_[ToIndex(ModBodyPart::Body)];
  if (body == nullptr) {
    return;
  }

  ObjectPart *bodyRoot = &body->mainPosition;

  // 1段目
  modObjects_[ToIndex(ModBodyPart::Neck)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::Neck)]->mainPosition.parentPart = bodyRoot;

  // modObjects_[ToIndex(ModBodyPart::Head)]->followObject_ = bodyRoot;
  // modObjects_[ToIndex(ModBodyPart::Head)]->mainPosition.parentPart =
  // bodyRoot;

  modObjects_[ToIndex(ModBodyPart::Head)]->followObject_ =
      &modObjects_[ToIndex(ModBodyPart::Neck)]->mainPosition;
  modObjects_[ToIndex(ModBodyPart::Head)]->mainPosition.parentPart =
      &modObjects_[ToIndex(ModBodyPart::Neck)]->mainPosition;

  modObjects_[ToIndex(ModBodyPart::LeftUpperArm)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::LeftUpperArm)]->mainPosition.parentPart =
      bodyRoot;

  modObjects_[ToIndex(ModBodyPart::RightUpperArm)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::RightUpperArm)]->mainPosition.parentPart =
      bodyRoot;

  modObjects_[ToIndex(ModBodyPart::LeftThigh)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::LeftThigh)]->mainPosition.parentPart =
      bodyRoot;

  modObjects_[ToIndex(ModBodyPart::RightThigh)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::RightThigh)]->mainPosition.parentPart =
      bodyRoot;

  //============================
  // 二段目
  //============================

  // 左前腕
  // modObjects_[ToIndex(ModBodyPart::LeftForeArm)]->followObject_ =
  //    &modObjects_[ToIndex(ModBodyPart::LeftUpperArm)]->mainPosition;
  // modObjects_[ToIndex(ModBodyPart::LeftForeArm)]->mainPosition.parentPart =
  //    &modObjects_[ToIndex(ModBodyPart::LeftUpperArm)]->mainPosition;
  modObjects_[ToIndex(ModBodyPart::LeftForeArm)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::LeftForeArm)]->mainPosition.parentPart =
      bodyRoot;

  // 右前腕
  //  modObjects_[ToIndex(ModBodyPart::RightForeArm)]->followObject_ =
  //      &modObjects_[ToIndex(ModBodyPart::RightUpperArm)]->mainPosition;
  //  modObjects_[ToIndex(ModBodyPart::RightForeArm)]->mainPosition.parentPart =
  //      &modObjects_[ToIndex(ModBodyPart::RightUpperArm)]->mainPosition;
  modObjects_[ToIndex(ModBodyPart::RightForeArm)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::RightForeArm)]->mainPosition.parentPart =
      bodyRoot;

  // 左脛
  // modObjects_[ToIndex(ModBodyPart::LeftShin)]->followObject_ =
  //    &modObjects_[ToIndex(ModBodyPart::LeftThigh)]->mainPosition;
  // modObjects_[ToIndex(ModBodyPart::LeftShin)]->mainPosition.parentPart =
  //    &modObjects_[ToIndex(ModBodyPart::LeftThigh)]->mainPosition;
  modObjects_[ToIndex(ModBodyPart::LeftShin)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::LeftShin)]->mainPosition.parentPart =
      bodyRoot;

  // 右脛
  // modObjects_[ToIndex(ModBodyPart::RightShin)]->followObject_ =
  //    &modObjects_[ToIndex(ModBodyPart::RightThigh)]->mainPosition;
  // modObjects_[ToIndex(ModBodyPart::RightShin)]->mainPosition.parentPart =
  //    &modObjects_[ToIndex(ModBodyPart::RightThigh)]->mainPosition;
  modObjects_[ToIndex(ModBodyPart::RightShin)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::RightShin)]->mainPosition.parentPart =
      bodyRoot;
}

/*   各部位の初期配置を設定する   */
void TravelScene::SetupInitialLayout() {
  Object *body = modObjects_[ToIndex(ModBodyPart::Body)];
  Object *neck = modObjects_[ToIndex(ModBodyPart::Neck)];
  Object *head = modObjects_[ToIndex(ModBodyPart::Head)];

  Object *leftUpperArm = modObjects_[ToIndex(ModBodyPart::LeftUpperArm)];
  Object *leftForeArm = modObjects_[ToIndex(ModBodyPart::LeftForeArm)];
  Object *rightUpperArm = modObjects_[ToIndex(ModBodyPart::RightUpperArm)];
  Object *rightForeArm = modObjects_[ToIndex(ModBodyPart::RightForeArm)];

  Object *leftThigh = modObjects_[ToIndex(ModBodyPart::LeftThigh)];
  Object *leftShin = modObjects_[ToIndex(ModBodyPart::LeftShin)];
  Object *rightThigh = modObjects_[ToIndex(ModBodyPart::RightThigh)];
  Object *rightShin = modObjects_[ToIndex(ModBodyPart::RightShin)];

  const Vector3 baseRotate = {0.0f, 0.0f, 0.0f};

  body->mainPosition.transform.translate = {0.0f, 0.0f, 0.0f};
  body->mainPosition.transform.rotate = baseRotate;

  neck->mainPosition.transform.translate = {0.0f, 1.0f, 0.0f};
  neck->mainPosition.transform.rotate = baseRotate;

  head->mainPosition.transform.translate = {0.0f, 1.0f, 0.0f};
  head->mainPosition.transform.rotate = baseRotate;

  leftUpperArm->mainPosition.transform.translate = {-1.25f, 1.0f, 0.0f};
  leftUpperArm->mainPosition.transform.rotate = baseRotate;

  leftForeArm->mainPosition.transform.translate = {0.0f, -1.0f, 0.0f};
  leftForeArm->mainPosition.transform.rotate = baseRotate;

  rightUpperArm->mainPosition.transform.translate = {1.25f, 1.0f, 0.0f};
  rightUpperArm->mainPosition.transform.rotate = baseRotate;

  rightForeArm->mainPosition.transform.translate = {0.0f, -1.0f, 0.0f};
  rightForeArm->mainPosition.transform.rotate = baseRotate;

  leftThigh->mainPosition.transform.translate = {-0.5f, -1.25f, 0.0f};
  leftThigh->mainPosition.transform.rotate = baseRotate;

  leftShin->mainPosition.transform.translate = {0.0f, -1.0f, 0.0f};
  leftShin->mainPosition.transform.rotate = baseRotate;

  rightThigh->mainPosition.transform.translate = {0.5f, -1.25f, 0.0f};
  rightThigh->mainPosition.transform.rotate = baseRotate;

  rightShin->mainPosition.transform.translate = {0.0f, -1.0f, 0.0f};
  rightShin->mainPosition.transform.rotate = baseRotate;
}

/*   各部位Objectの更新をまとめて行う   */
/*   各部位Objectの更新をまとめて行う   */
void TravelScene::UpdateModObjects() {
  for (size_t i = 0; i < modObjects_.size(); ++i) {
    if (modObjects_[i] != nullptr) {
      // modBodies_[i].Apply(modObjects_[i]);
    }
  }

  for (auto &object : modObjects_) {
    if (object != nullptr) {
      object->Update(usingCamera_);
    }
  }

  static int foreArmWorldLogFrame = 0;
  foreArmWorldLogFrame++;

  if (foreArmWorldLogFrame % 20 == 0) {
    Object *leftForeArm = modObjects_[ToIndex(ModBodyPart::LeftForeArm)];
    Object *leftUpperArm = modObjects_[ToIndex(ModBodyPart::LeftUpperArm)];
  }
}

void TravelScene::DrawModObjects() {
  for (auto &object : modObjects_) {
    if (object != nullptr) {
      object->Draw();
    }
  }
}

void TravelScene::SetupBodyJointOffsets() {
  for (auto &offset : bodyJointOffsets_) {
    offset = {0.0f, 0.0f, 0.0f};
  }

  bodyJointOffsets_[ToIndex(ModBodyPart::Neck)] = {0.0f, 1.0f, 0.0f};

  bodyJointOffsets_[ToIndex(ModBodyPart::LeftUpperArm)] = {-1.25f, 1.0f, 0.0f};
  bodyJointOffsets_[ToIndex(ModBodyPart::RightUpperArm)] = {1.25f, 1.0f, 0.0f};

  bodyJointOffsets_[ToIndex(ModBodyPart::LeftThigh)] = {-0.5f, -1.25f, 0.0f};
  bodyJointOffsets_[ToIndex(ModBodyPart::RightThigh)] = {0.5f, -1.25f, 0.0f};
}

void TravelScene::LoadCustomizeData() {
  if (customizeData_ == nullptr) {
    return;
  }

  // ModScene から引き継いだ残り時間を復元する
  timeLimit_ = customizeData_->timeLimit_;
  isTimeUp_ = customizeData_->isTimeUp_;

  bodyJointOffsets_ = customizeData_->bodyJointOffsets;

  for (size_t i = 0; i < modBodies_.size(); ++i) {
    modBodies_[i].SetParam(customizeData_->partParams[i]);
  }
}

void TravelScene::UpdateChildRootsFromBody() {
  const Vector3 bodyScale =
      modBodies_[ToIndex(ModBodyPart::Body)].GetVisualScaleRatio();

  const Vector3 neckScale =
      modBodies_[ToIndex(ModBodyPart::Neck)].GetVisualScaleRatio();

  const Vector3 headScale =
      modBodies_[ToIndex(ModBodyPart::Head)].GetVisualScaleRatio();

  const Vector3 neckLocal =
      customizeData_->partInstances[ToIndex(ModBodyPart::Neck)]
          .localTransform.translate;

  const Vector3 headLocal =
      customizeData_->partInstances[ToIndex(ModBodyPart::Head)]
          .localTransform.translate;

  const Vector3 leftUpperArmLocal =
      customizeData_->partInstances[ToIndex(ModBodyPart::LeftUpperArm)]
          .localTransform.translate;

  const Vector3 leftForeArmLocal =
      customizeData_->partInstances[ToIndex(ModBodyPart::LeftForeArm)]
          .localTransform.translate;

  const Vector3 rightUpperArmLocal =
      customizeData_->partInstances[ToIndex(ModBodyPart::RightUpperArm)]
          .localTransform.translate;

  const Vector3 leftUpperArmJoint = ScaleByRatio(
      bodyJointOffsets_[ToIndex(ModBodyPart::LeftUpperArm)], bodyScale);

  const Vector3 rightUpperArmJoint = ScaleByRatio(
      bodyJointOffsets_[ToIndex(ModBodyPart::RightUpperArm)], bodyScale);

  const Vector3 leftThighJoint = ScaleByRatio(
      bodyJointOffsets_[ToIndex(ModBodyPart::LeftThigh)], bodyScale);

  const Vector3 rightThighJoint = ScaleByRatio(
      bodyJointOffsets_[ToIndex(ModBodyPart::RightThigh)], bodyScale);

  const Vector3 leftUpperArmScale =
      modBodies_[ToIndex(ModBodyPart::LeftUpperArm)].GetVisualScaleRatio();
  const Vector3 leftForeArmScale =
      modBodies_[ToIndex(ModBodyPart::LeftForeArm)].GetVisualScaleRatio();

  const Vector3 rightUpperArmScale =
      modBodies_[ToIndex(ModBodyPart::RightUpperArm)].GetVisualScaleRatio();
  const Vector3 leftThighScale =
      modBodies_[ToIndex(ModBodyPart::LeftThigh)].GetVisualScaleRatio();
  const Vector3 rightThighScale =
      modBodies_[ToIndex(ModBodyPart::RightThigh)].GetVisualScaleRatio();

  //================================
  // 体・首・頭だけは固定接続点で置く
  // ModAssemblyGraph と同じ値
  //================================
  const Vector3 bodyNeckConnector = {0.0f, 0.75f, 0.0f};
  const Vector3 neckRootConnector = {0.0f, 0.0f, 0.0f};
  const Vector3 neckHeadConnector = {0.0f, 0.65f, 0.0f};
  const Vector3 headRootConnector = {0.0f, 0.0f, 0.0f};
  const Vector3 bodyLeftUpperArmConnector = {-1.25f, 1.0f, 0.0f};
  const Vector3 leftUpperArmRootConnector = {0.0f, 0.0f, 0.0f};
  const Vector3 leftUpperArmForeArmConnector = {0.0f, -1.0f, 0.0f};
  const Vector3 leftForeArmRootConnector = {0.0f, 0.0f, 0.0f};
  const Vector3 bodyRightUpperArmConnector = {1.25f, 1.0f, 0.0f};
  const Vector3 rightUpperArmRootConnector = {0.0f, 0.0f, 0.0f};

  modObjects_[ToIndex(ModBodyPart::Neck)]->mainPosition.transform.translate = {
      bodyNeckConnector.x * bodyScale.x - neckRootConnector.x * neckScale.x +
          neckLocal.x,
      bodyNeckConnector.y * bodyScale.y - neckRootConnector.y * neckScale.y +
          neckLocal.y,
      bodyNeckConnector.z * bodyScale.z - neckRootConnector.z * neckScale.z +
          neckLocal.z};

  modObjects_[ToIndex(ModBodyPart::Head)]->mainPosition.transform.translate = {
      neckHeadConnector.x * neckScale.x - headRootConnector.x * headScale.x +
          headLocal.x,
      neckHeadConnector.y * neckScale.y - headRootConnector.y * headScale.y +
          headLocal.y,
      neckHeadConnector.z * neckScale.z - headRootConnector.z * headScale.z +
          headLocal.z};

  modObjects_[ToIndex(ModBodyPart::LeftUpperArm)]
      ->mainPosition.transform.translate = leftUpperArmJoint;

  // modObjects_[ToIndex(ModBodyPart::LeftUpperArm)]
  //     ->mainPosition.transform.translate = {
  //     bodyLeftUpperArmConnector.x * bodyScale.x -
  //         leftUpperArmRootConnector.x * leftUpperArmScale.x +
  //         leftUpperArmLocal.x,
  //     bodyLeftUpperArmConnector.y * bodyScale.y -
  //         leftUpperArmRootConnector.y * leftUpperArmScale.y +
  //         leftUpperArmLocal.y,
  //     bodyLeftUpperArmConnector.z * bodyScale.z -
  //         leftUpperArmRootConnector.z * leftUpperArmScale.z +
  //         leftUpperArmLocal.z};

  modObjects_[ToIndex(ModBodyPart::RightUpperArm)]
      ->mainPosition.transform.translate = rightUpperArmJoint;

  // modObjects_[ToIndex(ModBodyPart::RightUpperArm)]
  //     ->mainPosition.transform.translate = {
  //     bodyRightUpperArmConnector.x * bodyScale.x -
  //         rightUpperArmRootConnector.x * rightUpperArmScale.x +
  //         rightUpperArmLocal.x,
  //     bodyRightUpperArmConnector.y * bodyScale.y -
  //         rightUpperArmRootConnector.y * rightUpperArmScale.y +
  //         rightUpperArmLocal.y,
  //     bodyRightUpperArmConnector.z * bodyScale.z -
  //         rightUpperArmRootConnector.z * rightUpperArmScale.z +
  //         rightUpperArmLocal.z};

  modObjects_[ToIndex(ModBodyPart::LeftThigh)]
      ->mainPosition.transform.translate = leftThighJoint;

  modObjects_[ToIndex(ModBodyPart::RightThigh)]
      ->mainPosition.transform.translate = rightThighJoint;

  const float leftUpperArmRotX = modObjects_[ToIndex(ModBodyPart::LeftUpperArm)]
                                     ->mainPosition.transform.rotate.x;
  const float rightUpperArmRotX =
      modObjects_[ToIndex(ModBodyPart::RightUpperArm)]
          ->mainPosition.transform.rotate.x;

  const float leftThighRotX = modObjects_[ToIndex(ModBodyPart::LeftThigh)]
                                  ->mainPosition.transform.rotate.x;
  const float rightThighRotX = modObjects_[ToIndex(ModBodyPart::RightThigh)]
                                   ->mainPosition.transform.rotate.x;

  // modObjects_[ToIndex(ModBodyPart::LeftForeArm)]
  //     ->mainPosition.transform.translate = {
  //     0.0f, -std::cos(leftUpperArmRotX) * leftUpperArmScale.y,
  //     -std::sin(leftUpperArmRotX) * leftUpperArmScale.y};

  // 上腕の位置
  // Vector3 upperArmPos = modObjects_[ToIndex(ModBodyPart::LeftUpperArm)]
  //                          ->mainPosition.transform.translate;

  //// 回転後の肘オフセット（さっきログで出してたやつと同じ計算）
  // float tipY = leftUpperArmForeArmConnector.y * leftUpperArmScale.y *
  //                  std::cos(leftUpperArmRotX) -
  //              leftUpperArmForeArmConnector.z * leftUpperArmScale.z *
  //                  std::sin(leftUpperArmRotX);

  // float tipZ = leftUpperArmForeArmConnector.y * leftUpperArmScale.y *
  //                  std::sin(leftUpperArmRotX) +
  //              leftUpperArmForeArmConnector.z * leftUpperArmScale.z *
  //                  std::cos(leftUpperArmRotX);

  // 上腕ローカル空間での肘位置
  // Vector3 elbowLocalPos = {0.0f, tipY, tipZ};

  //// 前腕モデルは中心基準っぽいので、半分先へずらす
  // float halfForeArmLength = leftForeArmScale.y * 0.5f;

  // float foreArmCenterOffsetY =
  //     leftUpperArmForeArmConnector.y * halfForeArmLength *
  //         std::cos(leftUpperArmRotX) -
  //     leftUpperArmForeArmConnector.z * halfForeArmLength *
  //         std::sin(leftUpperArmRotX);

  // float foreArmCenterOffsetZ =
  //     leftUpperArmForeArmConnector.y * halfForeArmLength *
  //         std::sin(leftUpperArmRotX) +
  //     leftUpperArmForeArmConnector.z * halfForeArmLength *
  //         std::cos(leftUpperArmRotX);

  // modObjects_[ToIndex(ModBodyPart::LeftForeArm)]
  //       ->mainPosition.transform.translate = elbowLocalPos;

  Vector3 elbowLocalPos = {
      0.0f, leftUpperArmForeArmConnector.y * leftUpperArmScale.y,
      leftUpperArmForeArmConnector.z * leftUpperArmScale.z};

  Vector3 foreArmLocalPos = {elbowLocalPos.x,
                             elbowLocalPos.y + leftForeArmScale.y * 0.5f,
                             elbowLocalPos.z};

  modObjects_[ToIndex(ModBodyPart::LeftForeArm)]
      ->mainPosition.transform.translate = foreArmLocalPos;

  Logger::Log("==== A UpdateChildRootsFromBody ====");
  Logger::Log("LeftForeArm mainPosition : %.3f %.3f %.3f",
              modObjects_[ToIndex(ModBodyPart::LeftForeArm)]
                  ->mainPosition.transform.translate.x,
              modObjects_[ToIndex(ModBodyPart::LeftForeArm)]
                  ->mainPosition.transform.translate.y,
              modObjects_[ToIndex(ModBodyPart::LeftForeArm)]
                  ->mainPosition.transform.translate.z);

  Logger::Log("==== ForeArm Travel Compensation Check ====");
  Logger::Log("elbowLocalPos : %.3f %.3f %.3f", elbowLocalPos.x,
              elbowLocalPos.y, elbowLocalPos.z);
  Logger::Log("leftForeArmScale : %.3f %.3f %.3f", leftForeArmScale.x,
              leftForeArmScale.y, leftForeArmScale.z);
  Logger::Log("foreArmLocalPos : %.3f %.3f %.3f", foreArmLocalPos.x,
              foreArmLocalPos.y, foreArmLocalPos.z);

  // modObjects_[ToIndex(ModBodyPart::LeftForeArm)]
  //     ->mainPosition.transform.translate = {
  //     leftUpperArmForeArmConnector.x * leftUpperArmScale.x -
  //         leftForeArmRootConnector.x * leftForeArmScale.x +
  //         leftForeArmLocal.x,
  //     leftUpperArmForeArmConnector.y * leftUpperArmScale.y -
  //         leftForeArmRootConnector.y * leftForeArmScale.y +
  //         leftForeArmLocal.y,
  //     leftUpperArmForeArmConnector.z * leftUpperArmScale.z -
  //         leftForeArmRootConnector.z * leftForeArmScale.z +
  //         leftForeArmLocal.z};

  modObjects_[ToIndex(ModBodyPart::RightForeArm)]
      ->mainPosition.transform.translate = {
      0.0f, -std::cos(rightUpperArmRotX) * rightUpperArmScale.y,
      -std::sin(rightUpperArmRotX) * rightUpperArmScale.y};

  modObjects_[ToIndex(ModBodyPart::LeftShin)]
      ->mainPosition.transform.translate = {
      0.0f, -std::cos(leftThighRotX) * leftThighScale.y,
      -std::sin(leftThighRotX) * leftThighScale.y};

  modObjects_[ToIndex(ModBodyPart::RightShin)]
      ->mainPosition.transform.translate = {
      0.0f, -std::cos(rightThighRotX) * rightThighScale.y,
      -std::sin(rightThighRotX) * rightThighScale.y};
}

void TravelScene::ApplyCustomizeToMovementParam() {
  //==============================
  // 各部位のパラメータ取得
  //==============================
  const auto &body =
      modBodies_[static_cast<size_t>(ModBodyPart::Body)].GetParam();
  const auto &neck =
      modBodies_[static_cast<size_t>(ModBodyPart::Neck)].GetParam();
  const auto &leftThigh =
      modBodies_[static_cast<size_t>(ModBodyPart::LeftThigh)].GetParam();
  const auto &rightThigh =
      modBodies_[static_cast<size_t>(ModBodyPart::RightThigh)].GetParam();

  const auto &leftShin =
      modBodies_[static_cast<size_t>(ModBodyPart::LeftShin)].GetParam();
  const auto &rightShin =
      modBodies_[static_cast<size_t>(ModBodyPart::RightShin)].GetParam();

  const auto &leftUpperArm =
      modBodies_[static_cast<size_t>(ModBodyPart::LeftUpperArm)].GetParam();
  const auto &rightUpperArm =
      modBodies_[static_cast<size_t>(ModBodyPart::RightUpperArm)].GetParam();

  //==============================
  // 基本値の計算
  //==============================

  // 脚の長さ感（Y）
  const float legLengthScale = (leftThigh.scale.y + rightThigh.scale.y +
                                leftShin.scale.y + rightShin.scale.y) *
                               0.25f;

  // 脚の太さ感（X/Z）
  const float leftThighThickness =
      (leftThigh.scale.x + leftThigh.scale.z) * 0.5f;
  const float rightThighThickness =
      (rightThigh.scale.x + rightThigh.scale.z) * 0.5f;
  const float leftShinThickness = (leftShin.scale.x + leftShin.scale.z) * 0.5f;
  const float rightShinThickness =
      (rightShin.scale.x + rightShin.scale.z) * 0.5f;

  const float legThicknessScale = (leftThighThickness + rightThighThickness +
                                   leftShinThickness + rightShinThickness) *
                                  0.25f;

  // 長くて細いほど大きくなる
  const float legSlimness = legLengthScale / (legThicknessScale + 0.001f);

  // 腕の太さ感（安定補助に少しだけ使う）
  const float leftUpperArmThickness =
      (leftUpperArm.scale.x + leftUpperArm.scale.z) * 0.5f;
  const float rightUpperArmThickness =
      (rightUpperArm.scale.x + rightUpperArm.scale.z) * 0.5f;
  const float armThicknessScale =
      (leftUpperArmThickness + rightUpperArmThickness) * 0.5f;

  // Bodyの大きさ感（XYZ全部）
  const float bodyScaleAvg =
      (body.scale.x + body.scale.y + body.scale.z) / 3.0f;

  // 左右差（脚の長さ差）
  const float leftLegTotal = leftThigh.scale.y + leftShin.scale.y;
  const float rightLegTotal = rightThigh.scale.y + rightShin.scale.y;
  const float legDiff = std::abs(leftLegTotal - rightLegTotal);

  float neckLengthScale = neck.scale.y;
  float neckThicknessScale = (neck.scale.x + neck.scale.z) * 0.5f;

  //==============================
  // チューニング値生成
  //==============================

  // 前進力
  float lengthDiff = legLengthScale - 1.0f;
  float thicknessDiff = legThicknessScale - 1.0f;

  float lengthPenalty = lengthDiff * lengthDiff; // 長すぎても短すぎてもダメ
  float thicknessPenalty = thicknessDiff * thicknessDiff;

  tuning_.runPower =
      (std::max)(0.5f, 1.0f - lengthPenalty * 0.4f - thicknessPenalty * 0.55f);

  tuning_.runPower -= (bodyScaleAvg - 1.0f) * 0.25f;

  // 最高速度
  // 長いとちょっと速い（歩幅）
  float lengthBonus = lengthDiff * 0.45f;

  // 太いと遅い
  float thicknessPenaltySpeed = thicknessDiff * 0.2f;

  // 左右差でロス
  float balancePenalty = legDiff * 0.15f;

  tuning_.maxSpeed =
      1.0f + lengthBonus - thicknessPenaltySpeed - balancePenalty;

  // 下限
  tuning_.maxSpeed = (std::max)(0.4f, tuning_.maxSpeed);

  // 安定性
  // 太い脚・太い腕・大きい胴体は安定
  // 長くて細い脚、左右差は不安定
  tuning_.stability = 1.0f + (legThicknessScale - 1.0f) * 0.6f +
                      (armThicknessScale - 1.0f) * 0.2f +
                      (bodyScaleAvg - 1.0f) * 0.35f -
                      (legSlimness - 1.0f) * 0.45f - legDiff * 0.30f;

  float neckLengthDiff = neckLengthScale - 1.0f;
  float neckThicknessDiff = neckThicknessScale - 1.0f;

  // 長いほど不安定、太いほど安定
  tuning_.stability -= neckLengthDiff * 0.15f;
  tuning_.stability += neckThicknessDiff * 0.10f;

  // 上方向の出やすさ
  // 長い脚は少し有利、太い脚と大きい胴体は重くする
  tuning_.lift = 1.0f + (legLengthScale - 1.0f) * 0.20f -
                 (legThicknessScale - 1.0f) * 0.25f -
                 (bodyScaleAvg - 1.0f) * 0.20f;

  // 傾きの反応
  // 細長い脚や左右差があると不安定に反応しやすい
  // 大きい胴体は鈍くする
  tuning_.turnResponse = 1.0f + (legSlimness - 1.0f) * 0.25f + legDiff * 0.20f -
                         (bodyScaleAvg - 1.0f) * 0.15f;

  tuning_.turnResponse += neckLengthDiff * 0.10f;

  //--------------------------------
  // CharacterFeatures 反映
  //--------------------------------

  // 安全用
  float stability = tuning_.stability;
  float turnResponse = tuning_.turnResponse;
  float lift = tuning_.lift;

  //----------------------
  // 頭の数
  //----------------------
  float extraHeadCount = (std::max)(0.0f, features_.headCount - 1.0f);
  stability -= extraHeadCount * 0.05f;

  //----------------------
  // 腕の数
  //----------------------
  float extraArmCount = (std::max)(0.0f, features_.armCount - 2.0f);
  stability -= extraArmCount * 0.03f;

  //----------------------
  // 左右非対称
  //----------------------
  stability -= features_.asymmetry * 0.2f;
  turnResponse += features_.asymmetry * 0.3f;

  //----------------------
  // 重心（高いほど不安定＆跳ねる）
  //----------------------
  float comOffset = (std::clamp)(features_.centerOfMassY, -3.0f, 3.0f);
  stability -= comOffset * 0.1f;
  lift += comOffset * 0.15f;

  //----------------------
  // 下限・上限
  //----------------------
  stability = (std::clamp)(stability, 0.3f, 2.0f);
  turnResponse = (std::clamp)(turnResponse, 0.3f, 2.0f);
  lift = (std::clamp)(lift, 0.3f, 2.0f);

  //----------------------
  // 反映
  //----------------------
  tuning_.stability = stability;
  tuning_.turnResponse = turnResponse;
  tuning_.lift = lift;

  //==============================
  // 安全クランプ
  //==============================
  tuning_.runPower = std::clamp(tuning_.runPower, 0.5f, 2.5f);
  tuning_.maxSpeed = std::clamp(tuning_.maxSpeed, 0.3f, 2.0f);
  tuning_.stability = std::clamp(tuning_.stability, 0.3f, 2.0f);
  tuning_.lift = std::clamp(tuning_.lift, 0.3f, 2.0f);
  tuning_.turnResponse = std::clamp(tuning_.turnResponse, 0.3f, 2.0f);

  // Logger::Log("==== CUSTOMIZE PARAM ====");
  // Logger::Log("legLengthScale : %.3f", legLengthScale);
  // Logger::Log("legThicknessScale : %.3f", legThicknessScale);
  // Logger::Log("legDiff : %.3f", legDiff);

  // Logger::Log("runPower : %.3f", tuning_.runPower);
  // Logger::Log("maxSpeed : %.3f", tuning_.maxSpeed);
  // Logger::Log("stability : %.3f", tuning_.stability);
  // Logger::Log("lift : %.3f", tuning_.lift);
  // Logger::Log("turnResponse : %.3f", tuning_.turnResponse);
  // Logger::Log("neckLengthScale : %.3f", neckLengthScale);
  // Logger::Log("neckThicknessScale : %.3f", neckThicknessScale);
  // Logger::Log("neckLengthDiff : %.3f", neckLengthDiff);
  // Logger::Log("neckThicknessDiff : %.3f", neckThicknessDiff);

  // Logger::Log("headCount: %d\n", features_.headCount);
  // Logger::Log("armCount: %d\n", features_.armCount);
  // Logger::Log("asymmetry: %f\n", features_.asymmetry);
  // Logger::Log("centerOfMassY: %f\n", features_.centerOfMassY);
  // Logger::Log("partInstances size: %d",
  // customizeData_->partInstances.size());
}

float TravelScene::ComputeLegHeightOffset() const {
  const Vector3 bodyScale =
      modBodies_[ToIndex(ModBodyPart::Body)].GetVisualScaleRatio();

  const Vector3 leftThighScale =
      modBodies_[ToIndex(ModBodyPart::LeftThigh)].GetVisualScaleRatio();
  const Vector3 rightThighScale =
      modBodies_[ToIndex(ModBodyPart::RightThigh)].GetVisualScaleRatio();
  const Vector3 leftShinScale =
      modBodies_[ToIndex(ModBodyPart::LeftShin)].GetVisualScaleRatio();
  const Vector3 rightShinScale =
      modBodies_[ToIndex(ModBodyPart::RightShin)].GetVisualScaleRatio();

  //==============================
  // 基準値（無改造時）
  //==============================
  const float baseBodyHeight = 1.0f;
  const float baseThighLength = 1.25f;
  const float baseShinLength = 1.0f;

  //==============================
  // 現在の脚長
  //==============================
  const float leftLegLength = leftThighScale.y + leftShinScale.y;
  const float rightLegLength = rightThighScale.y + rightShinScale.y;
  const float avgLegLength = (leftLegLength + rightLegLength) * 0.5f;

  const float baseLegLength = baseThighLength + baseShinLength;
  const float legOffset = avgLegLength - baseLegLength;

  //==============================
  // 胴体の縦補正
  // 胴体は原点中心スケールの可能性が高いので
  // 伸びたぶんの半分だけ高さ補正に使う
  //==============================
  const float bodyOffset = (bodyScale.y - baseBodyHeight) * 1.2f;

  return legOffset + bodyOffset;
}

bool TravelScene::HasRequiredParts() const {
  Object *body = modObjects_[ToIndex(ModBodyPart::Body)];
  Object *neck = modObjects_[ToIndex(ModBodyPart::Neck)];
  Object *head = modObjects_[ToIndex(ModBodyPart::Head)];

  Object *leftUpperArm = modObjects_[ToIndex(ModBodyPart::LeftUpperArm)];
  Object *leftForeArm = modObjects_[ToIndex(ModBodyPart::LeftForeArm)];
  Object *rightUpperArm = modObjects_[ToIndex(ModBodyPart::RightUpperArm)];
  Object *rightForeArm = modObjects_[ToIndex(ModBodyPart::RightForeArm)];

  Object *leftThigh = modObjects_[ToIndex(ModBodyPart::LeftThigh)];
  Object *leftShin = modObjects_[ToIndex(ModBodyPart::LeftShin)];
  Object *rightThigh = modObjects_[ToIndex(ModBodyPart::RightThigh)];
  Object *rightShin = modObjects_[ToIndex(ModBodyPart::RightShin)];

  // nullチェック
  if (body == nullptr || neck == nullptr || head == nullptr ||
      leftUpperArm == nullptr || leftForeArm == nullptr ||
      rightUpperArm == nullptr || rightForeArm == nullptr ||
      leftThigh == nullptr || leftShin == nullptr || rightThigh == nullptr ||
      rightShin == nullptr) {
    return false;
  }

  return true;
}

void TravelScene::SavePreviousFrameState() {
  leftLegPrevBend_ = leftLegBend_;
  rightLegPrevBend_ = rightLegBend_;

  leftLegPrevBendSpeed_ = leftLegBendSpeed_;
  rightLegPrevBendSpeed_ = rightLegBendSpeed_;
}

void TravelScene::UpdateTimeLimit(float deltaTime) {
  if (!isTimeUp_ && !isGoalReached_) {
    timeLimit_ -= deltaTime;

    if (timeLimit_ <= 0.0f) {
      timeLimit_ = 0.0f;
      isTimeUp_ = true;
    }
  }

  if (customizeData_ != nullptr) {
    customizeData_->timeLimit_ = timeLimit_;
    customizeData_->isTimeUp_ = isTimeUp_;
  }
}

void TravelScene::UpdateHoldState(bool leftNowInput, bool rightNowInput,
                                  float deltaTime) {
  if (leftNowInput && isGrounded_) {
    leftHoldTime_ += deltaTime;
  } else {
    leftHoldTime_ = 0.0f;
  }

  if (rightNowInput && isGrounded_) {
    rightHoldTime_ += deltaTime;
  } else {
    rightHoldTime_ = 0.0f;
  }

  if (!leftNowInput) {
    requireReleaseAfterLandLeft_ = false;
  }
  if (!rightNowInput) {
    requireReleaseAfterLandRight_ = false;
  }
}

void TravelScene::UpdateLegBendState(bool leftNowInput, bool rightNowInput) {
  float leftTargetLegAngle = leftNowInput ? legKickAngle_ : legRecoverAngle_;
  float rightTargetLegAngle = rightNowInput ? legKickAngle_ : legRecoverAngle_;

  leftLegBendSpeed_ += (leftTargetLegAngle - leftLegBend_) * legFollowPower_;
  rightLegBendSpeed_ += (rightTargetLegAngle - rightLegBend_) * legFollowPower_;

  leftLegBendSpeed_ =
      std::clamp(leftLegBendSpeed_, -legMaxSpeed_, legMaxSpeed_);
  rightLegBendSpeed_ =
      std::clamp(rightLegBendSpeed_, -legMaxSpeed_, legMaxSpeed_);

  //--------------------------------
  // 減衰
  //--------------------------------
  leftLegBendSpeed_ *= jointDamping_;
  rightLegBendSpeed_ *= jointDamping_;
  bodyStretchSpeed_ *= jointDamping_;

  //--------------------------------
  // 値更新
  //--------------------------------
  leftLegBend_ += leftLegBendSpeed_;
  rightLegBend_ += rightLegBendSpeed_;
  bodyStretch_ += bodyStretchSpeed_;

  // 曲げ量は大きくなりすぎないように軽く制限
  leftLegBend_ = std::clamp(leftLegBend_, -0.8f, 1.0f);
  rightLegBend_ = std::clamp(rightLegBend_, -0.8f, 1.0f);
  bodyStretch_ = std::clamp(bodyStretch_, -0.5f, 1.0f);
}

void TravelScene::UpdateMovementState(bool leftNowInput, bool rightNowInput) {

  bool bothInput = leftNowInput && rightNowInput;

  //--------------------------------
  // 姿勢更新
  //--------------------------------

  float forwardTorque = 0.0f;
  float backwardTorque = 0.0f;
  float pushTorque = 0.0f;

  //==============================
  // 脚回収による後傾トルク
  // 「離している間ずっと」ではなく
  // 「前に戻す加速」が出た時だけ反動を入れる
  //==============================

  float stability = useCustomizeMove_ ? tuning_.stability : 1.0f;
  float turnResponse = useCustomizeMove_ ? tuning_.turnResponse : 1.0f;

  if (!leftNowInput) {
    float leftRecoveryAccel = leftLegBendSpeed_ - leftLegPrevBendSpeed_;

    if (leftRecoveryAccel > 0.0f) {
      float torque = leftRecoveryAccel * 0.6f / stability;
      backwardTorque += torque;
    }
  }

  if (!rightNowInput) {
    float rightRecoveryAccel = rightLegBendSpeed_ - rightLegPrevBendSpeed_;

    if (rightRecoveryAccel > 0.0f) {
      float torque = rightRecoveryAccel * 0.6f / stability;
      backwardTorque += torque;
    }
  }

  float targetTilt = 0.0f;

  // 両押し：前傾
  if (bothInput) {
    targetTilt = -0.15f;
  }

  float tiltError = targetTilt - bodyTilt_;
  bodyTiltVelocity_ += tiltError * 0.1f * turnResponse;

  float restoreTorque = -bodyTilt_ * 0.01f * stability;
  bodyTiltVelocity_ += restoreTorque;

  //--------------------------------
  // 姿勢の良し悪しを作る
  //--------------------------------

  float postureError = std::abs(bodyTilt_ - idealRunTilt_);
  float badPosture = std::clamp(postureError / postureTolerance_, 0.0f, 1.0f);

  // 良い姿勢ほど前進しやすい
  float forwardRate = 1.0f - badPosture;

  // 悪い姿勢ほど上に逃げやすい
  float upwardRate = 0.30f + badPosture * 0.70f;

  //--------------------------------
  // 押している間に drive を使って前進する
  //--------------------------------
  bool leftHoldEnough = (leftHoldTime_ >= minHoldTimeToKick_);
  bool rightHoldEnough = (rightHoldTime_ >= minHoldTimeToKick_);

  float leftPushCandidate = 0.0f;
  float rightPushCandidate = 0.0f;

  bool useLeftPush = false;
  bool useRightPush = false;
  float adoptedDriveUse = 0.0f;

  if (leftNowInput && isGrounded_ && velocityY_ <= 0.0f &&
      leftDriveAccum_ >= minDriveToKick_ && leftHoldEnough &&
      !requireReleaseAfterLandLeft_) {

    float sequenceBonus = 1.0f;

    if (lastKickSide_ == 1) {
      sequenceBonus = alternateKickBonus_;
    } else if (lastKickSide_ == -1) {
      sequenceBonus = sameSideKickPenalty_;
    }

    float driveUse = (std::min)(leftDriveAccum_, driveUsePerFrame_);

    leftPushCandidate = driveUse * drivePushScale_ * pushPower_ *
                        groundAssist_ * groundedKickFactor_ * 2.0f *
                        sequenceBonus;
  }

  if (rightNowInput && isGrounded_ && velocityY_ <= 0.0f &&
      rightDriveAccum_ >= minDriveToKick_ && rightHoldEnough &&
      !requireReleaseAfterLandRight_) {
    float sequenceBonus = 1.0f;
    if (lastKickSide_ == -1) {
      sequenceBonus = alternateKickBonus_;
    } else if (lastKickSide_ == 1) {
      sequenceBonus = sameSideKickPenalty_;
    }

    float driveUse = (std::min)(rightDriveAccum_, driveUsePerFrame_);

    rightPushCandidate = driveUse * drivePushScale_ * pushPower_ *
                         groundAssist_ * groundedKickFactor_ * 2.0f *
                         sequenceBonus;
  }

  float totalPush = 0.0f;

  if (leftPushCandidate >= rightPushCandidate && leftPushCandidate > 0.0f) {
    totalPush = leftPushCandidate;
    useLeftPush = true;
    adoptedDriveUse = (std::min)(leftDriveAccum_, driveUsePerFrame_);
    lastKickSide_ = -1;

    leftDriveAccum_ -= adoptedDriveUse;
    leftDriveAccum_ = (std::max)(0.0f, leftDriveAccum_);

    // 同時押しで反対側が溜まり続けるのを少し抑える
    if (rightNowInput) {
      rightDriveAccum_ *= 0.85f;
    }

  } else if (rightPushCandidate > leftPushCandidate &&
             rightPushCandidate > 0.0f) {
    totalPush = rightPushCandidate;
    useRightPush = true;
    adoptedDriveUse = (std::min)(rightDriveAccum_, driveUsePerFrame_);
    lastKickSide_ = 1;

    rightDriveAccum_ -= adoptedDriveUse;
    rightDriveAccum_ = (std::max)(0.0f, rightDriveAccum_);

    if (leftNowInput) {
      leftDriveAccum_ *= 0.85f;
    }
  }

  if (totalPush > 0.0f) {
    //==============================
    // 体の前傾で前進効率を強く変える
    //==============================
    float tiltForwardFactor =
        std::clamp((-bodyTilt_ - 0.02f) * 3.5f, 0.0f, 1.5f);

    //==============================
    // 採用された脚が前にあるかどうかで push の向きを補正する
    //==============================
    float kickLegBend = 0.0f;
    if (useLeftPush) {
      kickLegBend = leftLegBend_;
    } else if (useRightPush) {
      kickLegBend = rightLegBend_;
    }

    // 0.0 = 蹴り姿勢側 / 1.0 = 回収姿勢側
    float kickLegForwardness =
        1.0f - std::clamp((kickLegBend - legKickAngle_) /
                              (legRecoverAngle_ - legKickAngle_),
                          0.0f, 1.0f);

    // 脚が前にあるほど前向きの蹴りになりやすい
    float footForwardBonus = 0.75f + kickLegForwardness * 0.75f;
    float footUpwardSuppress = 1.15f - kickLegForwardness * 0.55f;
    float groundBoost = isGrounded_ ? 1.2f : 0.6f;

    float runPower = useCustomizeMove_ ? tuning_.runPower : 1.0f;

    float pushX = totalPush * runPower *
                  (0.40f + forwardRate * 0.50f + tiltForwardFactor * 0.75f) *
                  footForwardBonus * groundBoost;

    float upwardSuppressByTilt =
        std::clamp(1.0f - tiltForwardFactor * 0.25f, 0.75f, 1.0f);

    float lift = useCustomizeMove_ ? tuning_.lift : 1.0f;

    float pushY = totalPush * lift * jumpRatio_ * upwardRate *
                  upwardSuppressByTilt * footUpwardSuppress;

    velocityX_ += pushX;
    velocityY_ += pushY;

    // 押した瞬間にも反動を入れる
    // 片足押しでも揺れるようにする
    pushTorque = pushTiltKick_ * adoptedDriveUse * 1.2f;

    // 両押しはさらに強く揺らす
    if (bothInput) {
      pushTorque *= 1.5f;
    }

    // どっちの脚で蹴ったかで少し差をつけたいならここで補正
    if (useLeftPush) {
      pushTorque *= 1.0f;
    } else if (useRightPush) {
      pushTorque *= 1.0f;
    }
  }

  // 蹴りにならなかった側は少しずつ減衰
  if (!(leftNowInput && isGrounded_ && velocityY_ <= 0.0f &&
        !requireReleaseAfterLandLeft_)) {
    leftDriveAccum_ = (std::max)(0.0f, leftDriveAccum_ - driveDecay_);
  }

  if (!(rightNowInput && isGrounded_ && velocityY_ <= 0.0f &&
        !requireReleaseAfterLandRight_)) {
    rightDriveAccum_ = (std::max)(0.0f, rightDriveAccum_ - driveDecay_);
  }

  if (pushTorque < 0.0f) {
    forwardTorque += -pushTorque;
  } else {
    backwardTorque += pushTorque;
  }

  float netTorque = backwardTorque - forwardTorque;
  bodyTiltVelocity_ += netTorque;

  // 減衰
  float tiltDamping = 0.80f + (stability - 1.0f) * 0.08f;
  tiltDamping = std::clamp(tiltDamping, 0.70f, 0.92f);
  bodyTiltVelocity_ *= tiltDamping;

  // 更新
  bodyTilt_ += bodyTiltVelocity_;

  // 限界角で止める
  bodyTilt_ = std::clamp(bodyTilt_, maxForwardTilt_, maxBackwardTilt_);

  // デバッグ：姿勢固定
  if (debugForceTilt_) {
    bodyTilt_ = debugTiltValue_;
  }

  //--------------------------------
  // 接地中に押し込み量を蓄積する
  //--------------------------------
  float leftExtendAmount = 0.0f;
  float rightExtendAmount = 0.0f;

  float leftRecoverAmount = 0.0f;
  float rightRecoverAmount = 0.0f;

  // 接地中に「ちゃんと押し込めている量」を溜める
  if (leftNowInput && isGrounded_ && velocityY_ <= 0.0f) {
    leftExtendAmount = (std::max)(0.0f, leftLegPrevBend_ - leftLegBend_);

    leftRecoverAmount = std::clamp((leftLegPrevBend_ - legKickAngle_) /
                                       (legRecoverAngle_ - legKickAngle_),
                                   0.0f, 1.0f);

    float buildScale = driveBuildScale_;
    if (leftNowInput && rightNowInput) {
      buildScale *= bothHoldBuildPenalty_;
    }

    float leftDriveGain = leftExtendAmount *
                          (minKickPower_ + leftRecoverAmount * 0.70f) *
                          buildScale;

    leftDriveAccum_ += leftDriveGain;
    leftDriveAccum_ = std::clamp(leftDriveAccum_, 0.0f, maxDriveAccum_);
  }

  if (rightNowInput && isGrounded_ && velocityY_ <= 0.0f) {
    rightExtendAmount = (std::max)(0.0f, rightLegPrevBend_ - rightLegBend_);

    rightRecoverAmount = std::clamp((rightLegPrevBend_ - legKickAngle_) /
                                        (legRecoverAngle_ - legKickAngle_),
                                    0.0f, 1.0f);

    float buildScale = driveBuildScale_;
    if (leftNowInput && rightNowInput) {
      buildScale *= bothHoldBuildPenalty_;
    }

    float rightDriveGain = rightExtendAmount *
                           (minKickPower_ + rightRecoverAmount * 0.70f) *
                           buildScale;

    rightDriveAccum_ += rightDriveGain;
    rightDriveAccum_ = std::clamp(rightDriveAccum_, 0.0f, maxDriveAccum_);
  }

  bool wasGrounded = isGrounded_;
  //--------------------------------
  // 速度減衰
  //--------------------------------
  velocityX_ *= inertia_;

  float maxSpeed = useCustomizeMove_ ? tuning_.maxSpeed : 1.0f;
  velocityX_ = std::clamp(velocityX_, -1.2f * maxSpeed, 1.2f * maxSpeed);

  moveX_ += velocityX_;

  velocityY_ -= gravity_;
  moveY_ += velocityY_;

  if (moveY_ <= groundY_) {
    isGrounded_ = true;
    moveY_ = groundY_;
    velocityY_ = 0.0f;
  } else {
    isGrounded_ = false;
  }

  bool justLanded = (!wasGrounded && isGrounded_);

  static int logFrame = 0;
  logFrame++;

  if (logFrame % 10 == 0) {
  }

  if (justLanded) {
    leftDriveAccum_ = 0.0f;
    rightDriveAccum_ = 0.0f;

    leftHoldTime_ = 0.0f;
    rightHoldTime_ = 0.0f;

    requireReleaseAfterLandLeft_ = leftNowInput;
    requireReleaseAfterLandRight_ = rightNowInput;
  }

  //--------------------------------
  // 自然に元へ戻る
  //--------------------------------
  bodyStretch_ *= 0.90f;
}

void TravelScene::ApplyVisualState() {
  //================================
  // 体全体の位置・姿勢
  //================================

  Object *body = modObjects_[ToIndex(ModBodyPart::Body)];
  Object *neck = modObjects_[ToIndex(ModBodyPart::Neck)];
  Object *head = modObjects_[ToIndex(ModBodyPart::Head)];

  Object *leftUpperArm = modObjects_[ToIndex(ModBodyPart::LeftUpperArm)];
  Object *leftForeArm = modObjects_[ToIndex(ModBodyPart::LeftForeArm)];
  Object *rightUpperArm = modObjects_[ToIndex(ModBodyPart::RightUpperArm)];
  Object *rightForeArm = modObjects_[ToIndex(ModBodyPart::RightForeArm)];

  Object *leftThigh = modObjects_[ToIndex(ModBodyPart::LeftThigh)];
  Object *leftShin = modObjects_[ToIndex(ModBodyPart::LeftShin)];
  Object *rightThigh = modObjects_[ToIndex(ModBodyPart::RightThigh)];
  Object *rightShin = modObjects_[ToIndex(ModBodyPart::RightShin)];

  body->mainPosition.transform.translate.x = moveX_;
  // const float legHeightOffset = ComputeLegHeightOffset();
  // body->mainPosition.transform.translate.y = moveY_ + legHeightOffset;
  body->mainPosition.transform.translate.y = moveY_;

  // body->mainPosition.transform.rotate.x = 0.0f;
  // body->mainPosition.transform.rotate.y = 1.57f;
  // body->mainPosition.transform.rotate.z = bodyTilt_;

  body->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  body->mainPosition.transform.scale.x = 1.0f;
  body->mainPosition.transform.scale.y =
      (std::max)(0.65f, 1.0f - bodyStretch_ * 0.2f);
  body->mainPosition.transform.scale.z = 1.0f;

  //================================
  // 各部位の回転をいったん初期化
  //================================
  neck->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  head->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  leftUpperArm->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  leftForeArm->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  rightUpperArm->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  rightForeArm->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  leftThigh->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  leftShin->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  rightThigh->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  rightShin->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  // head->mainPosition.transform.rotate.x =
  //     (leftLegBend_ - rightLegBend_) * 0.15f;

  // leftUpperArm->mainPosition.transform.rotate.x = rightLegBend_ * 0.6f;
  // rightUpperArm->mainPosition.transform.rotate.x = leftLegBend_ * 0.6f;

  //// leftForeArm->mainPosition.transform.rotate.x =
  ////     leftUpperArm->mainPosition.transform.rotate.x - 0.45f;
  //// rightForeArm->mainPosition.transform.rotate.x =
  ////     rightUpperArm->mainPosition.transform.rotate.x - 0.45f;

  //// leftForeArm->mainPosition.transform.rotate.x = 0.45f;
  //// leftForeArm->mainPosition.transform.rotate.x = 0.0f;
  // leftForeArm->mainPosition.transform.rotate.x =
  //     leftUpperArm->mainPosition.transform.rotate.x;

  // rightForeArm->mainPosition.transform.rotate.x = 0.45f;

  // static int armAxisLogFrame = 0;
  // armAxisLogFrame++;

  // if (armAxisLogFrame % 20 == 0) {
  // }

  // static int foreArmAxisCheckFrame = 0;
  // foreArmAxisCheckFrame++;

  // if (foreArmAxisCheckFrame % 20 == 0) {
  // }

  // const float thighBase = 0.10f;

  // leftThigh->mainPosition.transform.rotate.x = thighBase + leftLegBend_ *
  // 0.70f; rightThigh->mainPosition.transform.rotate.x =
  //     thighBase + rightLegBend_ * 0.70f;

  //// leftShin->mainPosition.transform.rotate.x =
  ////     leftThigh->mainPosition.transform.rotate.x + 0.45f;
  //// rightShin->mainPosition.transform.rotate.x =
  ////     rightThigh->mainPosition.transform.rotate.x + 0.45f;

  // leftShin->mainPosition.transform.rotate.x = -0.45f;
  // rightShin->mainPosition.transform.rotate.x = -0.45f;

  //================================
  // 回転を入れたあと、親子接続位置を再計算
  //================================
  UpdatePartRootsFromControlPoints();

  const ModControlPointData *cp = GetControlPoints();
  Vector3 saved = {0.0f, 0.0f, 0.0f};

  if (cp != nullptr) {
    saved = cp->leftShoulderPos;
  }

  if (!neck->objectParts_.empty()) {
    neck->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }
  if (!head->objectParts_.empty()) {
    head->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }
  if (!leftUpperArm->objectParts_.empty()) {
    leftUpperArm->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }
  if (!leftForeArm->objectParts_.empty()) {
    leftForeArm->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }

  if (!rightUpperArm->objectParts_.empty()) {
    rightUpperArm->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }
  if (!rightForeArm->objectParts_.empty()) {
    rightForeArm->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }
  if (!leftThigh->objectParts_.empty()) {
    leftThigh->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }
  if (!leftShin->objectParts_.empty()) {
    leftShin->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }
  if (!rightThigh->objectParts_.empty()) {
    rightThigh->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }
  if (!rightShin->objectParts_.empty()) {
    rightShin->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }

  static bool logged = false;
  if (!logged) {
    Logger::Log("==== Fixed Head / Neck ====");
    Logger::Log("neck.translate : %.3f %.3f %.3f",
                neck->mainPosition.transform.translate.x,
                neck->mainPosition.transform.translate.y,
                neck->mainPosition.transform.translate.z);
    Logger::Log("neck.rotate : %.3f %.3f %.3f",
                neck->mainPosition.transform.rotate.x,
                neck->mainPosition.transform.rotate.y,
                neck->mainPosition.transform.rotate.z);
    Logger::Log("head.translate : %.3f %.3f %.3f",
                head->mainPosition.transform.translate.x,
                head->mainPosition.transform.translate.y,
                head->mainPosition.transform.translate.z);
    Logger::Log("head.rotate : %.3f %.3f %.3f",
                head->mainPosition.transform.rotate.x,
                head->mainPosition.transform.rotate.y,
                head->mainPosition.transform.rotate.z);
    Logger::Log("body.rotate.y : %.3f", body->mainPosition.transform.rotate.y);
    logged = true;
  }

  //================================
  // 固定部位と extra head を更新
  //================================
  UpdateModObjects();

  UpdateExtraVisualParts();

  // 地面のUpdate　一旦仮でここに配置
  if (ground_ != nullptr) {
    ground_->Update(usingCamera_);
  }
}

void TravelScene::UpdateSceneTransition() {

  // ゴール到達で次シーンへ
  if (!isGoalReached_ && moveX_ >= goalX_) {
    isGoalReached_ = true;

    if (!fade_.IsBusy()) {
      fade_.StartFadeOut();
      isStartTransition_ = true;
      nextOutcome_ = SceneOutcome::NEXT;
    }
  }

  // 時間制限で強制リトライ(仮)
  if (isTimeUp_ && !fade_.IsBusy()) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
    nextOutcome_ = SceneOutcome::RETRY;

    timeLimit_ = customizeData_->totalTimeLimit_;
    isTimeUp_ = false;
  }

#ifdef _DEBUG

  // デバッグ用次シーン移行処理
  if (!fade_.IsBusy() && system_->GetTriggerOn(DIK_N)) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
    nextOutcome_ = SceneOutcome::NEXT;
  }

  // デバッグ用リトライ
  if (!fade_.IsBusy() && system_->GetTriggerOn(DIK_RETURN)) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
    nextOutcome_ = SceneOutcome::RETRY;
  }

#endif

  // フェード更新
  fade_.Update(usingCamera_);

  // フェード終了後シーン遷移
  if (isStartTransition_ && fade_.IsFinished()) {
    outcome_ = nextOutcome_;

    // シーン遷移前にカスタマイズデータに時間制限関連の情報を反映させる
    if (customizeData_ != nullptr) {
      customizeData_->timeLimit_ = timeLimit_;
      customizeData_->isTimeUp_ = isTimeUp_;
      ModBody::SetSharedCustomizeData(*customizeData_);
    }
  }
}

void TravelScene::BuildFeaturesFromCustomizeData() {
  features_ = {};

  if (customizeData_ == nullptr) {
    return;
  }

  for (const auto &instance : customizeData_->partInstances) {

    // Logger::Log("---- INSTANCE ----");
    // Logger::Log("partType: %d", (int)instance.partType);
    // Logger::Log("posX: %.3f", instance.localTransform.translate.x);
    // Logger::Log("posY: %.3f", instance.localTransform.translate.y);

    if (instance.partType == ModBodyPart::Head) {
      features_.headCount++;
      // Logger::Log("HEAD COUNTED");
    }

    if (instance.partType == ModBodyPart::LeftUpperArm ||
        instance.partType == ModBodyPart::RightUpperArm) {
      features_.armCount++;
      //  Logger::Log("ARM COUNTED");
    }

    features_.centerOfMassY += instance.localTransform.translate.y;
    features_.asymmetry += std::abs(instance.localTransform.translate.x);
    features_.lowestPoint =
        std::min(features_.lowestPoint, instance.localTransform.translate.y);
  }

  // Logger::Log("==== RESULT ====");
  // Logger::Log("headCount: %d", features_.headCount);
  // Logger::Log("armCount: %d", features_.armCount);
  // Logger::Log("asymmetry: %.3f", features_.asymmetry);
  // Logger::Log("centerOfMassY: %.3f", features_.centerOfMassY);
  // Logger::Log("lowestPoint: %.3f", features_.lowestPoint);
}

void TravelScene::ClearExtraVisualParts() {
  for (auto *object : extraObjects_) {
    delete object;
  }

  extraObjects_.clear();
  extraParentIds_.clear();
}

const ModControlPointData *TravelScene::GetControlPoints() const {
  if (customizeData_ == nullptr) {
    return nullptr;
  }
  return &customizeData_->controlPoints;
}

void TravelScene::BuildExtraVisualParts() {

  const Vector3 neckScale =
      modBodies_[ToIndex(ModBodyPart::Neck)].GetVisualScaleRatio();

  // Logger::Log("neck visual scale : %.3f %.3f %.3f", neckScale.x, neckScale.y,
  //             neckScale.z);

  ClearExtraVisualParts();

  if (customizeData_ == nullptr) {
    return;
  }

  bool baseHeadSkipped = false;

  for (const auto &instance : customizeData_->partInstances) {

    if (instance.partType != ModBodyPart::Head) {
      continue;
    }

    // 既存の固定Head 1個ぶんはスキップ
    if (!baseHeadSkipped) {
      baseHeadSkipped = true;
      continue;
    }

    const int modelHandle =
        system_->SetModelObj("GAME/resources/modBody/head/head.obj");

    Object *obj = new Object;
    obj->IntObject(system_);
    obj->CreateModelData(modelHandle);
    obj->mainPosition.transform = instance.localTransform;

    //================================
    // ModBody::Apply 相当の見た目補正
    //================================
    if (!obj->objectParts_.empty()) {
      Transform &mesh = obj->objectParts_[0].transform;

      // 元のメッシュスケールを基準に param を反映
      Vector3 baseScale = mesh.scale;

      Vector3 newScale = {baseScale.x * instance.param.scale.x,
                          baseScale.y * instance.param.scale.y *
                              instance.param.length,
                          baseScale.z * instance.param.scale.z};

      if (!instance.param.enabled) {
        newScale = {0.0f, 0.0f, 0.0f};
      }

      mesh.scale = newScale;

      mesh.translate.y += newScale.y * 0.5f;

      const Vector3 neckScale =
          modBodies_[ToIndex(ModBodyPart::Neck)].GetVisualScaleRatio();

      const Vector3 neckHeadConnector = {0.0f, 0.65f, 0.0f};
      const Vector3 headRootConnector = {0.0f, 0.0f, 0.0f};

      if (instance.parentId == 2) {
        obj->mainPosition.transform.translate = {
            neckHeadConnector.x * neckScale.x -
                headRootConnector.x * newScale.x +
                instance.localTransform.translate.x,
            neckHeadConnector.y * neckScale.y -
                headRootConnector.y * newScale.y +
                instance.localTransform.translate.y,
            neckHeadConnector.z * neckScale.z -
                headRootConnector.z * newScale.z +
                instance.localTransform.translate.z};
      }
    }

    // 親設定
    auto it = fixedPartIdToPart_.find(instance.parentId);
    if (it != fixedPartIdToPart_.end()) {
      obj->followObject_ = it->second;
    }

    extraObjects_.push_back(obj);

    // Logger::Log("Extra head created");
  }
}

void TravelScene::UpdateExtraVisualParts() {
  static bool logged = false;

  for (auto *obj : extraObjects_) {
    if (obj == nullptr) {
      continue;
    }

    if (!logged) {

      logged = true;
    }

    Object *fixedHead = modObjects_[ToIndex(ModBodyPart::Head)];

    if (fixedHead != nullptr) {
      obj->mainPosition.transform.rotate =
          fixedHead->mainPosition.transform.rotate;

      if (!obj->objectParts_.empty() && !fixedHead->objectParts_.empty()) {
        obj->objectParts_[0].transform.rotate =
            fixedHead->objectParts_[0].transform.rotate;
      }
    }

    obj->Update(usingCamera_);
  }
}

void TravelScene::DrawExtraVisualParts() {
  for (auto *obj : extraObjects_) {
    if (obj != nullptr) {
      obj->Draw();
    }
  }
}

void TravelScene::UpdatePartRootsFromControlPoints() {

  const ModControlPointData *cp = GetControlPoints();
  if (cp == nullptr) {
    return;
  }

  Object *body = modObjects_[ToIndex(ModBodyPart::Body)];
  Object *neck = modObjects_[ToIndex(ModBodyPart::Neck)];
  Object *head = modObjects_[ToIndex(ModBodyPart::Head)];

  Object *leftUpperArm = modObjects_[ToIndex(ModBodyPart::LeftUpperArm)];
  Object *leftForeArm = modObjects_[ToIndex(ModBodyPart::LeftForeArm)];
  Object *rightUpperArm = modObjects_[ToIndex(ModBodyPart::RightUpperArm)];
  Object *rightForeArm = modObjects_[ToIndex(ModBodyPart::RightForeArm)];

  Object *leftThigh = modObjects_[ToIndex(ModBodyPart::LeftThigh)];
  Object *leftShin = modObjects_[ToIndex(ModBodyPart::LeftShin)];
  Object *rightThigh = modObjects_[ToIndex(ModBodyPart::RightThigh)];
  Object *rightShin = modObjects_[ToIndex(ModBodyPart::RightShin)];

  if (body == nullptr || neck == nullptr || head == nullptr ||
      leftUpperArm == nullptr || leftForeArm == nullptr ||
      rightUpperArm == nullptr || rightForeArm == nullptr ||
      leftThigh == nullptr || leftShin == nullptr || rightThigh == nullptr ||
      rightShin == nullptr) {
    return;
  }

  Logger::Log("==== BODY / CP CHECK ====");
  Logger::Log("body world : %.3f %.3f %.3f",
              body->mainPosition.transform.translate.x,
              body->mainPosition.transform.translate.y,
              body->mainPosition.transform.translate.z);

  Logger::Log("chestPos : %.3f %.3f %.3f", cp->chestPos.x, cp->chestPos.y,
              cp->chestPos.z);
  Logger::Log("bellyPos : %.3f %.3f %.3f", cp->bellyPos.x, cp->bellyPos.y,
              cp->bellyPos.z);
  Logger::Log("waistPos : %.3f %.3f %.3f", cp->waistPos.x, cp->waistPos.y,
              cp->waistPos.z);

  Logger::Log("leftHipPos : %.3f %.3f %.3f", cp->leftHipPos.x, cp->leftHipPos.y,
              cp->leftHipPos.z);
  Logger::Log("rightHipPos : %.3f %.3f %.3f", cp->rightHipPos.x,
              cp->rightHipPos.y, cp->rightHipPos.z);

  auto Sub = [](const Vector3 &a, const Vector3 &b) -> Vector3 {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
  };

  // body 自体の位置は移動処理側が持つので、ここでは触らない

  //==============================
  // 1段目 : body の子
  //==============================
  // 首と腕は Chest 基準
  neck->mainPosition.transform.translate = Sub(cp->lowerNeckPos, cp->chestPos);

  // 左上腕
  leftUpperArm->mainPosition.transform.translate =
      Sub(cp->leftShoulderPos, cp->chestPos);

  Vector3 leftUpperArmDir = Sub(cp->leftElbowPos, cp->leftShoulderPos);
  leftUpperArmDir = Normalize(leftUpperArmDir);

  float leftUpperArmAngleZ = atan2(leftUpperArmDir.x, -leftUpperArmDir.y);

  leftUpperArm->mainPosition.transform.rotate = {0.0f, 0.0f,
                                                 leftUpperArmAngleZ};

  // 右上腕
  rightUpperArm->mainPosition.transform.translate =
      Sub(cp->rightShoulderPos, cp->chestPos);

  Vector3 rightUpperArmDir = Sub(cp->rightElbowPos, cp->rightShoulderPos);
  rightUpperArmDir = Normalize(rightUpperArmDir);

  float rightUpperArmAngleZ = atan2(rightUpperArmDir.x, -rightUpperArmDir.y);

  rightUpperArm->mainPosition.transform.rotate = {0.0f, 0.0f,
                                                  rightUpperArmAngleZ};

  // 左腿
  leftThigh->mainPosition.transform.translate =
      Sub(cp->leftHipPos, cp->bellyPos);

  Vector3 leftThighDir = Sub(cp->leftKneePos, cp->leftHipPos);
  leftThighDir = Normalize(leftThighDir);

  float leftThighAngleZ = atan2(leftThighDir.x, -leftThighDir.y);

  leftThigh->mainPosition.transform.rotate = {0.0f, 0.0f, leftThighAngleZ};

  // 右腿
  rightThigh->mainPosition.transform.translate =
      Sub(cp->rightHipPos, cp->bellyPos);

  Vector3 rightThighDir = Sub(cp->rightKneePos, cp->rightHipPos);
  rightThighDir = Normalize(rightThighDir);

  float rightThighAngleZ = atan2(rightThighDir.x, -rightThighDir.y);

  rightThigh->mainPosition.transform.rotate = {0.0f, 0.0f, rightThighAngleZ};

  //==============================
  // 2段目 : 親の子
  //==============================
  // 頭
  head->mainPosition.transform.translate =
      Sub(cp->headCenterPos, cp->lowerNeckPos);

  // 左前腕
  Vector3 leftForeArmDir = Sub(cp->leftWristPos, cp->leftElbowPos);
  leftForeArmDir = Normalize(leftForeArmDir);

  float leftUpperArmLength = Length(Sub(cp->leftElbowPos, cp->leftShoulderPos));
  float leftForeArmAngleZ = atan2(leftForeArmDir.x, -leftForeArmDir.y);

  leftForeArm->mainPosition.transform.translate =
      Sub(cp->leftElbowPos, cp->chestPos);

  leftForeArm->mainPosition.transform.rotate = {0.0f, 0.0f, leftForeArmAngleZ};

  // 右前腕
  Vector3 rightForeArmDir = Sub(cp->rightWristPos, cp->rightElbowPos);
  rightForeArmDir = Normalize(rightForeArmDir);

  float rightUpperArmLength =
      Length(Sub(cp->rightElbowPos, cp->rightShoulderPos));
  float rightForeArmAngleZ = atan2(rightForeArmDir.x, -rightForeArmDir.y);

  rightForeArm->mainPosition.transform.translate =
      Sub(cp->rightElbowPos, cp->chestPos);

  rightForeArm->mainPosition.transform.rotate = {0.0f, 0.0f,
                                                 rightForeArmAngleZ};

  // 左脛
  Vector3 leftShinDir = Sub(cp->leftAnklePos, cp->leftKneePos);
  leftShinDir = Normalize(leftShinDir);

  float leftShinAngleZ = atan2(leftShinDir.x, -leftShinDir.y);

  leftShin->mainPosition.transform.translate =
      Sub(cp->leftKneePos, cp->bellyPos);

  leftShin->mainPosition.transform.rotate = {0.0f, 0.0f, leftShinAngleZ};

  // 右脛
  Vector3 rightShinDir = Sub(cp->rightAnklePos, cp->rightKneePos);
  rightShinDir = Normalize(rightShinDir);

  float rightShinAngleZ = atan2(rightShinDir.x, -rightShinDir.y);

  rightShin->mainPosition.transform.translate =
      Sub(cp->rightKneePos, cp->bellyPos);
  rightShin->mainPosition.transform.rotate = {0.0f, 0.0f, rightShinAngleZ};

  if (leftUpperArm->objectParts_.empty() || leftForeArm->objectParts_.empty() ||
      rightUpperArm->objectParts_.empty() ||
      rightForeArm->objectParts_.empty() || leftThigh->objectParts_.empty() ||
      leftShin->objectParts_.empty() || rightThigh->objectParts_.empty() ||
      rightShin->objectParts_.empty()) {
    return;
  }

  // 左上腕
  leftUpperArm->objectParts_[0].transform.translate = {
      0.0f, -leftUpperArmLength * 0.5f, 0.0f};
  leftUpperArm->objectParts_[0].transform.scale.y = leftUpperArmLength;

  // 右上腕
  rightUpperArm->objectParts_[0].transform.translate = {
      0.0f, -rightUpperArmLength * 0.5f, 0.0f};
  rightUpperArm->objectParts_[0].transform.scale.y = rightUpperArmLength;

  // 左腿
  float leftThighLength = Length(Sub(cp->leftHipPos, cp->leftKneePos));
  leftThigh->objectParts_[0].transform.translate = {
      0.0f, -leftThighLength * 0.5f, 0.0f};
  leftThigh->objectParts_[0].transform.scale.y = leftThighLength;

  // 右腿
  float rightThighLength = Length(Sub(cp->rightHipPos, cp->rightKneePos));
  rightThigh->objectParts_[0].transform.translate = {
      0.0f, -rightThighLength * 0.5f, 0.0f};
  rightThigh->objectParts_[0].transform.scale.y = rightThighLength;

  // 左前腕
  float leftForeArmLength = Length(Sub(cp->leftElbowPos, cp->leftWristPos));
  leftForeArm->objectParts_[0].transform.translate = {
      0.0f, -leftForeArmLength * 0.5f, 0.0f};
  leftForeArm->objectParts_[0].transform.scale.y = leftForeArmLength;

  // 右前腕
  float rightForeArmLength = Length(Sub(cp->rightElbowPos, cp->rightWristPos));
  rightForeArm->objectParts_[0].transform.translate = {
      0.0f, -rightForeArmLength * 0.5f, 0.0f};
  rightForeArm->objectParts_[0].transform.scale.y = rightForeArmLength;

  // 左脛
  float leftShinLength = Length(Sub(cp->leftKneePos, cp->leftAnklePos));
  leftShin->objectParts_[0].transform.translate = {0.0f, -leftShinLength * 0.5f,
                                                   0.0f};
  leftShin->objectParts_[0].transform.scale.y = leftShinLength;

  // 右脛
  float rightShinLength = Length(Sub(cp->rightKneePos, cp->rightAnklePos));
  rightShin->objectParts_[0].transform.translate = {
      0.0f, -rightShinLength * 0.5f, 0.0f};
  rightShin->objectParts_[0].transform.scale.y = rightShinLength;

  static int debugFrame = 0;
  debugFrame++;

  if (debugFrame % 30 == 0) {

    Logger::Log("==== Travel Check ====");

    // 前腕
    Logger::Log("LFore root : %.3f %.3f %.3f",
                leftForeArm->mainPosition.transform.translate.x,
                leftForeArm->mainPosition.transform.translate.y,
                leftForeArm->mainPosition.transform.translate.z);

    Logger::Log("LFore mesh : %.3f %.3f %.3f",
                leftForeArm->objectParts_[0].transform.translate.x,
                leftForeArm->objectParts_[0].transform.translate.y,
                leftForeArm->objectParts_[0].transform.translate.z);

    // 上腕
    Logger::Log("LUpper root : %.3f %.3f %.3f",
                leftUpperArm->mainPosition.transform.translate.x,
                leftUpperArm->mainPosition.transform.translate.y,
                leftUpperArm->mainPosition.transform.translate.z);

    Logger::Log("LUpper mesh : %.3f %.3f %.3f",
                leftUpperArm->objectParts_[0].transform.translate.x,
                leftUpperArm->objectParts_[0].transform.translate.y,
                leftUpperArm->objectParts_[0].transform.translate.z);

    // すね
    Logger::Log("LShin root : %.3f %.3f %.3f",
                leftShin->mainPosition.transform.translate.x,
                leftShin->mainPosition.transform.translate.y,
                leftShin->mainPosition.transform.translate.z);

    Logger::Log("LShin mesh : %.3f %.3f %.3f",
                leftShin->objectParts_[0].transform.translate.x,
                leftShin->objectParts_[0].transform.translate.y,
                leftShin->objectParts_[0].transform.translate.z);
  }
}