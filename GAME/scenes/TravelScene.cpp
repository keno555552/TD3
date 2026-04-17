#include "TravelScene.h"
#include <cmath>

namespace {
/*    enum class を配列添字に変換するための補助関数   */
size_t ToIndex(ModBodyPart part) { return static_cast<size_t>(part); }

/*   成分ごとのスケール適用   */
Vector3 ScaleByRatio(const Vector3 &base, const Vector3 &ratio) {
  return {base.x * ratio.x, base.y * ratio.y, base.z * ratio.z};
}

float RandomFloat(float minValue, float maxValue) {
  float t = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
  return minValue + (maxValue - minValue) * t;
}

std::string GetRankText(int rank) {
  switch (rank) {
  case 1:
    return "1ST";
  case 2:
    return "2ND";
  case 3:
    return "3RD";
  default:
    return std::to_string(rank) + "TH";
  }
}

} // namespace

TravelScene::TravelScene(kEngine *system) {
  Logger::Log("TravelScene ctor");
  system_ = system;

  //===============================
  // ライト
  //===============================
  light1_ = new Light;
  light1_->direction = {0.3f, -1.0f, 0.5f};
  light1_->color = {1.0f, 1.0f, 1.0f};
  light1_->intensity = 1.0f;
  system_->AddLight(light1_);

  //===============================
  // カメラ
  //===============================
  debugCamera_ = system_->CreateDebugCamera();
  camera_ = system_->CreateCamera();

  // デバッグカメラ初期位置
  debugCamera_->SetTranslate({48.0f, 5.0f, 0.0f});
  debugCamera_->SetDefaultTransform(debugCamera_->GetTransform());
  debugCamera_->SetRotation({0.0f, -1.57f, 0.0f});

  // 通常カメラ初期位置
  camera_->SetTranslate({48.0f, 5.0f, 0.0f});
  camera_->SetDefaultTransform(camera_->GetTransform());
  camera_->SetRotation({0.0f, -1.57f, 0.0f});

  usingCamera_ = camera_;
  system_->SetCamera(usingCamera_);

  //===============================
  // ビットマップフォント
  //===============================
  bitmapFont.Initialize(system_);

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

  // leftLegBend_ = legRecoverAngle_;
  // rightLegBend_ = legRecoverAngle_;
  leftLegBend_ = 0.0f;
  rightLegBend_ = 0.0f;

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

  gaitTiltTarget_ = 0.0f;
  landTimer_ = 999.0f;

  // bodyで代用中
  groundModelHandle_ =
      system_->SetModelObj("GAME/resources/modBody/body/body.obj");

  ground_ = std::make_unique<Object>();
  ground_->IntObject(system_);
  ground_->CreateModelData(groundModelHandle_);
  ground_->mainPosition.transform = CreateDefaultTransform();

  ground_->mainPosition.transform.translate = {0.0f, groundY_ - 3.5f, 0.0f};
  ground_->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  ground_->mainPosition.transform.scale = {20.0f, 0.3f, 50.0f};

  //===============================
  // 2D
  //===============================
  fade_.Initialize(system_);
  fade_.StartFadeIn();

  //===============================
  // NPC
  //===============================
  npcModelHandle_ =
      system_->SetModelObj("GAME/resources/modBody/body/body.obj");
  InitializeNpcRunners();
}

TravelScene::~TravelScene() {
  Logger::Log("TravelScene dtor");
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

  bitmapFont.Cleanup();

  ResourceManager::GetInstance()->CleanupUnusedMaterials();
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

  if (!isRaceFinished_) {

    UpdateHoldState(leftNowInput, rightNowInput, deltaTime);

    UpdateLegBendState(leftNowInput, rightNowInput);

    UpdateMovementState(leftNowInput, rightNowInput);

    UpdateNpcRunners(deltaTime);

    UpdateRaceRanking();

    UpdateRaceFinishState();
  }

  if (kickFeedbackTimer_ > 0.0f) {
    kickFeedbackTimer_ -= deltaTime;
    if (kickFeedbackTimer_ <= 0.0f) {
      kickFeedbackTimer_ = 0.0f;
      kickFeedbackType_ = KickFeedbackType::None;
    }
  }

  ApplyVisualState();

  UpdateSceneTransition();

  const ModControlPointData *cp = GetControlPoints();
  if (cp != nullptr) {
  }
}

void TravelScene::Draw() {

  DrawModObjects();

  DrawExtraVisualParts();

  for (auto &npc : npcRunners_) {
    if (npc.useCustomizedVisual) {
      DrawNpcCustomizedVisual(npc);
    } else if (npc.debugObject != nullptr) {
      npc.debugObject->Draw();
    }
  }

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
  ImGui::Text("TimeLimit : %.2f", travelTimeLimit_);
  ImGui::Text("Time Up : %s", isTimeUp_ ? "YES" : "NO");

  ImGui::End();
#endif

#ifdef USE_IMGUI
  {
    LowestBodyPart lowestPart = LowestBodyPart::None;
    float lowestBodyLocalY = GetLowestVisualBodyY(&lowestPart);
    float lowestBodyWorldY = moveY_ + visualLiftY_ + lowestBodyLocalY;
    float penetration = groundY_ - lowestBodyWorldY;

    ImGui::Begin("LowestBodyCheck");
    ImGui::Text("LowestBodyLocalY : %.3f", lowestBodyLocalY);
    ImGui::Text("LowestBodyWorldY : %.3f", lowestBodyWorldY);
    ImGui::Text("LowestPart       : %s", GetLowestBodyPartName(lowestPart));
    ImGui::Text("GroundY          : %.3f", groundY_);
    ImGui::Text("Penetration      : %.3f", penetration);
    ImGui::Text("VisualLiftY      : %.3f", visualLiftY_);
    ImGui::End();
  }
#endif

#ifdef USE_IMGUI
  ImGui::Begin("NpcDebug");

  for (size_t i = 0; i < npcRunners_.size(); ++i) {
    const auto &npc = npcRunners_[i];

    const char *resultStr = npc.lastTimingResult == 1   ? "Perfect"
                            : npc.lastTimingResult == 2 ? "Good"
                            : npc.lastTimingResult == 3 ? "Bad"
                                                        : "-";

    ImGui::Text("NPC %zu | Skill %.2f | %s", i, npc.timingSkill, resultStr);
  }

  ImGui::End();
#endif

  bitmapFont.BeginFrame();

  // 順位を表示
  std::string rankText = GetRankText(playerRank_);

  bitmapFont.RenderText(rankText, {1100, 600}, 96, BitmapFont::Align::Left);

  // 残り枠表示
  std::string goalText = "GOAL " + std::to_string(goalCount_) + "/" +
                         std::to_string(qualifyCount_);

  bitmapFont.RenderText(goalText, {1000, 60}, 48, BitmapFont::Align::Left);

  if (raceResultState_ == RaceResultState::Clear) {
    bitmapFont.RenderText("CLEAR", {900, 300}, 96, BitmapFont::Align::Left);
  } else if (raceResultState_ == RaceResultState::GameOver) {
    bitmapFont.RenderText("GAME OVER", {800, 300}, 96, BitmapFont::Align::Left);
  }

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
  // SetupPartObject(ModBodyPart::Body, "GAME/resources/modBody/body/body.obj");
  SetupPartObject(ModBodyPart::ChestBody,
                  "GAME/resources/modBody/chest/chest.obj");
  SetupPartObject(ModBodyPart::StomachBody,
                  "GAME/resources/modBody/stomach/stomach.obj");

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
      &modObjects_[ToIndex(ModBodyPart::ChestBody)]->mainPosition;
  fixedPartIdToPart_[2] =
      &modObjects_[ToIndex(ModBodyPart::StomachBody)]->mainPosition;
  fixedPartIdToPart_[3] =
      &modObjects_[ToIndex(ModBodyPart::Neck)]->mainPosition;
  fixedPartIdToPart_[4] =
      &modObjects_[ToIndex(ModBodyPart::Head)]->mainPosition;
  fixedPartIdToPart_[5] =
      &modObjects_[ToIndex(ModBodyPart::LeftUpperArm)]->mainPosition;
  fixedPartIdToPart_[6] =
      &modObjects_[ToIndex(ModBodyPart::LeftForeArm)]->mainPosition;
  fixedPartIdToPart_[7] =
      &modObjects_[ToIndex(ModBodyPart::RightUpperArm)]->mainPosition;
  fixedPartIdToPart_[8] =
      &modObjects_[ToIndex(ModBodyPart::RightForeArm)]->mainPosition;
  fixedPartIdToPart_[9] =
      &modObjects_[ToIndex(ModBodyPart::LeftThigh)]->mainPosition;
  fixedPartIdToPart_[10] =
      &modObjects_[ToIndex(ModBodyPart::LeftShin)]->mainPosition;
  fixedPartIdToPart_[11] =
      &modObjects_[ToIndex(ModBodyPart::RightThigh)]->mainPosition;
  fixedPartIdToPart_[12] =
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
  Object *chestBody = modObjects_[ToIndex(ModBodyPart::ChestBody)];
  Object *stomachBody = modObjects_[ToIndex(ModBodyPart::StomachBody)];

  if (chestBody == nullptr || stomachBody == nullptr) {
    return;
  }

  ObjectPart *chestRoot = &chestBody->mainPosition;
  ObjectPart *stomachRoot = &stomachBody->mainPosition;

  //============================
  // 胴体
  //============================
  stomachBody->followObject_ = chestRoot;
  stomachBody->mainPosition.parentPart = chestRoot;

  // 首
  modObjects_[ToIndex(ModBodyPart::Neck)]->followObject_ = chestRoot;
  modObjects_[ToIndex(ModBodyPart::Neck)]->mainPosition.parentPart = chestRoot;

  // 頭
  modObjects_[ToIndex(ModBodyPart::Head)]->followObject_ = chestRoot;
  modObjects_[ToIndex(ModBodyPart::Head)]->mainPosition.parentPart = chestRoot;

  // modObjects_[ToIndex(ModBodyPart::Head)]->followObject_ = bodyRoot;
  // modObjects_[ToIndex(ModBodyPart::Head)]->mainPosition.parentPart =
  // bodyRoot;

  // modObjects_[ToIndex(ModBodyPart::Head)]->followObject_ =
  //     &modObjects_[ToIndex(ModBodyPart::Neck)]->mainPosition;
  // modObjects_[ToIndex(ModBodyPart::Head)]->mainPosition.parentPart =
  //     &modObjects_[ToIndex(ModBodyPart::Neck)]->mainPosition;

  // 腕
  modObjects_[ToIndex(ModBodyPart::LeftUpperArm)]->followObject_ = chestRoot;
  modObjects_[ToIndex(ModBodyPart::LeftUpperArm)]->mainPosition.parentPart =
      chestRoot;

  modObjects_[ToIndex(ModBodyPart::RightUpperArm)]->followObject_ = chestRoot;
  modObjects_[ToIndex(ModBodyPart::RightUpperArm)]->mainPosition.parentPart =
      chestRoot;

  // 脚
  modObjects_[ToIndex(ModBodyPart::LeftThigh)]->followObject_ = stomachRoot;
  modObjects_[ToIndex(ModBodyPart::LeftThigh)]->mainPosition.parentPart =
      stomachRoot;

  modObjects_[ToIndex(ModBodyPart::RightThigh)]->followObject_ = stomachRoot;
  modObjects_[ToIndex(ModBodyPart::RightThigh)]->mainPosition.parentPart =
      stomachRoot;

  //============================
  // 二段目
  //============================

  // 左前腕
  // modObjects_[ToIndex(ModBodyPart::LeftForeArm)]->followObject_ =
  //    &modObjects_[ToIndex(ModBodyPart::LeftUpperArm)]->mainPosition;
  // modObjects_[ToIndex(ModBodyPart::LeftForeArm)]->mainPosition.parentPart =
  //    &modObjects_[ToIndex(ModBodyPart::LeftUpperArm)]->mainPosition;
  modObjects_[ToIndex(ModBodyPart::LeftForeArm)]->followObject_ = chestRoot;
  modObjects_[ToIndex(ModBodyPart::LeftForeArm)]->mainPosition.parentPart =
      chestRoot;

  // 右前腕
  //  modObjects_[ToIndex(ModBodyPart::RightForeArm)]->followObject_ =
  //      &modObjects_[ToIndex(ModBodyPart::RightUpperArm)]->mainPosition;
  //  modObjects_[ToIndex(ModBodyPart::RightForeArm)]->mainPosition.parentPart =
  //      &modObjects_[ToIndex(ModBodyPart::RightUpperArm)]->mainPosition;
  modObjects_[ToIndex(ModBodyPart::RightForeArm)]->followObject_ = chestRoot;
  modObjects_[ToIndex(ModBodyPart::RightForeArm)]->mainPosition.parentPart =
      chestRoot;

  // 左脛
  // modObjects_[ToIndex(ModBodyPart::LeftShin)]->followObject_ =
  //    &modObjects_[ToIndex(ModBodyPart::LeftThigh)]->mainPosition;
  // modObjects_[ToIndex(ModBodyPart::LeftShin)]->mainPosition.parentPart =
  //    &modObjects_[ToIndex(ModBodyPart::LeftThigh)]->mainPosition;
  modObjects_[ToIndex(ModBodyPart::LeftShin)]->followObject_ = stomachRoot;
  modObjects_[ToIndex(ModBodyPart::LeftShin)]->mainPosition.parentPart =
      stomachRoot;

  // 右脛
  // modObjects_[ToIndex(ModBodyPart::RightShin)]->followObject_ =
  //    &modObjects_[ToIndex(ModBodyPart::RightThigh)]->mainPosition;
  // modObjects_[ToIndex(ModBodyPart::RightShin)]->mainPosition.parentPart =
  //    &modObjects_[ToIndex(ModBodyPart::RightThigh)]->mainPosition;
  modObjects_[ToIndex(ModBodyPart::RightShin)]->followObject_ = stomachRoot;
  modObjects_[ToIndex(ModBodyPart::RightShin)]->mainPosition.parentPart =
      stomachRoot;
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
// void TravelScene::UpdateModObjects() {
//   for (size_t i = 0; i < modObjects_.size(); ++i) {
//     if (modObjects_[i] == nullptr) {
//       continue;
//     }
//
//     ModBodyPart part = static_cast<ModBodyPart>(i);
//
//     if (part == ModBodyPart::Body || part == ModBodyPart::Neck ||
//         part == ModBodyPart::Head) {
//       // modBodies_[i].Apply(modObjects_[i]);
//     }
//   }
//
//   for (auto &object : modObjects_) {
//     if (object != nullptr) {
//       object->Update(usingCamera_);
//     }
//   }
// }

void TravelScene::UpdateModObjects() {
  if (useModBodyApplyTorso_) {
    PrepareTorsoApplySource();

    Object *chestBody = modObjects_[ToIndex(ModBodyPart::ChestBody)];
    Object *stomachBody = modObjects_[ToIndex(ModBodyPart::StomachBody)];

    if (chestBody != nullptr) {
      modBodies_[ToIndex(ModBodyPart::ChestBody)].Apply(chestBody);
    }

    if (stomachBody != nullptr) {
      modBodies_[ToIndex(ModBodyPart::StomachBody)].Apply(stomachBody);
    }
  }

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
  travelTimeLimit_ = timeLimit_;
  isTimeUp_ = customizeData_->isTimeUp_;

  bodyJointOffsets_ = customizeData_->bodyJointOffsets;
  controlPointSnapshots_ = customizeData_->controlPointSnapshots;

  for (size_t i = 0; i < modBodies_.size(); ++i) {
    modBodies_[i].SetParam(customizeData_->partParams[i]);
  }

  const auto &cp = customizeData_->controlPoints;

  for (const auto &snap : customizeData_->controlPointSnapshots) {
    if (snap.ownerPartType == ModBodyPart::RightUpperArm) {
    }
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
  // 長さは scale.y で扱う
  //==============================
  const auto &chest = modBodies_[ToIndex(ModBodyPart::ChestBody)].GetParam();
  const auto &stomach =
      modBodies_[ToIndex(ModBodyPart::StomachBody)].GetParam();
  const auto &neck = modBodies_[ToIndex(ModBodyPart::Neck)].GetParam();
  const auto &head = modBodies_[ToIndex(ModBodyPart::Head)].GetParam();

  const auto &leftUpperArm =
      modBodies_[ToIndex(ModBodyPart::LeftUpperArm)].GetParam();
  const auto &rightUpperArm =
      modBodies_[ToIndex(ModBodyPart::RightUpperArm)].GetParam();
  const auto &leftForeArm =
      modBodies_[ToIndex(ModBodyPart::LeftForeArm)].GetParam();
  const auto &rightForeArm =
      modBodies_[ToIndex(ModBodyPart::RightForeArm)].GetParam();

  const auto &leftThigh =
      modBodies_[ToIndex(ModBodyPart::LeftThigh)].GetParam();
  const auto &rightThigh =
      modBodies_[ToIndex(ModBodyPart::RightThigh)].GetParam();
  const auto &leftShin = modBodies_[ToIndex(ModBodyPart::LeftShin)].GetParam();
  const auto &rightShin =
      modBodies_[ToIndex(ModBodyPart::RightShin)].GetParam();

  auto AvgXZ = [](const ModBodyPartParam &p) -> float {
    return (p.scale.x + p.scale.z) * 0.5f;
  };

  //==============================
  // 長さ（scale.y をそのまま使う）
  //==============================
  const float leftLegLength = leftThigh.scale.y + leftShin.scale.y;
  const float rightLegLength = rightThigh.scale.y + rightShin.scale.y;
  const float avgLegLength = (leftLegLength + rightLegLength) * 0.5f;

  const float neckLengthScale = neck.scale.y;

  //==============================
  // 太さ
  //==============================
  const float baseRadius = 0.1f;

  const float leftThighStartR = GetSnapshotRadius(ModBodyPart::LeftThigh, 1);
  const float leftThighBendR = GetSnapshotRadius(ModBodyPart::LeftThigh, 2);
  const float leftThighEndR = GetSnapshotRadius(ModBodyPart::LeftThigh, 3);

  const float rightThighStartR = GetSnapshotRadius(ModBodyPart::RightThigh, 1);
  const float rightThighBendR = GetSnapshotRadius(ModBodyPart::RightThigh, 2);
  const float rightThighEndR = GetSnapshotRadius(ModBodyPart::RightThigh, 3);

  // 太さは見た目と同じく snapshot 半径ベースで取る
  const float leftThighThickness =
      (std::max)(leftThighStartR, leftThighBendR) / baseRadius;
  const float leftShinThickness =
      (std::max)(leftThighBendR, leftThighEndR) / baseRadius;

  const float rightThighThickness =
      (std::max)(rightThighStartR, rightThighBendR) / baseRadius;
  const float rightShinThickness =
      (std::max)(rightThighBendR, rightThighEndR) / baseRadius;

  const float leftLegThickness =
      (leftThighThickness + leftShinThickness) * 0.5f;
  const float rightLegThickness =
      (rightThighThickness + rightShinThickness) * 0.5f;

  const float legThicknessScale = (leftLegThickness + rightLegThickness) * 0.5f;

  const float leftArmThickness =
      (AvgXZ(leftUpperArm) + AvgXZ(leftForeArm)) * 0.5f;
  const float rightArmThickness =
      (AvgXZ(rightUpperArm) + AvgXZ(rightForeArm)) * 0.5f;
  const float armThicknessScale = (leftArmThickness + rightArmThickness) * 0.5f;

  const float neckThicknessScale = AvgXZ(neck);

  //==============================
  // 頭サイズは snapshot + control point ベースで取る
  //==============================
  const float upperNeckR = GetSnapshotRadius(ModBodyPart::Head, 8);
  const float headCenterR = GetSnapshotRadius(ModBodyPart::Head, 9);

  const float headThicknessScale =
      (std::max)(upperNeckR, headCenterR) / baseRadius;

  const Vector3 &upperNeckPos = customizeData_->controlPoints.upperNeckPos;
  const Vector3 &headCenterPos = customizeData_->controlPoints.headCenterPos;

  Vector3 headVec = {headCenterPos.x - upperNeckPos.x,
                     headCenterPos.y - upperNeckPos.y,
                     headCenterPos.z - upperNeckPos.z};

  const float currentHeadLength = Length(headVec);
  const float baseHeadLength = 0.55f;

  float headLengthScale = 1.0f;
  if (currentHeadLength > 0.0001f) {
    headLengthScale = currentHeadLength / baseHeadLength;
  }

  // 太さ寄りで平均
  const float headSizeScale =
      (headThicknessScale * 2.0f + headLengthScale) / 3.0f;

  headSizeScale_ = headSizeScale;

  //==============================
  // 胴体サイズ
  // 旧: 太さだけ
  // 新: 頭と同じ考え方で「太さ + 長さ」
  //==============================
  const float chestR = GetControlPointRadius(ModControlPointRole::Chest);
  const float bellyR = GetControlPointRadius(ModControlPointRole::Belly);
  const float waistR = GetControlPointRadius(ModControlPointRole::Waist);

  const float chestThicknessScale = (std::max)(chestR, bellyR) / baseRadius;
  const float stomachThicknessScale = (std::max)(bellyR, waistR) / baseRadius;

  const Vector3 &chestPos = customizeData_->controlPoints.chestPos;
  const Vector3 &bellyPos = customizeData_->controlPoints.bellyPos;
  const Vector3 &waistPos = customizeData_->controlPoints.waistPos;

  Vector3 chestToBelly = {bellyPos.x - chestPos.x, bellyPos.y - chestPos.y,
                          bellyPos.z - chestPos.z};
  Vector3 bellyToWaist = {waistPos.x - bellyPos.x, waistPos.y - bellyPos.y,
                          waistPos.z - bellyPos.z};

  const float chestLength = Length(chestToBelly);
  const float stomachLength = Length(bellyToWaist);

  // 無改造基準は今の見た目に合わせた仮値
  const float baseChestLength = 0.45f;
  const float baseStomachLength = 0.45f;

  float chestLengthScale = 1.0f;
  if (chestLength > 0.0001f) {
    chestLengthScale = chestLength / baseChestLength;
  }

  float stomachLengthScale = 1.0f;
  if (stomachLength > 0.0001f) {
    stomachLengthScale = stomachLength / baseStomachLength;
  }

  // 頭と同じく「太さ寄り」で平均
  const float chestScale =
      (chestThicknessScale * 2.0f + chestLengthScale) / 3.0f;
  const float stomachScale =
      (stomachThicknessScale * 2.0f + stomachLengthScale) / 3.0f;

  const float torsoScaleAvg = (chestScale + stomachScale) * 0.5f;

  torsoSizeScale_ = torsoScaleAvg;

  torsoStabilityScale_ =
      std::clamp(1.0f + (torsoScaleAvg - 1.0f) * 3.00f, 0.45f, 4.20f);

  torsoTiltResistance_ =
      std::clamp(1.0f - (torsoScaleAvg - 1.0f) * 1.80f, 0.10f, 1.15f);

  //==============================
  // 左右差
  //==============================
  const float legDiff = std::abs(leftLegLength - rightLegLength);

  //==============================
  // 細長さ
  //==============================
  const float legSlimness = avgLegLength / (legThicknessScale + 0.001f);

  //==============================
  // ベース
  //==============================
  tuning_.runPower = 0.75f;
  tuning_.maxSpeed = 1.0f;
  tuning_.stability = 1.0f;
  tuning_.lift = 1.0f;
  tuning_.turnResponse = 1.0f;

  //==============================
  // 前進力
  //==============================
  tuning_.runPower += (avgLegLength - 1.0f) * 2.2f;
  tuning_.runPower -= (legThicknessScale - 1.0f) * 1.2f;
  tuning_.runPower -= legDiff * 1.8f;

  //==============================
  // 最高速
  //==============================
  tuning_.maxSpeed += (avgLegLength - 1.0f) * 2.7f;
  tuning_.maxSpeed -= (legThicknessScale - 1.0f) * 0.9f;
  tuning_.maxSpeed -= (headSizeScale - 1.0f) * 0.5f;
  tuning_.maxSpeed -= legDiff * 1.6f;

  //==============================
  // 安定性
  //==============================
  tuning_.stability += (legThicknessScale - 1.0f) * 1.5f;
  tuning_.stability += (armThicknessScale - 1.0f) * 0.5f;
  tuning_.stability += (neckThicknessScale - 1.0f) * 0.7f;

  tuning_.stability -= (legSlimness - 1.0f) * 1.5f;
  tuning_.stability -= (neckLengthScale - 1.0f) * 1.2f;
  tuning_.stability -= (headSizeScale - 1.0f) * 1.0f;
  tuning_.stability -= legDiff * 1.8f;
  tuning_.stability -= features_.asymmetry * 0.35f;

  //==============================
  // 上方向
  //==============================
  tuning_.lift += (avgLegLength - 1.0f) * 2.3f;
  tuning_.lift -= (legThicknessScale - 1.0f) * 0.7f;
  tuning_.lift -= (headSizeScale - 1.0f) * 0.3f;

  float comOffset = std::clamp(features_.centerOfMassY, -3.0f, 3.0f);
  tuning_.lift += comOffset * 0.2f;

  //==============================
  // 傾きやすさ
  //==============================
  tuning_.turnResponse += (legSlimness - 1.0f) * 1.2f;
  tuning_.turnResponse += legDiff * 2.0f;
  tuning_.turnResponse += (neckLengthScale - 1.0f) * 0.8f;
  tuning_.turnResponse += features_.asymmetry * 0.5f;

  //==============================
  // 追加パーツの影響
  //==============================
  float extraHeadCount = (std::max)(0.0f, features_.headCount - 1.0f);
  float extraArmCount = (std::max)(0.0f, features_.armCount - 2.0f);

  tuning_.stability -= extraHeadCount * 0.8f;
  tuning_.turnResponse += extraHeadCount * 0.8f;
  tuning_.lift += extraHeadCount * 0.2f;

  tuning_.stability -= extraArmCount * 0.15f;
  tuning_.runPower += extraArmCount * 0.1f;

  //==============================
  // クランプ
  //==============================
  tuning_.runPower = std::clamp(tuning_.runPower, 0.1f, 4.0f);
  tuning_.maxSpeed = std::clamp(tuning_.maxSpeed, 0.2f, 4.5f);
  tuning_.stability = std::clamp(tuning_.stability, 0.1f, 3.5f);
  tuning_.lift = std::clamp(tuning_.lift, 0.2f, 3.5f);
  tuning_.turnResponse = std::clamp(tuning_.turnResponse, 0.2f, 4.0f);

  leftLegReturnScale_ =
      std::clamp(1.0f - (leftLegThickness - 1.0f) * 0.20f, 0.55f, 1.0f);

  rightLegReturnScale_ =
      std::clamp(1.0f - (rightLegThickness - 1.0f) * 0.20f, 0.55f, 1.0f);

  // 安定してる体ほどタイミング判定を広く
  timingWindowScale_ = std::clamp(0.8f + tuning_.stability * 0.25f -
                                      tuning_.turnResponse * 0.08f,
                                  0.55f, 1.25f);

  // 安定してる体ほど起き上がりやすい
  recoveryAssist_ = std::clamp(0.7f + tuning_.stability * 0.25f, 0.6f, 1.4f);
}

float TravelScene::ComputeLegHeightOffset() const {
  const ModControlPointData *cp = GetControlPoints();
  if (cp == nullptr) {
    return 0.0f;
  }

  auto Sub = [](const Vector3 &a, const Vector3 &b) -> Vector3 {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
  };

  const float leftThighLength = Length(Sub(cp->leftKneePos, cp->leftHipPos));
  const float leftShinLength = Length(Sub(cp->leftAnklePos, cp->leftKneePos));

  const float rightThighLength = Length(Sub(cp->rightKneePos, cp->rightHipPos));
  const float rightShinLength =
      Length(Sub(cp->rightAnklePos, cp->rightKneePos));

  const float leftLegLength = leftThighLength + leftShinLength;
  const float rightLegLength = rightThighLength + rightShinLength;
  const float avgLegLength = (leftLegLength + rightLegLength) * 0.5f;

  // 無改造基準
  const float baseThighLength = 1.25f;
  const float baseShinLength = 1.0f;
  const float baseLegLength = baseThighLength + baseShinLength;

  return avgLegLength - baseLegLength;
}

bool TravelScene::HasRequiredParts() const {
  Object *chestBody = modObjects_[ToIndex(ModBodyPart::ChestBody)];
  Object *stomachBody = modObjects_[ToIndex(ModBodyPart::StomachBody)];
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
  if (chestBody == nullptr || neck == nullptr || head == nullptr ||
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
    if (!fade_.IsBusy()) {
      timeLimit_ -= deltaTime;
    }

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

  float leftFollow = legFollowPower_;
  float rightFollow = legFollowPower_;

  float leftMaxSpeed = legMaxSpeed_;
  float rightMaxSpeed = legMaxSpeed_;

  float tiltDiff = bodyTilt_ - idealRunTilt_;
  float forwardLean = (std::max)(0.0f, -tiltDiff);

  // 前傾しすぎるほど脚を前に戻しづらくする
  float returnPenalty = 1.0f - forwardLean * 0.65f;
  returnPenalty = std::clamp(returnPenalty, 0.35f, 1.0f);

  // 脚を前へ戻す時だけ重さを強く反映
  if (!leftNowInput) {
    leftFollow *= leftLegReturnScale_ * leftLegReturnScale_ *
                  leftLegReturnScale_ * 0.25f * returnPenalty;
    leftMaxSpeed *= leftLegReturnScale_ * leftLegReturnScale_ *
                    leftLegReturnScale_ * returnPenalty;
  }
  if (!rightNowInput) {
    rightFollow *= rightLegReturnScale_ * rightLegReturnScale_ *
                   rightLegReturnScale_ * 0.25f * returnPenalty;
    rightMaxSpeed *= rightLegReturnScale_ * rightLegReturnScale_ *
                     rightLegReturnScale_ * returnPenalty;
  }

  leftLegBendSpeed_ += (leftTargetLegAngle - leftLegBend_) * leftFollow;
  rightLegBendSpeed_ += (rightTargetLegAngle - rightLegBend_) * rightFollow;

  leftLegBendSpeed_ =
      std::clamp(leftLegBendSpeed_, -leftMaxSpeed, leftMaxSpeed);
  rightLegBendSpeed_ =
      std::clamp(rightLegBendSpeed_, -rightMaxSpeed, rightMaxSpeed);

  leftLegBendSpeed_ *= jointDamping_;
  rightLegBendSpeed_ *= jointDamping_;
  bodyStretchSpeed_ *= jointDamping_;

  leftLegBend_ += leftLegBendSpeed_;
  rightLegBend_ += rightLegBendSpeed_;
  bodyStretch_ += bodyStretchSpeed_;

  leftLegBend_ = std::clamp(leftLegBend_, -0.8f, 1.0f);
  rightLegBend_ = std::clamp(rightLegBend_, -0.8f, 1.0f);

  bodyStretch_ = std::clamp(bodyStretch_, -0.5f, 1.0f);
}

void TravelScene::UpdateMovementState(bool leftNowInput, bool rightNowInput) {

  bool bothInput = leftNowInput && rightNowInput;

  kickFeedbackType_ = KickFeedbackType::None;

  float stability = useCustomizeMove_ ? tuning_.stability : 1.0f;
  float turnResponse = useCustomizeMove_ ? tuning_.turnResponse : 1.0f;

  const float recoverStartTilt = 0.42f;
  const float recoverTargetTilt = -0.12f;
  const float heavyFallTilt = 0.65f;

  isRecoveringFromTilt_ = (std::abs(bodyTilt_) > recoverStartTilt);

  //==============================
  // 姿勢：通常時は中立へ戻すだけ
  // 蹴った瞬間にだけ bodyTiltVelocity_ を入れる
  //==============================
  const float neutralTilt = -0.03f;

  float headRecoveryPenalty =
      std::clamp(1.0f - (headSizeScale_ - 1.0f) * 0.55f, 0.25f, 1.0f);

  // 常時戻しを弱くして、姿勢を少し引きずるようにする
  float neutralReturnPower =
      0.020f * stability * headRecoveryPenalty * torsoStabilityScale_;

  if (bothInput) {
    neutralReturnPower *= 0.80f;
  }

  // 空中ではさらに戻りにくくする
  if (!isGrounded_) {
    neutralReturnPower *= 0.35f;
  }

  bodyTiltVelocity_ += (neutralTilt - bodyTilt_) * neutralReturnPower;

  //==============================
  // 姿勢の良し悪し
  //==============================
  float postureError = std::abs(bodyTilt_ - idealRunTilt_);
  float badPosture = std::clamp(postureError / postureTolerance_, 0.0f, 1.0f);
  float forwardRate = 1.0f - badPosture;

  //==============================
  // 入力保持
  //==============================
  bool leftHoldEnough = (leftHoldTime_ >= minHoldTimeToKick_);
  bool rightHoldEnough = (rightHoldTime_ >= minHoldTimeToKick_);

  float leftPushCandidate = 0.0f;
  float rightPushCandidate = 0.0f;

  bool useLeftPush = false;
  bool useRightPush = false;
  float adoptedDriveUse = 0.0f;

  float singleStepBasePush = 0.12f;
  float singleStepJumpScale = 0.7f;

  //==============================
  // 左脚候補
  //==============================
  if (leftNowInput && isGrounded_ && velocityY_ <= 0.0f &&
      !requireReleaseAfterLandLeft_) {

    float sequenceBonus = 1.0f;
    if (lastKickSide_ == 1) {
      sequenceBonus = alternateKickBonus_;
    } else if (lastKickSide_ == -1) {
      sequenceBonus = sameSideKickPenalty_;
    }

    float driveUse = 0.0f;
    if (leftDriveAccum_ >= minDriveToKick_ && leftHoldEnough) {
      driveUse = (std::min)(leftDriveAccum_, driveUsePerFrame_);
    }

    float drivePush = driveUse * drivePushScale_ * pushPower_ * groundAssist_ *
                      groundedKickFactor_ * 2.0f * sequenceBonus;

    float basePush = singleStepBasePush * pushPower_ * sequenceBonus;

    leftPushCandidate = (std::max)(basePush, drivePush);
  }

  //==============================
  // 右脚候補
  //==============================
  if (rightNowInput && isGrounded_ && velocityY_ <= 0.0f &&
      !requireReleaseAfterLandRight_) {

    float sequenceBonus = 1.0f;
    if (lastKickSide_ == -1) {
      sequenceBonus = alternateKickBonus_;
    } else if (lastKickSide_ == 1) {
      sequenceBonus = sameSideKickPenalty_;
    }

    float driveUse = 0.0f;
    if (rightDriveAccum_ >= minDriveToKick_ && rightHoldEnough) {
      driveUse = (std::min)(rightDriveAccum_, driveUsePerFrame_);
    }

    float drivePush = driveUse * drivePushScale_ * pushPower_ * groundAssist_ *
                      groundedKickFactor_ * 2.0f * sequenceBonus;

    float basePush = singleStepBasePush * pushPower_ * sequenceBonus;

    rightPushCandidate = (std::max)(basePush, drivePush);
  }

  //==============================
  // 採用する蹴りを決定
  //==============================
  float totalPush = 0.0f;

  if (leftPushCandidate >= rightPushCandidate && leftPushCandidate > 0.0f) {
    totalPush = leftPushCandidate;
    useLeftPush = true;
    adoptedDriveUse = (std::min)(leftDriveAccum_, driveUsePerFrame_);
    lastKickSide_ = -1;

    leftDriveAccum_ -= adoptedDriveUse;
    leftDriveAccum_ = (std::max)(0.0f, leftDriveAccum_);

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

  //==============================
  // 脚の戻り具合で蹴り威力を補正
  //==============================
  float kickLegBendNow = 0.0f;

  if (useLeftPush) {
    kickLegBendNow = leftLegBend_;
  } else if (useRightPush) {
    kickLegBendNow = rightLegBend_;
  }

  // 戻り具合（0〜1）
  float kickReadyRatio = std::clamp((kickLegBendNow - legKickAngle_) /
                                        (legRecoverAngle_ - legKickAngle_),
                                    0.0f, 1.0f);

  // 少ししか戻していない蹴りはほぼ無意味にする
  const float minKickReady = 0.35f;

  float normalizedKickReady = std::clamp(
      (kickReadyRatio - minKickReady) / (1.0f - minKickReady), 0.0f, 1.0f);

  // 二乗で弱い蹴りをさらに弱くする
  float kickReadyPower = normalizedKickReady * normalizedKickReady;

  // 姿勢が悪いほど蹴りの力が地面に乗らない
  float postureKickScale = 1.0f - badPosture * 0.55f;
  postureKickScale = std::clamp(postureKickScale, 0.35f, 1.0f);

  // 前傾しすぎは特に蹴りづらくする
  float overForwardPenalty = 1.0f;
  if (bodyTilt_ < idealRunTilt_) {
    float forwardOver =
        std::clamp((idealRunTilt_ - bodyTilt_) / 0.22f, 0.0f, 1.0f);
    overForwardPenalty = 1.0f - forwardOver * 0.35f;
  }

  totalPush *= kickReadyPower;
  // totalPush *= postureKickScale;
  // totalPush *= overForwardPenalty;

  bool startStepTrigger = isGrounded_ && landTimer_ > 0.30f && totalPush > 0.0f;

  //==============================
  // 着地タイミングで push を補正
  //==============================
  if (totalPush > 0.0f) {

    // gaitTiltTarget_ = std::clamp(gaitTiltTarget_, -0.75f, 0.75f);

    // const float bestTimingEnd = 0.08f;
    // const float lateTimingEnd = 0.22f;

    // if (!isGrounded_) {
    //   // 空中入力：ほぼ無効
    //   totalPush *= 0.20f;

    //} else if (landTimer_ <= bestTimingEnd) {
    //  // ベスト：少し前傾寄り
    //  gaitTiltTarget_ += (bestTilt - gaitTiltTarget_) * 0.25f;

    //} else if (landTimer_ <= lateTimingEnd) {
    //  // 遅い：少し後ろ寄り
    //  float lateRatio =
    //      (landTimer_ - bestTimingEnd) / (lateTimingEnd - bestTimingEnd);
    //  lateRatio = std::clamp(lateRatio, 0.0f, 1.0f);

    //  totalPush *= (0.90f - lateRatio * 0.20f);

    //  float targetLateTilt = lateTilt + lateRatio * 0.03f;
    //  gaitTiltTarget_ += (targetLateTilt - gaitTiltTarget_) * 0.20f;

    //} else {
    //  // 遅すぎる：かなり後ろ寄り
    //  totalPush *= 0.20f;
    //  gaitTiltTarget_ += (veryLateTilt - gaitTiltTarget_) * 0.20f;
    //}

    const float bestTimingEnd = 0.08f;
    const float lateTimingEnd = 0.22f;

    const float perfectTimingEnd = bestTimingEnd * 0.45f;

    float tiltImpulse = 0.0f;

    if (!startStepTrigger && isGrounded_) {
      if (landTimer_ <= perfectTimingEnd) {
        kickFeedbackType_ = KickFeedbackType::Perfect;
        kickFeedbackTimer_ = 0.18f;

        Logger::Log("KICK : PERFECT");

      } else if (landTimer_ <= bestTimingEnd) {
        kickFeedbackType_ = KickFeedbackType::Good;
        kickFeedbackTimer_ = 0.12f;

        Logger::Log("KICK : GOOD");

      } else {
        Logger::Log("KICK : BAD");
      }
    }

    float perfectBonus = 1.0f;

    if (kickFeedbackType_ == KickFeedbackType::Perfect) {
      perfectBonus = 1.25f;
    }

    totalPush *= perfectBonus;

    if (startStepTrigger) {
      tiltImpulse = 0.0f;

    } else if (!isGrounded_) {
      // 空中入力：ほぼ無効
      // totalPush *= 0.20f;
      tiltImpulse = 0.0f;

    } else if (landTimer_ <= bestTimingEnd) {
      // ベスト：蹴った瞬間に少し前傾へ入る
      tiltImpulse = -0.020f;

    } else if (landTimer_ <= lateTimingEnd) {
      // 遅い：少しずつ後傾へ
      float lateRatio =
          (landTimer_ - bestTimingEnd) / (lateTimingEnd - bestTimingEnd);
      lateRatio = std::clamp(lateRatio, 0.0f, 1.0f);

      // totalPush *= (0.90f - lateRatio * 0.20f);
      tiltImpulse = 0.010f + lateRatio * 0.020f;

    } else {
      // 遅すぎる：かなり後傾、前進も弱い
      // totalPush *= 0.20f;
      tiltImpulse = 0.035f;
    }

    float headHeavyFactor =
        std::clamp(1.0f + (headSizeScale_ - 1.0f) * 3.0f, 1.0f, 5.0f);

    bodyTiltVelocity_ +=
        tiltImpulse * turnResponse * headHeavyFactor * torsoTiltResistance_;

    const Vector3 lt = modObjects_[ToIndex(ModBodyPart::LeftThigh)]
                           ->objectParts_[0]
                           .transform.scale;
    const Vector3 rt = modObjects_[ToIndex(ModBodyPart::RightThigh)]
                           ->objectParts_[0]
                           .transform.scale;
    const Vector3 ls = modObjects_[ToIndex(ModBodyPart::LeftShin)]
                           ->objectParts_[0]
                           .transform.scale;
    const Vector3 rs = modObjects_[ToIndex(ModBodyPart::RightShin)]
                           ->objectParts_[0]
                           .transform.scale;

    float avgLegScaleY = (lt.y + rt.y + ls.y + rs.y) * 0.25f;
    float legLengthScale =
        std::clamp(1.0f + (avgLegScaleY - 1.0f) * 0.75f, 0.45f, 2.0f);

    float kickLegBend = 0.0f;
    if (useLeftPush) {
      kickLegBend = leftLegBend_;
    } else if (useRightPush) {
      kickLegBend = rightLegBend_;
    }

    float kickLegForwardness =
        1.0f - std::clamp((kickLegBend - legKickAngle_) /
                              (legRecoverAngle_ - legKickAngle_),
                          0.0f, 1.0f);

    float kickEfficiency = 0.75f + kickLegForwardness * 0.75f;
    float groundBoost = isGrounded_ ? 1.2f : 0.6f;

    float runPower = useCustomizeMove_ ? tuning_.runPower : 1.0f;
    float lift = useCustomizeMove_ ? tuning_.lift : 1.0f;

    //==============================
    // まず総量だけ決める
    //==============================
    float pushMagnitude = totalPush *
                          (0.55f + runPower * 1.10f + lift * 0.75f) *
                          legLengthScale * kickEfficiency * groundBoost;

    if (bothInput) {
      pushMagnitude *= 0.85f;
    } else {
      pushMagnitude *= 1.10f;
    }

    //==============================
    // 姿勢から方向だけ決める
    //==============================
    float tiltDiff = bodyTilt_ - idealRunTilt_;

    float forwardRatio = 0.5f - tiltDiff * 0.25f;
    float upwardRatio = 0.5f + tiltDiff * 0.25f;

    // 後傾側だけ追加で前進を削る
    if (tiltDiff > 0.0f) {
      float backwardPenalty = std::clamp(tiltDiff / 0.22f, 0.0f, 1.0f);
      forwardRatio *= (1.0f - backwardPenalty * 0.55f);
    }

    forwardRatio = std::clamp(forwardRatio, 0.15f, 1.50f);
    upwardRatio = std::clamp(upwardRatio, 0.15f, 1.50f);

    float dirLen =
        std::sqrt(forwardRatio * forwardRatio + upwardRatio * upwardRatio);

    if (dirLen > 0.0001f) {
      forwardRatio /= dirLen;
      upwardRatio /= dirLen;
    } else {
      forwardRatio = 0.707f;
      upwardRatio = 0.707f;
    }

    float pushX = pushMagnitude * forwardRatio;
    float pushY = pushMagnitude * upwardRatio;

    velocityX_ += pushX;
    velocityY_ += pushY;
  }

  //==============================
  // 蹴りにならなかった側は減衰
  //==============================
  if (!(leftNowInput && isGrounded_ && velocityY_ <= 0.0f &&
        !requireReleaseAfterLandLeft_)) {
    leftDriveAccum_ = (std::max)(0.0f, leftDriveAccum_ - driveDecay_);
  }

  if (!(rightNowInput && isGrounded_ && velocityY_ <= 0.0f &&
        !requireReleaseAfterLandRight_)) {
    rightDriveAccum_ = (std::max)(0.0f, rightDriveAccum_ - driveDecay_);
  }

  //==============================
  // 姿勢更新
  //==============================
  float tiltDamping =
      std::clamp(0.82f + (torsoSizeScale_ - 1.0f) * 0.08f, 0.76f, 0.92f);
  bodyTiltVelocity_ *= tiltDamping;

  float headTiltRangeFactor =
      std::clamp(1.0f + (headSizeScale_ - 1.0f) * 1.60f, 1.0f, 3.20f);

  float torsoTiltRangeFactor =
      std::clamp(1.0f - (torsoSizeScale_ - 1.0f) * 0.45f, 0.65f, 1.10f);

  float dynamicForwardTilt =
      maxForwardTilt_ * headTiltRangeFactor * torsoTiltRangeFactor;
  float dynamicBackwardTilt =
      maxBackwardTilt_ * headTiltRangeFactor * torsoTiltRangeFactor;

  bodyTilt_ += bodyTiltVelocity_;
  bodyTilt_ = std::clamp(bodyTilt_, dynamicForwardTilt, dynamicBackwardTilt);

  if (debugForceTilt_) {
    bodyTilt_ = debugTiltValue_;
  }

  if (isRecoveringFromTilt_) {
    float recoverPower =
        0.010f * recoveryAssist_ * headRecoveryPenalty * torsoStabilityScale_;

    if (std::abs(bodyTilt_) > heavyFallTilt) {
      recoverPower *= 1.4f;
      velocityX_ *= 0.96f;
    }

    float tiltToTarget = recoverTargetTilt - bodyTilt_;
    bodyTiltVelocity_ += tiltToTarget * recoverPower;
  }

  //==============================
  // 接地中の drive 蓄積
  //==============================
  float leftExtendAmount = 0.0f;
  float rightExtendAmount = 0.0f;
  float leftRecoverAmount = 0.0f;
  float rightRecoverAmount = 0.0f;

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

  //==============================
  // 速度更新
  //==============================
  bool wasGrounded = isGrounded_;

  velocityX_ *= inertia_;

  float maxSpeed = useCustomizeMove_ ? tuning_.maxSpeed : 1.0f;
  velocityX_ = std::clamp(velocityX_, -1.2f * maxSpeed, 1.2f * maxSpeed);

  moveX_ += velocityX_;

  velocityY_ -= gravity_;
  moveY_ += velocityY_;

  // if (moveY_ <= groundY_) {
  //   isGrounded_ = true;
  //   moveY_ = groundY_;
  //   velocityY_ = 0.0f;
  // } else {
  //   isGrounded_ = false;
  // }

  const ModControlPointData *cp = GetControlPoints();

  if (cp != nullptr && customizeData_ != nullptr) {
    Vector3 leftHipAnchorLocal = {-0.5f, -1.25f, 0.0f};
    Vector3 rightHipAnchorLocal = {0.5f, -1.25f, 0.0f};

    Vector3 leftLegRootLocal = {0.0f, 0.0f, 0.0f};
    Vector3 leftLegEndLocal = {0.0f, -1.40f, 0.0f};

    Vector3 rightLegRootLocal = {0.0f, 0.0f, 0.0f};
    Vector3 rightLegEndLocal = {0.0f, -1.40f, 0.0f};

    int torsoAnchorOwnerId = -1;
    int leftThighOwnerId = -1;
    int rightThighOwnerId = -1;

    for (const auto &instance : customizeData_->partInstances) {
      if (torsoAnchorOwnerId < 0 &&
          instance.partType == ModBodyPart::ChestBody) {
        torsoAnchorOwnerId = instance.partId;
      }
      if (leftThighOwnerId < 0 && instance.partType == ModBodyPart::LeftThigh) {
        leftThighOwnerId = instance.partId;
      }
      if (rightThighOwnerId < 0 &&
          instance.partType == ModBodyPart::RightThigh) {
        rightThighOwnerId = instance.partId;
      }
    }

    for (const auto &snap : customizeData_->controlPointSnapshots) {
      if (snap.ownerPartId == torsoAnchorOwnerId) {
        if (snap.role == ModControlPointRole::LeftHip) {
          leftHipAnchorLocal = snap.localPosition;
        } else if (snap.role == ModControlPointRole::RightHip) {
          rightHipAnchorLocal = snap.localPosition;
        }
      }

      if (snap.ownerPartId == leftThighOwnerId) {
        if (snap.role == ModControlPointRole::Root) {
          leftLegRootLocal = snap.localPosition;
        } else if (snap.role == ModControlPointRole::End) {
          leftLegEndLocal = snap.localPosition;
        }
      }

      if (snap.ownerPartId == rightThighOwnerId) {
        if (snap.role == ModControlPointRole::Root) {
          rightLegRootLocal = snap.localPosition;
        } else if (snap.role == ModControlPointRole::End) {
          rightLegEndLocal = snap.localPosition;
        }
      }
    }

    const float leftAnkleRadius = GetSnapshotRadius(ModBodyPart::LeftThigh, 3);
    const float rightAnkleRadius =
        GetSnapshotRadius(ModBodyPart::RightThigh, 3);

    // 実際の見た目配置に合わせた ankle 下端
    const float leftFootBottomLocalY =
        leftHipAnchorLocal.y + (leftLegEndLocal.y - leftLegRootLocal.y) -
        leftAnkleRadius;

    const float rightFootBottomLocalY =
        rightHipAnchorLocal.y + (rightLegEndLocal.y - rightLegRootLocal.y) -
        rightAnkleRadius;

    const float lowestLegBottomLocalY =
        (std::min)(leftFootBottomLocalY, rightFootBottomLocalY);

    const float groundEpsilon = 0.01f;
    const float contactMoveY = groundY_ - lowestLegBottomLocalY;

    if (moveY_ <= contactMoveY + groundEpsilon) {
      isGrounded_ = true;
      moveY_ = contactMoveY;
      velocityY_ = 0.0f;
    } else {
      isGrounded_ = false;
    }

  } else {
    if (moveY_ <= groundY_) {
      isGrounded_ = true;
      moveY_ = groundY_;
      velocityY_ = 0.0f;
    } else {
      isGrounded_ = false;
    }
  }

  bool justLanded = (!wasGrounded && isGrounded_);

  if (justLanded) {
    landTimer_ = 0.0f;
  } else if (isGrounded_) {
    landTimer_ += system_->GetDeltaTime();
  } else {
    landTimer_ = 999.0f;
  }

  if (justLanded) {
    leftDriveAccum_ = 0.0f;
    rightDriveAccum_ = 0.0f;

    leftHoldTime_ = 0.0f;
    rightHoldTime_ = 0.0f;

    requireReleaseAfterLandLeft_ = leftNowInput;
    requireReleaseAfterLandRight_ = rightNowInput;
  }

  bodyStretch_ *= 0.90f;

  leftPrevInput_ = leftNowInput;
  rightPrevInput_ = rightNowInput;
}

void TravelScene::ApplyVisualState() {
  //================================
  // 体全体の位置・姿勢
  //================================

  // Object *body = modObjects_[ToIndex(ModBodyPart::Body)];
  Object *chestBody = modObjects_[ToIndex(ModBodyPart::ChestBody)];
  Object *stomachBody = modObjects_[ToIndex(ModBodyPart::StomachBody)];

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

  // body->mainPosition.transform.translate.x = moveX_;
  // const float legHeightOffset = ComputeLegHeightOffset();
  // body->mainPosition.transform.translate.y = moveY_ + legHeightOffset;
  // body->mainPosition.transform.translate.y = moveY_;

  // body->mainPosition.transform.rotate.x = 0.0f;
  // body->mainPosition.transform.rotate.y = 1.57f;
  // body->mainPosition.transform.rotate.z = bodyTilt_;

  // body->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  // body->mainPosition.transform.scale.x = 1.0f;
  // body->mainPosition.transform.scale.y =
  //     (std::max)(0.65f, 1.0f - bodyStretch_ * 0.2f);
  // body->mainPosition.transform.scale.z = 1.0f;

  if (chestBody != nullptr) {
    chestBody->mainPosition.transform.translate.x = 0.0f;
    chestBody->mainPosition.transform.translate.y = moveY_ + visualLiftY_;
    chestBody->mainPosition.transform.translate.z = moveX_;

    chestBody->mainPosition.transform.rotate = {-bodyTilt_, 0.0f, 0.0f};
  }

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
    // leftThigh->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }
  if (!leftShin->objectParts_.empty()) {
    // leftShin->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }
  if (!rightThigh->objectParts_.empty()) {
    // rightThigh->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }
  if (!rightShin->objectParts_.empty()) {
    // rightShin->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  }

  //================================
  // 固定部位と extra head を更新
  //================================
  UpdateModObjects();

  UpdateExtraVisualParts();

  ResolveVisualGroundPenetration();

  // 地面のUpdate　一旦仮でここに配置
  if (ground_ != nullptr) {
    constexpr float groundTopOffset = 0.5f;

    ground_->mainPosition.transform.translate.x = 0.0f;
    ground_->mainPosition.transform.translate.y = groundY_ - groundTopOffset;
    ground_->mainPosition.transform.translate.z = 0.0f;

    ground_->Update(usingCamera_);
  }
}

void TravelScene::UpdateSceneTransition() {

  //================================
  // レース結果による遷移
  // Clear    -> 次シーン
  // GameOver -> リトライ
  //================================
  if (isRaceFinished_ && !fade_.IsBusy() && !isStartTransition_) {
    if (raceResultState_ == RaceResultState::Clear) {
      fade_.StartFadeOut();
      isStartTransition_ = true;
      nextOutcome_ = SceneOutcome::NEXT;
    } else if (raceResultState_ == RaceResultState::GameOver) {
      fade_.StartFadeOut();
      isStartTransition_ = true;
      nextOutcome_ = SceneOutcome::RETRY;

      // リトライ用に時間だけ戻しておく
      timeLimit_ = travelTimeLimit_;
      isTimeUp_ = false;
    }
  }

  //================================
  // 時間切れでもリトライ
  //================================
  // if (!isRaceFinished_ && isTimeUp_ && !fade_.IsBusy() &&
  // !isStartTransition_) {
  //  fade_.StartFadeOut();
  //  isStartTransition_ = true;
  //  nextOutcome_ = SceneOutcome::RETRY;

  //  timeLimit_ = travelTimeLimit_;
  //  isTimeUp_ = false;
  //}

#ifdef _DEBUG

  // デバッグ用次シーン移行
  if (!fade_.IsBusy() && !isStartTransition_ && system_->GetTriggerOn(DIK_N)) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
    nextOutcome_ = SceneOutcome::NEXT;
  }

  // デバッグ用リトライ
  if (!fade_.IsBusy() && !isStartTransition_ &&
      system_->GetTriggerOn(DIK_RETURN)) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
    nextOutcome_ = SceneOutcome::RETRY;

    timeLimit_ = travelTimeLimit_;
    isTimeUp_ = false;
  }

#endif

  //================================
  // フェード更新
  //================================
  fade_.Update(usingCamera_);

  //================================
  // フェード終了後シーン遷移
  //================================
  if (isStartTransition_ && fade_.IsFinished()) {
    outcome_ = nextOutcome_;

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

    if (instance.partType == ModBodyPart::Head) {
      features_.headCount++;
    }

    if (instance.partType == ModBodyPart::LeftUpperArm ||
        instance.partType == ModBodyPart::RightUpperArm) {
      features_.armCount++;
    }

    features_.centerOfMassY += instance.localTransform.translate.y;
    features_.asymmetry += std::abs(instance.localTransform.translate.x);
    features_.lowestPoint =
        std::min(features_.lowestPoint, instance.localTransform.translate.y);
  }
}

void TravelScene::ClearExtraVisualParts() {
  for (auto *object : extraObjects_) {
    delete object;
  }

  extraObjects_.clear();
  extraParentIds_.clear();
  extraPartTypes_.clear();
  extraPartIds_.clear();
}

void TravelScene::CollectSnapshotsByOwnerId(
    int ownerPartId,
    std::vector<const ModControlPointSnapshot *> &outSnapshots) const {

  outSnapshots.clear();

  if (customizeData_ == nullptr) {
    return;
  }

  for (const auto &snap : customizeData_->controlPointSnapshots) {
    if (snap.ownerPartId == ownerPartId) {
      outSnapshots.push_back(&snap);
    }
  }
}

Vector3 TravelScene::BuildAnimatedChildRootFromParent(const Vector3 &root,
                                                      float angleZ,
                                                      float angleX,
                                                      float length) const {
  Vector3 result{};
  result.x = root.x + std::sin(angleZ) * std::cos(angleX) * length;
  result.y = root.y - std::cos(angleZ) * std::cos(angleX) * length;
  result.z = root.z - std::sin(angleX) * length;
  return result;
}

bool TravelScene::GetExtraPartSnapshotPositions(int partId, Vector3 &outRoot,
                                                Vector3 &outBend,
                                                Vector3 &outEnd) const {
  if (customizeData_ == nullptr) {
    return false;
  }

  bool hasRoot = false;
  bool hasBend = false;
  bool hasEnd = false;

  for (const auto &snap : customizeData_->controlPointSnapshots) {
    if (snap.ownerPartId != partId) {
      continue;
    }

    if (snap.role == ModControlPointRole::Root) {
      outRoot = snap.localPosition;
      hasRoot = true;
    } else if (snap.role == ModControlPointRole::Bend) {
      outBend = snap.localPosition;
      hasBend = true;
    } else if (snap.role == ModControlPointRole::End) {
      outEnd = snap.localPosition;
      hasEnd = true;
    }
  }

  return hasRoot && hasBend && hasEnd;
}

bool TravelScene::GetExtraPartParentObject(
    ModBodyPart partType, int parentId,
    const std::unordered_map<int, Object *> &extraPartObjectMap,
    Object *&outParent) const {
  outParent = nullptr;

  auto it = extraPartObjectMap.find(parentId);
  if (it != extraPartObjectMap.end()) {
    outParent = it->second;
    return true;
  }

  switch (partType) {
  case ModBodyPart::LeftForeArm:
    outParent = modObjects_[ToIndex(ModBodyPart::LeftUpperArm)];
    return outParent != nullptr;

  case ModBodyPart::RightForeArm:
    outParent = modObjects_[ToIndex(ModBodyPart::RightUpperArm)];
    return outParent != nullptr;

  case ModBodyPart::LeftShin:
    outParent = modObjects_[ToIndex(ModBodyPart::LeftThigh)];
    return outParent != nullptr;

  case ModBodyPart::RightShin:
    outParent = modObjects_[ToIndex(ModBodyPart::RightThigh)];
    return outParent != nullptr;

  default:
    return false;
  }
}

int TravelScene::GetExtraSnapshotOwnerId(ModBodyPart partType, int partId,
                                         int parentId) const {
  switch (partType) {
  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
    if (parentId >= 0) {
      return parentId;
    }
    return partId;

  default:
    return partId;
  }
}

bool TravelScene::ComputeExtraBaseAngles(ModBodyPart partType,
                                         int snapshotOwnerId,
                                         float &outBaseAngleX,
                                         float &outBaseAngleZ) const {
  Vector3 snapRoot = {0.0f, 0.0f, 0.0f};
  Vector3 snapBend = {0.0f, -0.5f, 0.0f};
  Vector3 snapEnd = {0.0f, -1.0f, 0.0f};

  if (!GetExtraPartSnapshotPositions(snapshotOwnerId, snapRoot, snapBend,
                                     snapEnd)) {
    return false;
  }

  Vector3 from = snapRoot;
  Vector3 to = snapBend;

  switch (partType) {
  case ModBodyPart::LeftForeArm:
  case ModBodyPart::RightForeArm:
  case ModBodyPart::LeftShin:
  case ModBodyPart::RightShin:
    from = snapBend;
    to = snapEnd;
    break;

  default:
    from = snapRoot;
    to = snapBend;
    break;
  }

  Vector3 vec = {to.x - from.x, to.y - from.y, to.z - from.z};
  float length = Length(vec);

  if (length < 0.0001f) {
    vec = {0.0f, -1.0f, 0.0f};
  } else {
    vec = Normalize(vec);
  }

  outBaseAngleZ = atan2(vec.x, -vec.y);
  outBaseAngleX = -asinf(std::clamp(vec.z, -1.0f, 1.0f));
  return true;
}

float TravelScene::ComputeExtraAnimAngleX(ModBodyPart partType) const {
  const float armSwingScale = 0.60f;
  const float thighSwingScale = 0.70f;

  switch (partType) {
  case ModBodyPart::LeftUpperArm:
    return -rightLegBend_ * armSwingScale;

  case ModBodyPart::RightUpperArm:
    return -leftLegBend_ * armSwingScale;

  case ModBodyPart::LeftForeArm: {
    float upperArmSwing = -rightLegBend_ * armSwingScale;
    float elbowFold = std::clamp((rightLegBend_ - legKickAngle_) /
                                     (legRecoverAngle_ - legKickAngle_),
                                 0.0f, 1.0f);
    return -(upperArmSwing * 0.35f + elbowFold * 0.45f);
  }

  case ModBodyPart::RightForeArm: {
    float upperArmSwing = -leftLegBend_ * armSwingScale;
    float elbowFold = std::clamp((leftLegBend_ - legKickAngle_) /
                                     (legRecoverAngle_ - legKickAngle_),
                                 0.0f, 1.0f);
    return -(upperArmSwing * 0.35f + elbowFold * 0.45f);
  }

  case ModBodyPart::LeftThigh:
    return -leftLegBend_ * thighSwingScale;

  case ModBodyPart::RightThigh:
    return -rightLegBend_ * thighSwingScale;

  case ModBodyPart::LeftShin: {
    float thighSwing = -leftLegBend_ * thighSwingScale;
    float kneeFold = std::clamp((leftLegBend_ - legKickAngle_) /
                                    (legRecoverAngle_ - legKickAngle_),
                                0.0f, 1.0f);
    return thighSwing * 0.35f + kneeFold * 0.6f;
  }

  case ModBodyPart::RightShin: {
    float thighSwing = -rightLegBend_ * thighSwingScale;
    float kneeFold = std::clamp((rightLegBend_ - legKickAngle_) /
                                    (legRecoverAngle_ - legKickAngle_),
                                0.0f, 1.0f);
    return thighSwing * 0.35f + kneeFold * 0.6f;
  }

  default:
    return 0.0f;
  }
}

const ModControlPointData *TravelScene::GetControlPoints() const {
  if (customizeData_ == nullptr) {
    return nullptr;
  }
  return &customizeData_->controlPoints;
}

void TravelScene::BuildExtraVisualParts() {
  auto Sub = [](const Vector3 &a, const Vector3 &b) -> Vector3 {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
  };

  ClearExtraVisualParts();

  if (customizeData_ == nullptr) {
    return;
  }

  std::unordered_map<int, ObjectPart *> builtPartIdToPart;

  bool baseHeadSkipped = false;
  bool baseNeckSkipped = false;
  bool baseRightUpperArmSkipped = false;
  bool baseRightForeArmSkipped = false;
  bool baseLeftUpperArmSkipped = false;
  bool baseLeftForeArmSkipped = false;
  bool baseLeftThighSkipped = false;
  bool baseLeftShinSkipped = false;
  bool baseRightThighSkipped = false;
  bool baseRightShinSkipped = false;

  for (const auto &instance : customizeData_->partInstances) {

    if (instance.partType != ModBodyPart::Head &&
        instance.partType != ModBodyPart::Neck &&
        instance.partType != ModBodyPart::RightUpperArm &&
        instance.partType != ModBodyPart::RightForeArm &&
        instance.partType != ModBodyPart::LeftUpperArm &&
        instance.partType != ModBodyPart::LeftForeArm &&
        instance.partType != ModBodyPart::LeftThigh &&
        instance.partType != ModBodyPart::LeftShin &&
        instance.partType != ModBodyPart::RightThigh &&
        instance.partType != ModBodyPart::RightShin) {
      continue;
    }

    // 固定1個目はスキップ
    if (instance.partType == ModBodyPart::Head) {
      if (!baseHeadSkipped) {
        baseHeadSkipped = true;
        continue;
      }
    }

    if (instance.partType == ModBodyPart::Neck) {
      if (!baseNeckSkipped) {
        baseNeckSkipped = true;
        continue;
      }
    }

    if (instance.partType == ModBodyPart::RightUpperArm) {
      if (!baseRightUpperArmSkipped) {
        baseRightUpperArmSkipped = true;
        continue;
      }
    }

    if (instance.partType == ModBodyPart::RightForeArm) {
      if (!baseRightForeArmSkipped) {
        baseRightForeArmSkipped = true;
        continue;
      }
    }

    if (instance.partType == ModBodyPart::LeftUpperArm) {
      if (!baseLeftUpperArmSkipped) {
        baseLeftUpperArmSkipped = true;
        continue;
      }
    }

    if (instance.partType == ModBodyPart::LeftForeArm) {
      if (!baseLeftForeArmSkipped) {
        baseLeftForeArmSkipped = true;
        continue;
      }
    }

    if (instance.partType == ModBodyPart::LeftThigh) {
      if (!baseLeftThighSkipped) {
        baseLeftThighSkipped = true;
        continue;
      }
    }

    if (instance.partType == ModBodyPart::LeftShin) {
      if (!baseLeftShinSkipped) {
        baseLeftShinSkipped = true;
        continue;
      }
    }

    if (instance.partType == ModBodyPart::RightThigh) {
      if (!baseRightThighSkipped) {
        baseRightThighSkipped = true;
        continue;
      }
    }

    if (instance.partType == ModBodyPart::RightShin) {
      if (!baseRightShinSkipped) {
        baseRightShinSkipped = true;
        continue;
      }
    }

    std::string modelPath;
    if (instance.partType == ModBodyPart::Head) {
      modelPath = "GAME/resources/modBody/head/head.obj";
    } else if (instance.partType == ModBodyPart::Neck) {
      modelPath = "GAME/resources/modBody/neck/neck.obj";
    } else if (instance.partType == ModBodyPart::RightUpperArm) {
      modelPath = "GAME/resources/modBody/rightUpperArm/rightUpperArm.obj";
    } else if (instance.partType == ModBodyPart::RightForeArm) {
      modelPath = "GAME/resources/modBody/rightForeArm/rightForeArm.obj";
    } else if (instance.partType == ModBodyPart::LeftUpperArm) {
      modelPath = "GAME/resources/modBody/leftUpperArm/leftUpperArm.obj";
    } else if (instance.partType == ModBodyPart::LeftForeArm) {
      modelPath = "GAME/resources/modBody/leftForeArm/leftForeArm.obj";
    } else if (instance.partType == ModBodyPart::LeftThigh) {
      modelPath = "GAME/resources/modBody/leftThighs/leftThighs.obj";
    } else if (instance.partType == ModBodyPart::LeftShin) {
      modelPath = "GAME/resources/modBody/leftShin/leftShin.obj";
    } else if (instance.partType == ModBodyPart::RightThigh) {
      modelPath = "GAME/resources/modBody/rightThighs/rightThighs.obj";
    } else if (instance.partType == ModBodyPart::RightShin) {
      modelPath = "GAME/resources/modBody/rightShin/rightShin.obj";
    } else {
      continue;
    }

    const int modelHandle = system_->SetModelObj(modelPath);

    Object *obj = new Object;
    obj->IntObject(system_);
    obj->CreateModelData(modelHandle);

    // まずは localTransform をそのまま使う
    obj->mainPosition.transform = instance.localTransform;

    // 親設定
    if (instance.partType == ModBodyPart::Head ||
        instance.partType == ModBodyPart::Neck) {
      obj->followObject_ =
          &modObjects_[ToIndex(ModBodyPart::ChestBody)]->mainPosition;
      obj->mainPosition.parentPart =
          &modObjects_[ToIndex(ModBodyPart::ChestBody)]->mainPosition;
    } else if (instance.partType == ModBodyPart::RightForeArm ||
               instance.partType == ModBodyPart::LeftForeArm) {
      obj->followObject_ =
          &modObjects_[ToIndex(ModBodyPart::ChestBody)]->mainPosition;
      obj->mainPosition.parentPart =
          &modObjects_[ToIndex(ModBodyPart::ChestBody)]->mainPosition;
    } else if (instance.partType == ModBodyPart::LeftShin ||
               instance.partType == ModBodyPart::RightShin) {
      obj->followObject_ =
          &modObjects_[ToIndex(ModBodyPart::StomachBody)]->mainPosition;
      obj->mainPosition.parentPart =
          &modObjects_[ToIndex(ModBodyPart::StomachBody)]->mainPosition;
    } else {
      auto extraIt = builtPartIdToPart.find(instance.parentId);
      if (extraIt != builtPartIdToPart.end()) {
        obj->followObject_ = extraIt->second;
        obj->mainPosition.parentPart = extraIt->second;
      } else {
        auto fixedIt = fixedPartIdToPart_.find(instance.parentId);
        if (fixedIt != fixedPartIdToPart_.end()) {
          obj->followObject_ = fixedIt->second;
          obj->mainPosition.parentPart = fixedIt->second;
        }
      }
    }

    if (!obj->objectParts_.empty()) {
      Transform &mesh = obj->objectParts_[0].transform;
      const Vector3 baseScale = mesh.scale;

      //============================
      // Head
      //============================
      if (instance.partType == ModBodyPart::Head) {

        std::vector<const ModControlPointSnapshot *> snaps;
        snaps.reserve(customizeData_->controlPointSnapshots.size());

        for (const auto &snap : customizeData_->controlPointSnapshots) {
          if (snap.ownerPartId == instance.partId) {
            snaps.push_back(&snap);
          }
        }

        const ModControlPointSnapshot *upperSnap = nullptr;
        const ModControlPointSnapshot *headCenterSnap = nullptr;

        for (const auto *snap : snaps) {
          if (snap->role == ModControlPointRole::Bend) {
            upperSnap = snap;
          }
          if (snap->role == ModControlPointRole::End) {
            headCenterSnap = snap;
          }
        }

        if (upperSnap == nullptr || headCenterSnap == nullptr) {
          mesh.scale = baseScale;
          mesh.translate = {0.0f, 0.0f, 0.0f};
        } else {
          //================================
          // 長さ・向き
          // Head は UpperNeck -> HeadCenter
          //================================
          Vector3 headVector =
              Sub(headCenterSnap->localPosition, upperSnap->localPosition);
          float headLength = Length(headVector);

          if (headLength < 0.0001f) {
            headLength = 0.0001f;
            headVector = {0.0f, 1.0f, 0.0f};
          }

          Vector3 headDir = Normalize(headVector);
          float headAngleZ = atan2(headDir.x, -headDir.y);

          //================================
          // 太さ
          //================================
          const float baseRadius = 0.1f;
          float headThicknessScale =
              (std::max)(upperSnap->radius, headCenterSnap->radius) /
              baseRadius;

          if (headThicknessScale < 0.0001f) {
            headThicknessScale = 0.0001f;
          }

          const auto &headParam =
              modBodies_[ToIndex(ModBodyPart::Head)].GetParam();

          mesh.scale.x = headThicknessScale * headParam.scale.x;
          mesh.scale.y = headLength * headParam.scale.y * headParam.length;
          mesh.scale.z = headThicknessScale * headParam.scale.z;

          // 頭メッシュを区間の中央に置く
          mesh.translate = {0.0f, -headLength * 0.5f, 0.0f};

          if (!instance.param.enabled) {
            mesh.scale = {0.0f, 0.0f, 0.0f};
          }

          // root は UpperNeck
          obj->mainPosition.transform.translate = upperSnap->localPosition;
          obj->mainPosition.transform.rotate = {0.0f, 0.0f, headAngleZ};
        }
      }
      //============================
      // RightUpperArm
      //============================
      else if (instance.partType == ModBodyPart::RightUpperArm) {

        std::vector<const ModControlPointSnapshot *> snaps;
        snaps.reserve(customizeData_->controlPointSnapshots.size());

        for (const auto &snap : customizeData_->controlPointSnapshots) {
          if (snap.ownerPartId == instance.partId) {
            snaps.push_back(&snap);
          }
        }

        const ModControlPointSnapshot *bendSnap = nullptr;
        const ModControlPointSnapshot *endSnap = nullptr;

        for (const auto *snap : snaps) {
          if (snap->role == ModControlPointRole::Bend) {
            bendSnap = snap;
          }
          if (snap->role == ModControlPointRole::End) {
            endSnap = snap;
          }
        }

        if (bendSnap == nullptr || endSnap == nullptr) {

          mesh.scale = baseScale;
          mesh.translate = {0.0f, -baseScale.y * 0.5f, 0.0f};

        } else {
          const ModControlPointSnapshot *rootSnap = nullptr;

          for (const auto *snap : snaps) {
            if (snap->role == ModControlPointRole::Root) {
              rootSnap = snap;
            }
            if (snap->role == ModControlPointRole::Bend) {
              bendSnap = snap;
            }
          }

          //================================
          // 長さ・向き
          // 上腕は Root -> Bend
          //================================
          Vector3 armVector = {0.0f, -1.0f, 0.0f};
          float armLength = 0.0001f;
          float armAngleZ = 0.0f;
          float armAngleX = 0.0f;

          if (rootSnap != nullptr && bendSnap != nullptr) {
            armVector = Sub(bendSnap->localPosition, rootSnap->localPosition);
            armLength = Length(armVector);

            if (armLength < 0.0001f) {
              armLength = 0.0001f;
              armVector = {0.0f, -1.0f, 0.0f};
            }

            Vector3 armDir = Normalize(armVector);
            armAngleZ = atan2(armDir.x, -armDir.y);
            armAngleX = -asinf(std::clamp(armDir.z, -1.0f, 1.0f));

            obj->mainPosition.transform.translate =
                instance.localTransform.translate;
          } else {
            mesh.scale = baseScale;
            mesh.translate = {0.0f, -baseScale.y * 0.5f, 0.0f};
          }

          //================================
          // 太さ
          // fixed の RightUpperArm と同じ考え方に揃える
          //================================
          float startRadius = 0.1f;
          float bendRadius = 0.1f;

          for (const auto *snap : snaps) {
            if (snap->role == ModControlPointRole::Root) {
              startRadius = snap->radius;
            }
            if (snap->role == ModControlPointRole::Bend) {
              bendRadius = snap->radius;
            }
          }

          const auto &rightUpperArmParam =
              modBodies_[ToIndex(ModBodyPart::RightUpperArm)].GetParam();

          const float baseRadius = 0.1f;
          const float rightUpperArmThicknessScale =
              (std::max)(startRadius, bendRadius) / baseRadius;

          mesh.scale.x =
              rightUpperArmThicknessScale * rightUpperArmParam.scale.x;
          mesh.scale.y = armLength * rightUpperArmParam.scale.y *
                         rightUpperArmParam.length;
          mesh.scale.z =
              rightUpperArmThicknessScale * rightUpperArmParam.scale.z;

          mesh.translate = {0.0f, -armLength * 0.5f, 0.0f};

          obj->mainPosition.transform.rotate = {armAngleX, 0.0f, armAngleZ};
        }
      } else if (instance.partType == ModBodyPart::LeftUpperArm) {

        std::vector<const ModControlPointSnapshot *> snaps;
        snaps.reserve(customizeData_->controlPointSnapshots.size());

        for (const auto &snap : customizeData_->controlPointSnapshots) {
          if (snap.ownerPartId == instance.partId) {
            snaps.push_back(&snap);
          }
        }

        const ModControlPointSnapshot *bendSnap = nullptr;
        const ModControlPointSnapshot *endSnap = nullptr;

        for (const auto *snap : snaps) {
          if (snap->role == ModControlPointRole::Bend) {
            bendSnap = snap;
          }
          if (snap->role == ModControlPointRole::End) {
            endSnap = snap;
          }
        }

        if (bendSnap == nullptr || endSnap == nullptr) {

          mesh.scale = baseScale;
          mesh.translate = {0.0f, -baseScale.y * 0.5f, 0.0f};

        } else {
          const ModControlPointSnapshot *rootSnap = nullptr;

          for (const auto *snap : snaps) {
            if (snap->role == ModControlPointRole::Root) {
              rootSnap = snap;
            }
            if (snap->role == ModControlPointRole::Bend) {
              bendSnap = snap;
            }
          }

          //================================
          // 長さ・向き
          // 上腕は Root -> Bend
          //================================
          Vector3 armVector = {0.0f, -1.0f, 0.0f};
          float armLength = 0.0001f;
          float armAngleZ = 0.0f;
          float armAngleX = 0.0f;

          if (rootSnap != nullptr && bendSnap != nullptr) {
            armVector = Sub(bendSnap->localPosition, rootSnap->localPosition);
            armLength = Length(armVector);

            if (armLength < 0.0001f) {
              armLength = 0.0001f;
              armVector = {0.0f, -1.0f, 0.0f};
            }

            Vector3 armDir = Normalize(armVector);
            armAngleZ = atan2(armDir.x, -armDir.y);
            armAngleX = -asinf(std::clamp(armDir.z, -1.0f, 1.0f));

            obj->mainPosition.transform.translate =
                instance.localTransform.translate;
          } else {
            mesh.scale = baseScale;
            mesh.translate = {0.0f, -baseScale.y * 0.5f, 0.0f};
          }

          //================================
          // 太さ
          // fixed の LeftUpperArm と同じ考え方に揃える
          //================================
          float startRadius = 0.1f;
          float bendRadius = 0.1f;

          for (const auto *snap : snaps) {
            if (snap->role == ModControlPointRole::Root) {
              startRadius = snap->radius;
            }
            if (snap->role == ModControlPointRole::Bend) {
              bendRadius = snap->radius;
            }
          }

          const auto &leftUpperArmParam =
              modBodies_[ToIndex(ModBodyPart::LeftUpperArm)].GetParam();

          const float baseRadius = 0.1f;
          const float leftUpperArmThicknessScale =
              (std::max)(startRadius, bendRadius) / baseRadius;

          mesh.scale.x = leftUpperArmThicknessScale * leftUpperArmParam.scale.x;
          mesh.scale.y =
              armLength * leftUpperArmParam.scale.y * leftUpperArmParam.length;
          mesh.scale.z = leftUpperArmThicknessScale * leftUpperArmParam.scale.z;

          mesh.translate = {0.0f, -armLength * 0.5f, 0.0f};

          obj->mainPosition.transform.rotate = {armAngleX, 0.0f, armAngleZ};
        }
      } else if (instance.partType == ModBodyPart::RightForeArm) {

        int ownerPartId = instance.partId;

        // 右前腕の control owner は親の右上腕
        if (instance.parentId >= 0) {
          ownerPartId = instance.parentId;
        }

std::vector<const ModControlPointSnapshot *> snaps;
        CollectSnapshotsByOwnerId(ownerPartId, snaps);

        const ModControlPointSnapshot *bendSnap = nullptr;
        const ModControlPointSnapshot *endSnap = nullptr;

        for (const auto *snap : snaps) {
          if (snap->role == ModControlPointRole::Bend) {
            bendSnap = snap;
          }
          if (snap->role == ModControlPointRole::End) {
            endSnap = snap;
          }
        }

        if (bendSnap == nullptr || endSnap == nullptr) {

          mesh.scale = baseScale;
          mesh.translate = {0.0f, -baseScale.y * 0.5f, 0.0f};

        } else {
          //================================
          // 長さ
          //================================
          Vector3 armVector =
              Sub(endSnap->localPosition, bendSnap->localPosition);
          float armLength = Length(armVector);

          if (armLength < 0.0001f) {
            armLength = 0.0001f;
            armVector = {0.0f, -1.0f, 0.0f};
          }

          Vector3 armDir = Normalize(armVector);
          float armAngleZ = atan2(armDir.x, -armDir.y);
          float armAngleX = -asinf(std::clamp(armDir.z, -1.0f, 1.0f));

          //================================
          // 太さ
          // fixed の RightForeArm と同じ考え方に揃える
          //================================
          float bendRadius = bendSnap->radius;
          float endRadius = endSnap->radius;

          const auto &rightForeArmParam =
              modBodies_[ToIndex(ModBodyPart::RightForeArm)].GetParam();

          const float baseRadius = 0.1f;
          const float rightForeArmThicknessScale =
              (std::max)(bendRadius, endRadius) / baseRadius;

          mesh.scale.x = rightForeArmThicknessScale * rightForeArmParam.scale.x;
          mesh.scale.y =
              armLength * rightForeArmParam.scale.y * rightForeArmParam.length;
          mesh.scale.z = rightForeArmThicknessScale * rightForeArmParam.scale.z;

          mesh.translate = {0.0f, -armLength * 0.5f, 0.0f};

          // 右前腕の root は肘位置
          Vector3 ownerTranslate = {0.0f, 0.0f, 0.0f};

          // owner の追加右上腕 instance の root 位置を取る
          for (const auto &ownerInstance : customizeData_->partInstances) {
            if (ownerInstance.partId == ownerPartId) {
              ownerTranslate = ownerInstance.localTransform.translate;
              break;
            }
          }

          const ModControlPointSnapshot *rootSnap = nullptr;
          for (const auto *snap : snaps) {
            if (snap->role == ModControlPointRole::Root) {
              rootSnap = snap;
              break;
            }
          }

          if (rootSnap != nullptr) {
            Vector3 elbowOffset =
                Sub(bendSnap->localPosition, rootSnap->localPosition);

            obj->mainPosition.transform.translate = {
                ownerTranslate.x + elbowOffset.x,
                ownerTranslate.y + elbowOffset.y,
                ownerTranslate.z + elbowOffset.z};
          } else {
            obj->mainPosition.transform.translate = ownerTranslate;
          }
          obj->mainPosition.transform.rotate = {armAngleX, 0.0f, armAngleZ};
        }
      } else if (instance.partType == ModBodyPart::LeftForeArm) {

        int ownerPartId = instance.partId;

        // 左前腕の control owner は親の左上腕
        if (instance.parentId >= 0) {
          ownerPartId = instance.parentId;
        }

std::vector<const ModControlPointSnapshot *> snaps;
        CollectSnapshotsByOwnerId(ownerPartId, snaps);

        const ModControlPointSnapshot *bendSnap = nullptr;
        const ModControlPointSnapshot *endSnap = nullptr;

        for (const auto *snap : snaps) {
          if (snap->role == ModControlPointRole::Bend) {
            bendSnap = snap;
          }
          if (snap->role == ModControlPointRole::End) {
            endSnap = snap;
          }
        }

        if (bendSnap == nullptr || endSnap == nullptr) {

          mesh.scale = baseScale;
          mesh.translate = {0.0f, -baseScale.y * 0.5f, 0.0f};

        } else {
          //================================
          // 長さ
          //================================
          Vector3 armVector =
              Sub(endSnap->localPosition, bendSnap->localPosition);
          float armLength = Length(armVector);

          if (armLength < 0.0001f) {
            armLength = 0.0001f;
            armVector = {0.0f, -1.0f, 0.0f};
          }

          Vector3 armDir = Normalize(armVector);
          float armAngleZ = atan2(armDir.x, -armDir.y);
          float armAngleX = -asinf(std::clamp(armDir.z, -1.0f, 1.0f));

          //================================
          // 太さ
          // fixed の LeftForeArm と同じ考え方に揃える
          //================================
          float bendRadius = bendSnap->radius;
          float endRadius = endSnap->radius;

          const auto &leftForeArmParam =
              modBodies_[ToIndex(ModBodyPart::LeftForeArm)].GetParam();

          const float baseRadius = 0.1f;
          const float leftForeArmThicknessScale =
              (std::max)(bendRadius, endRadius) / baseRadius;

          mesh.scale.x = leftForeArmThicknessScale * leftForeArmParam.scale.x;
          mesh.scale.y =
              armLength * leftForeArmParam.scale.y * leftForeArmParam.length;
          mesh.scale.z = leftForeArmThicknessScale * leftForeArmParam.scale.z;

          mesh.translate = {0.0f, -armLength * 0.5f, 0.0f};

          // 左前腕の root は肘位置
          Vector3 ownerTranslate = {0.0f, 0.0f, 0.0f};

          for (const auto &ownerInstance : customizeData_->partInstances) {
            if (ownerInstance.partId == ownerPartId) {
              ownerTranslate = ownerInstance.localTransform.translate;
              break;
            }
          }

          const ModControlPointSnapshot *rootSnap = nullptr;
          for (const auto *snap : snaps) {
            if (snap->role == ModControlPointRole::Root) {
              rootSnap = snap;
              break;
            }
          }

          if (rootSnap != nullptr) {
            Vector3 elbowOffset =
                Sub(bendSnap->localPosition, rootSnap->localPosition);

            obj->mainPosition.transform.translate = {
                ownerTranslate.x + elbowOffset.x,
                ownerTranslate.y + elbowOffset.y,
                ownerTranslate.z + elbowOffset.z};
          } else {
            obj->mainPosition.transform.translate = ownerTranslate;
          }

          obj->mainPosition.transform.rotate = {armAngleX, 0.0f, armAngleZ};
        }
      } else if (instance.partType == ModBodyPart::LeftThigh) {

        std::vector<const ModControlPointSnapshot *> snaps;
        snaps.reserve(customizeData_->controlPointSnapshots.size());

        for (const auto &snap : customizeData_->controlPointSnapshots) {
          if (snap.ownerPartId == instance.partId) {
            snaps.push_back(&snap);
          }
        }

        const ModControlPointSnapshot *bendSnap = nullptr;
        const ModControlPointSnapshot *endSnap = nullptr;

        for (const auto *snap : snaps) {
          if (snap->role == ModControlPointRole::Bend) {
            bendSnap = snap;
          }
          if (snap->role == ModControlPointRole::End) {
            endSnap = snap;
          }
        }

        if (bendSnap == nullptr || endSnap == nullptr) {

          mesh.scale = baseScale;
          mesh.translate = {0.0f, -baseScale.y * 0.5f, 0.0f};

        } else {
          const ModControlPointSnapshot *rootSnap = nullptr;

          for (const auto *snap : snaps) {
            if (snap->role == ModControlPointRole::Root) {
              rootSnap = snap;
            }
            if (snap->role == ModControlPointRole::Bend) {
              bendSnap = snap;
            }
          }

          Vector3 legVector = {0.0f, -1.0f, 0.0f};
          float legLength = 0.0001f;
          float legAngleZ = 0.0f;

          if (rootSnap != nullptr && bendSnap != nullptr) {
            legVector = Sub(bendSnap->localPosition, rootSnap->localPosition);
            legLength = Length(legVector);

            if (legLength < 0.0001f) {
              legLength = 0.0001f;
              legVector = {0.0f, -1.0f, 0.0f};
            }

            Vector3 legDir = Normalize(legVector);
            legAngleZ = atan2(legDir.x, -legDir.y);

            obj->mainPosition.transform.translate =
                instance.localTransform.translate;
          } else {
            mesh.scale = baseScale;
            mesh.translate = {0.0f, -baseScale.y * 0.5f, 0.0f};
          }

          float startRadius = 0.1f;
          float bendRadius = 0.1f;

          for (const auto *snap : snaps) {
            if (snap->role == ModControlPointRole::Root) {
              startRadius = snap->radius;
            }
            if (snap->role == ModControlPointRole::Bend) {
              bendRadius = snap->radius;
            }
          }

          const auto &leftThighParam =
              modBodies_[ToIndex(ModBodyPart::LeftThigh)].GetParam();

          const float baseRadius = 0.1f;
          const float leftThighThicknessScale =
              (std::max)(startRadius, bendRadius) / baseRadius;

          mesh.scale.x = leftThighThicknessScale * leftThighParam.scale.x;
          mesh.scale.y =
              legLength * leftThighParam.scale.y * leftThighParam.length;
          mesh.scale.z = leftThighThicknessScale * leftThighParam.scale.z;

          mesh.translate = {0.0f, -legLength * 0.5f, 0.0f};

          obj->mainPosition.transform.rotate = {0.0f, 0.0f, legAngleZ};
        }
      } else if (instance.partType == ModBodyPart::LeftShin) {

        int ownerPartId = instance.partId;

        // 左すねの control owner は親の左腿
        if (instance.parentId >= 0) {
          ownerPartId = instance.parentId;
        }

std::vector<const ModControlPointSnapshot *> snaps;
        CollectSnapshotsByOwnerId(ownerPartId, snaps);

        const ModControlPointSnapshot *bendSnap = nullptr;
        const ModControlPointSnapshot *endSnap = nullptr;

        for (const auto *snap : snaps) {
          if (snap->role == ModControlPointRole::Bend) {
            bendSnap = snap;
          }
          if (snap->role == ModControlPointRole::End) {
            endSnap = snap;
          }
        }

        if (bendSnap == nullptr || endSnap == nullptr) {

          mesh.scale = baseScale;
          mesh.translate = {0.0f, -baseScale.y * 0.5f, 0.0f};

        } else {
          Vector3 legVector =
              Sub(endSnap->localPosition, bendSnap->localPosition);
          float legLength = Length(legVector);

          if (legLength < 0.0001f) {
            legLength = 0.0001f;
            legVector = {0.0f, -1.0f, 0.0f};
          }

          Vector3 legDir = Normalize(legVector);
          float legAngleZ = atan2(legDir.x, -legDir.y);

          float bendRadius = bendSnap->radius;
          float endRadius = endSnap->radius;

          const auto &leftShinParam =
              modBodies_[ToIndex(ModBodyPart::LeftShin)].GetParam();

          const float baseRadius = 0.1f;
          const float leftShinThicknessScale =
              (std::max)(bendRadius, endRadius) / baseRadius;

          mesh.scale.x = leftShinThicknessScale * leftShinParam.scale.x;
          mesh.scale.y =
              legLength * leftShinParam.scale.y * leftShinParam.length;
          mesh.scale.z = leftShinThicknessScale * leftShinParam.scale.z;

          mesh.translate = {0.0f, -legLength * 0.5f, 0.0f};

          Vector3 ownerTranslate = {0.0f, 0.0f, 0.0f};

          for (const auto &ownerInstance : customizeData_->partInstances) {
            if (ownerInstance.partId == ownerPartId) {
              ownerTranslate = ownerInstance.localTransform.translate;
              break;
            }
          }

          const ModControlPointSnapshot *rootSnap = nullptr;
          for (const auto *snap : snaps) {
            if (snap->role == ModControlPointRole::Root) {
              rootSnap = snap;
              break;
            }
          }

          if (rootSnap != nullptr) {
            Vector3 kneeOffset =
                Sub(bendSnap->localPosition, rootSnap->localPosition);

            obj->mainPosition.transform.translate = {
                ownerTranslate.x + kneeOffset.x,
                ownerTranslate.y + kneeOffset.y,
                ownerTranslate.z + kneeOffset.z};
          } else {
            obj->mainPosition.transform.translate = ownerTranslate;
          }

          obj->mainPosition.transform.rotate = {0.0f, 0.0f, legAngleZ};
        }
      } else if (instance.partType == ModBodyPart::RightThigh) {

        std::vector<const ModControlPointSnapshot *> snaps;
        snaps.reserve(customizeData_->controlPointSnapshots.size());

        for (const auto &snap : customizeData_->controlPointSnapshots) {
          if (snap.ownerPartId == instance.partId) {
            snaps.push_back(&snap);
          }
        }

        const ModControlPointSnapshot *bendSnap = nullptr;
        const ModControlPointSnapshot *endSnap = nullptr;

        for (const auto *snap : snaps) {
          if (snap->role == ModControlPointRole::Bend) {
            bendSnap = snap;
          }
          if (snap->role == ModControlPointRole::End) {
            endSnap = snap;
          }
        }

        if (bendSnap == nullptr || endSnap == nullptr) {

          mesh.scale = baseScale;
          mesh.translate = {0.0f, -baseScale.y * 0.5f, 0.0f};

        } else {
          const ModControlPointSnapshot *rootSnap = nullptr;

          for (const auto *snap : snaps) {
            if (snap->role == ModControlPointRole::Root) {
              rootSnap = snap;
            }
            if (snap->role == ModControlPointRole::Bend) {
              bendSnap = snap;
            }
          }

          Vector3 legVector = {0.0f, -1.0f, 0.0f};
          float legLength = 0.0001f;
          float legAngleZ = 0.0f;

          if (rootSnap != nullptr && bendSnap != nullptr) {
            legVector = Sub(bendSnap->localPosition, rootSnap->localPosition);
            legLength = Length(legVector);

            if (legLength < 0.0001f) {
              legLength = 0.0001f;
              legVector = {0.0f, -1.0f, 0.0f};
            }

            Vector3 legDir = Normalize(legVector);
            legAngleZ = atan2(legDir.x, -legDir.y);

            obj->mainPosition.transform.translate =
                instance.localTransform.translate;
          } else {
            mesh.scale = baseScale;
            mesh.translate = {0.0f, -baseScale.y * 0.5f, 0.0f};
          }

          float startRadius = 0.1f;
          float bendRadius = 0.1f;

          for (const auto *snap : snaps) {
            if (snap->role == ModControlPointRole::Root) {
              startRadius = snap->radius;
            }
            if (snap->role == ModControlPointRole::Bend) {
              bendRadius = snap->radius;
            }
          }

          const auto &rightThighParam =
              modBodies_[ToIndex(ModBodyPart::RightThigh)].GetParam();

          const float baseRadius = 0.1f;
          const float rightThighThicknessScale =
              (std::max)(startRadius, bendRadius) / baseRadius;

          mesh.scale.x = rightThighThicknessScale * rightThighParam.scale.x;
          mesh.scale.y =
              legLength * rightThighParam.scale.y * rightThighParam.length;
          mesh.scale.z = rightThighThicknessScale * rightThighParam.scale.z;

          mesh.translate = {0.0f, -legLength * 0.5f, 0.0f};

          obj->mainPosition.transform.rotate = {0.0f, 0.0f, legAngleZ};
        }
      } else if (instance.partType == ModBodyPart::RightShin) {

        int ownerPartId = instance.partId;

        // 右すねの control owner は親の右腿
        if (instance.parentId >= 0) {
          ownerPartId = instance.parentId;
        }

std::vector<const ModControlPointSnapshot *> snaps;
        CollectSnapshotsByOwnerId(ownerPartId, snaps);

        const ModControlPointSnapshot *bendSnap = nullptr;
        const ModControlPointSnapshot *endSnap = nullptr;

        for (const auto *snap : snaps) {
          if (snap->role == ModControlPointRole::Bend) {
            bendSnap = snap;
          }
          if (snap->role == ModControlPointRole::End) {
            endSnap = snap;
          }
        }

        if (bendSnap == nullptr || endSnap == nullptr) {

          mesh.scale = baseScale;
          mesh.translate = {0.0f, -baseScale.y * 0.5f, 0.0f};

        } else {
          Vector3 legVector =
              Sub(endSnap->localPosition, bendSnap->localPosition);
          float legLength = Length(legVector);

          if (legLength < 0.0001f) {
            legLength = 0.0001f;
            legVector = {0.0f, -1.0f, 0.0f};
          }

          Vector3 legDir = Normalize(legVector);
          float legAngleZ = atan2(legDir.x, -legDir.y);

          float bendRadius = bendSnap->radius;
          float endRadius = endSnap->radius;

          const auto &rightShinParam =
              modBodies_[ToIndex(ModBodyPart::RightShin)].GetParam();

          const float baseRadius = 0.1f;
          const float rightShinThicknessScale =
              (std::max)(bendRadius, endRadius) / baseRadius;

          mesh.scale.x = rightShinThicknessScale * rightShinParam.scale.x;
          mesh.scale.y =
              legLength * rightShinParam.scale.y * rightShinParam.length;
          mesh.scale.z = rightShinThicknessScale * rightShinParam.scale.z;

          mesh.translate = {0.0f, -legLength * 0.5f, 0.0f};

          Vector3 ownerTranslate = {0.0f, 0.0f, 0.0f};

          for (const auto &ownerInstance : customizeData_->partInstances) {
            if (ownerInstance.partId == ownerPartId) {
              ownerTranslate = ownerInstance.localTransform.translate;
              break;
            }
          }

          const ModControlPointSnapshot *rootSnap = nullptr;
          for (const auto *snap : snaps) {
            if (snap->role == ModControlPointRole::Root) {
              rootSnap = snap;
              break;
            }
          }

          if (rootSnap != nullptr) {
            Vector3 kneeOffset =
                Sub(bendSnap->localPosition, rootSnap->localPosition);

            obj->mainPosition.transform.translate = {
                ownerTranslate.x + kneeOffset.x,
                ownerTranslate.y + kneeOffset.y,
                ownerTranslate.z + kneeOffset.z};
          } else {
            obj->mainPosition.transform.translate = ownerTranslate;
          }

          obj->mainPosition.transform.rotate = {0.0f, 0.0f, legAngleZ};
        }
      }
      if (instance.partType == ModBodyPart::Neck) {

        int ownerPartId = -1;

        for (const auto &childInstance : customizeData_->partInstances) {
          if (childInstance.parentId == instance.partId &&
              childInstance.partType == ModBodyPart::Head) {
            ownerPartId = childInstance.partId;
            break;
          }
        }

        std::vector<const ModControlPointSnapshot *> snaps;
        snaps.reserve(customizeData_->controlPointSnapshots.size());

        for (const auto &snap : customizeData_->controlPointSnapshots) {
          if (snap.ownerPartId == ownerPartId) {
            snaps.push_back(&snap);
          }
        }

        const ModControlPointSnapshot *lowerSnap = nullptr;
        const ModControlPointSnapshot *upperSnap = nullptr;

        for (const auto *snap : snaps) {
          if (snap->role == ModControlPointRole::Root) {
            lowerSnap = snap;
          }
          if (snap->role == ModControlPointRole::Bend) {
            upperSnap = snap;
          }
        }

        if (lowerSnap == nullptr || upperSnap == nullptr) {
          mesh.scale = baseScale;
          mesh.translate = {0.0f, -baseScale.y * 0.5f, 0.0f};
        } else {
          Vector3 neckVector =
              Sub(upperSnap->localPosition, lowerSnap->localPosition);
          float neckLength = Length(neckVector);

          if (neckLength < 0.0001f) {
            neckLength = 0.0001f;
            neckVector = {0.0f, -1.0f, 0.0f};
          }

          Vector3 neckDir = Normalize(neckVector);
          float neckAngleZ = atan2(neckDir.x, -neckDir.y);

          float lowerRadius = lowerSnap->radius;
          float upperRadius = upperSnap->radius;

          const auto &neckParam =
              modBodies_[ToIndex(ModBodyPart::Neck)].GetParam();

          const float baseRadius = 0.1f;
          const float neckThicknessScale =
              (std::max)(lowerRadius, upperRadius) / baseRadius;

          mesh.scale.x = neckThicknessScale * neckParam.scale.x;
          mesh.scale.y = neckLength * neckParam.scale.y * neckParam.length;
          mesh.scale.z = neckThicknessScale * neckParam.scale.z;

          mesh.translate = {0.0f, -neckLength * 0.5f, 0.0f};

          obj->mainPosition.transform.translate =
              instance.localTransform.translate;
          obj->mainPosition.transform.rotate = {0.0f, 0.0f, neckAngleZ};
        }
      }
    }

    builtPartIdToPart[instance.partId] = &obj->mainPosition;

    extraObjects_.push_back(obj);
    extraPartTypes_.push_back(instance.partType);
    extraPartIds_.push_back(instance.partId);
    extraParentIds_.push_back(instance.parentId);
  }
}

void TravelScene::UpdateExtraVisualParts() {
  Object *fixedHead = modObjects_[ToIndex(ModBodyPart::Head)];
  Object *fixedNeck = modObjects_[ToIndex(ModBodyPart::Neck)];
  Object *fixedLeftUpperArm = modObjects_[ToIndex(ModBodyPart::LeftUpperArm)];
  Object *fixedLeftForeArm = modObjects_[ToIndex(ModBodyPart::LeftForeArm)];
  Object *fixedRightUpperArm = modObjects_[ToIndex(ModBodyPart::RightUpperArm)];
  Object *fixedRightForeArm = modObjects_[ToIndex(ModBodyPart::RightForeArm)];
  Object *fixedLeftThigh = modObjects_[ToIndex(ModBodyPart::LeftThigh)];
  Object *fixedLeftShin = modObjects_[ToIndex(ModBodyPart::LeftShin)];
  Object *fixedRightThigh = modObjects_[ToIndex(ModBodyPart::RightThigh)];
  Object *fixedRightShin = modObjects_[ToIndex(ModBodyPart::RightShin)];

  std::unordered_map<int, Object *> extraPartObjectMap;
  for (size_t i = 0; i < extraObjects_.size() && i < extraPartIds_.size();
       ++i) {
    if (extraObjects_[i] != nullptr) {
      extraPartObjectMap[extraPartIds_[i]] = extraObjects_[i];
    }
  }

  const size_t count = (std::min)(extraObjects_.size(), extraPartTypes_.size());

  for (size_t i = 0; i < count; ++i) {
    Object *obj = extraObjects_[i];
    if (obj == nullptr) {
      continue;
    }

    ModBodyPart partType = extraPartTypes_[i];

    int partId = -1;
    if (i < extraPartIds_.size()) {
      partId = extraPartIds_[i];
    }

    int parentId = -1;
    if (i < extraParentIds_.size()) {
      parentId = extraParentIds_[i];
    }

    Object *source = nullptr;
    switch (partType) {
    case ModBodyPart::Head:
      source = fixedHead;
      break;
    case ModBodyPart::Neck:
      source = fixedNeck;
      break;
    case ModBodyPart::LeftUpperArm:
      source = fixedLeftUpperArm;
      break;
    case ModBodyPart::LeftForeArm:
      source = fixedLeftForeArm;
      break;
    case ModBodyPart::RightUpperArm:
      source = fixedRightUpperArm;
      break;
    case ModBodyPart::RightForeArm:
      source = fixedRightForeArm;
      break;
    case ModBodyPart::LeftThigh:
      source = fixedLeftThigh;
      break;
    case ModBodyPart::LeftShin:
      source = fixedLeftShin;
      break;
    case ModBodyPart::RightThigh:
      source = fixedRightThigh;
      break;
    case ModBodyPart::RightShin:
      source = fixedRightShin;
      break;
    default:
      break;
    }

    if (source == nullptr) {
      obj->Update(usingCamera_);
      continue;
    }

    if (!obj->objectParts_.empty() && !source->objectParts_.empty()) {
      obj->objectParts_[0].transform.rotate =
          source->objectParts_[0].transform.rotate;
    }

    if (partType == ModBodyPart::Head || partType == ModBodyPart::Neck) {
      obj->mainPosition.transform.translate =
          source->mainPosition.transform.translate;
      obj->mainPosition.transform.rotate =
          source->mainPosition.transform.rotate;
      obj->Update(usingCamera_);
      continue;
    }

    int snapshotOwnerId = GetExtraSnapshotOwnerId(partType, partId, parentId);

    float baseAngleX = 0.0f;
    float baseAngleZ = 0.0f;
    bool hasBaseAngles = ComputeExtraBaseAngles(partType, snapshotOwnerId,
                                                baseAngleX, baseAngleZ);

    if (!hasBaseAngles) {
      obj->Update(usingCamera_);
      continue;
    }

    if (partType == ModBodyPart::LeftForeArm ||
        partType == ModBodyPart::RightForeArm ||
        partType == ModBodyPart::LeftShin ||
        partType == ModBodyPart::RightShin) {
      Object *parentObj = nullptr;
      bool hasParent = GetExtraPartParentObject(partType, parentId,
                                                extraPartObjectMap, parentObj);

      if (hasParent && parentObj != nullptr &&
          !parentObj->objectParts_.empty()) {
        const Vector3 parentRoot = parentObj->mainPosition.transform.translate;
        const float parentAngleX = parentObj->mainPosition.transform.rotate.x;
        const float parentAngleZ = parentObj->mainPosition.transform.rotate.z;
        const float parentLength =
            (std::max)(0.05f, parentObj->objectParts_[0].transform.scale.y);

        obj->mainPosition.transform.translate =
            BuildAnimatedChildRootFromParent(parentRoot, parentAngleZ,
                                             parentAngleX, parentLength);
      }
    }

    obj->mainPosition.transform.rotate = {
        baseAngleX + ComputeExtraAnimAngleX(partType), 0.0f, baseAngleZ};

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

  bool leftNowInput = system_->GetIsPush(DIK_A);
  bool rightNowInput = system_->GetIsPush(DIK_D);

  // bool hasMoveInput = leftNowInput || rightNowInput;
  // float poseAnimScale = hasMoveInput ? 1.0f : 0.0f;
  float poseAnimScale = 1.0f;

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

  if (neck == nullptr || head == nullptr || leftUpperArm == nullptr ||
      leftForeArm == nullptr || rightUpperArm == nullptr ||
      rightForeArm == nullptr || leftThigh == nullptr || leftShin == nullptr ||
      rightThigh == nullptr || rightShin == nullptr) {
    return;
  }

  auto Sub = [](const Vector3 &a, const Vector3 &b) -> Vector3 {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
  };

  auto BuildAnimatedChildRoot = [](const Vector3 &root, float angleZ,
                                   float angleX, float length) -> Vector3 {
    return {root.x + std::sin(angleZ) * std::cos(angleX) * length,
            root.y - std::cos(angleZ) * std::cos(angleX) * length,
            root.z - std::sin(angleX) * length};
  };

  int baseHeadPartId = -1;

  if (customizeData_ != nullptr) {
    for (const auto &snap : customizeData_->controlPointSnapshots) {
      if (snap.role == ModControlPointRole::Root ||
          snap.role == ModControlPointRole::Bend ||
          snap.role == ModControlPointRole::End) {

        baseHeadPartId = snap.ownerPartId;
        break;
      }
    }
  }

  Vector3 baseNeckLocalTranslate = {0.0f, 0.0f, 0.0f};
  Vector3 baseHeadLocalTranslate = {0.0f, 0.0f, 0.0f};

  Vector3 baseNeckLocalScale = {1.0f, 1.0f, 1.0f};
  Vector3 baseHeadLocalScale = {1.0f, 1.0f, 1.0f};

  if (customizeData_ != nullptr) {
    bool foundBaseNeck = false;
    bool foundBaseHead = false;

    for (const auto &instance : customizeData_->partInstances) {
      if (!foundBaseNeck && instance.partType == ModBodyPart::Neck) {
        baseNeckLocalTranslate = instance.localTransform.translate;
        baseNeckLocalScale = instance.localTransform.scale;
        foundBaseNeck = true;
      }

      if (!foundBaseHead && instance.partType == ModBodyPart::Head) {
        baseHeadLocalTranslate = instance.localTransform.translate;
        baseHeadLocalScale = instance.localTransform.scale;
        foundBaseHead = true;
      }

      if (foundBaseNeck && foundBaseHead) {
        break;
      }
    }
  }

  Vector3 baseLowerNeckPos = {0.0f, 0.0f, 0.0f};
  Vector3 baseUpperNeckPos = {0.0f, 0.0f, 0.0f};
  Vector3 baseHeadCenterPos = {0.0f, 0.0f, 0.0f};

  bool hasBaseLowerNeck = false;
  bool hasBaseUpperNeck = false;
  bool hasBaseHeadCenter = false;

  float lowerNeckR = 0.1f;
  float upperNeckR = 0.1f;
  float headRadius = 0.1f;

  if (customizeData_ != nullptr && baseHeadPartId >= 0) {
    for (size_t i = 0; i < customizeData_->controlPointSnapshots.size(); ++i) {
      const auto &snap = customizeData_->controlPointSnapshots[i];
      if (snap.ownerPartId != baseHeadPartId) {
        continue;
      }

      if (snap.role == ModControlPointRole::Root) {
        baseLowerNeckPos = snap.localPosition;
        lowerNeckR = snap.radius;
        hasBaseLowerNeck = true;
      } else if (snap.role == ModControlPointRole::Bend) {
        baseUpperNeckPos = snap.localPosition;
        upperNeckR = snap.radius;
        hasBaseUpperNeck = true;
      } else if (snap.role == ModControlPointRole::End) {
        baseHeadCenterPos = snap.localPosition;
        headRadius = snap.radius;
        hasBaseHeadCenter = true;
      }
    }
  }

  Object *chestBody = modObjects_[ToIndex(ModBodyPart::ChestBody)];
  Object *stomachBody = modObjects_[ToIndex(ModBodyPart::StomachBody)];

  if (chestBody == nullptr || stomachBody == nullptr) {
    return;
  }

  static bool torsoBaseLogOnce = false;
  if (!torsoBaseLogOnce) {
    if (!chestBody->objectParts_.empty()) {
      const Vector3 &chestBaseTranslate =
          chestBody->objectParts_[0].transform.translate;
    } else {
    }

    if (!stomachBody->objectParts_.empty()) {
      const Vector3 &stomachBaseTranslate =
          stomachBody->objectParts_[0].transform.translate;
    } else {
    }

    torsoBaseLogOnce = true;
  }

  auto Mid = [](const Vector3 &a, const Vector3 &b) -> Vector3 {
    return {(a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f, (a.z + b.z) * 0.5f};
  };

  // const Vector3 chestCenterWorld = Mid(cp->chestPos, cp->bellyPos);
  // const Vector3 stomachCenterWorld = Mid(cp->bellyPos, cp->waistPos);

  // const float chestLength = Length(Sub(cp->chestPos, cp->bellyPos));
  // const float stomachLength = Length(Sub(cp->bellyPos, cp->waistPos));

  // const float leftUpperArmLength =
  //     Length(Sub(cp->leftElbowPos, cp->leftShoulderPos));
  // const float leftForeArmLength =
  //     Length(Sub(cp->leftWristPos, cp->leftElbowPos));

  // const float leftThighLength = Length(Sub(cp->leftKneePos, cp->leftHipPos));
  // const float rightThighLength = Length(Sub(cp->rightKneePos,
  // cp->rightHipPos));

  const Vector3 chestTopWorld = cp->chestPos;
  const Vector3 bellyWorld = cp->bellyPos;
  const Vector3 waistWorld = cp->waistPos;

  const Vector3 chestCenterWorld = Mid(chestTopWorld, bellyWorld);
  const Vector3 stomachCenterWorld = Mid(bellyWorld, waistWorld);

  //  const float chestLength = Length(Sub(bellyWorld, chestTopWorld));
  // const float stomachLength = Length(Sub(waistWorld, bellyWorld));

  const float leftUpperArmLength =
      Length(Sub(cp->leftElbowPos, cp->leftShoulderPos));
  const float leftForeArmLength =
      Length(Sub(cp->leftWristPos, cp->leftElbowPos));

  const float leftThighLength = Length(Sub(cp->leftKneePos, cp->leftHipPos));
  const float rightThighLength = Length(Sub(cp->rightKneePos, cp->rightHipPos));

  //==============================
  // 1段目 : body の子
  //==============================

  // const Vector3 bodyNeckConnector = {0.0f, 0.0f, 0.0f};
  // neck->mainPosition.transform.translate = bodyNeckConnector;

  // Vector3 neckVec = Sub(cp->upperNeckPos, cp->lowerNeckPos);
  // float neckLength = Length(neckVec);

  // Vector3 neckDir = {0.0f, 1.0f, 0.0f};
  // if (neckLength > 0.0001f) {
  //   neckDir = Normalize(neckVec);
  // }

  // float neckAngleZ = atan2(neckDir.x, -neckDir.y);
  // neck->mainPosition.transform.rotate = {0.0f, 0.0f, neckAngleZ};

  //==============================
  // 首：head snapshotベース
  // root = LowerNeck
  // mesh = LowerNeck -> UpperNeck
  //==============================
  // const Vector3 bodyNeckConnector = {0.0f, 0.0f, 0.0f};
  // neck->mainPosition.transform.translate = bodyNeckConnector;

  // Vector3 neckVec = {0.0f, 1.0f, 0.0f};
  // float neckLength = 0.0001f;
  // float neckAngleZ = 0.0f;

  // if (hasBaseLowerNeck && hasBaseUpperNeck) {
  //   neckVec = Sub(baseUpperNeckPos, baseLowerNeckPos);
  //   neckLength = Length(neckVec);

  //  if (neckLength < 0.0001f) {
  //    neckLength = 0.0001f;
  //    neckVec = {0.0f, 1.0f, 0.0f};
  //  }

  //  Vector3 neckDir = Normalize(neckVec);
  //  neckAngleZ = atan2(neckDir.x, -neckDir.y);
  //} else {
  //  neckVec = Sub(cp->upperNeckPos, cp->lowerNeckPos);
  //  neckLength = Length(neckVec);

  //  if (neckLength < 0.0001f) {
  //    neckLength = 0.0001f;
  //    neckVec = {0.0f, 1.0f, 0.0f};
  //  }

  //  Vector3 neckDir = Normalize(neckVec);
  //  neckAngleZ = atan2(neckDir.x, -neckDir.y);
  //}

  // neck->mainPosition.transform.rotate = {0.0f, 0.0f, neckAngleZ};

  const float armSwingScale = 0.60f;

  // Vector3 chestOffset = Sub(cp->chestPos, chestCenterWorld);
  Vector3 chestOffset = {0.0f, 0.0f, 0.0f};

  // 左上腕
  // leftUpperArm->mainPosition.transform.translate =
  //    Sub(cp->leftShoulderPos, cp->chestPos) + chestOffset;

  // Vector3 leftUpperArmDir = Sub(cp->leftElbowPos, cp->leftShoulderPos);
  // leftUpperArmDir = Normalize(leftUpperArmDir);

  // float leftUpperArmAngleZ = atan2(leftUpperArmDir.x, -leftUpperArmDir.y);

  // leftUpperArm->mainPosition.transform.rotate = {-rightLegBend_ *
  // armSwingScale,
  //                                                0.0f, leftUpperArmAngleZ};

  // 左腿
  // leftThigh->mainPosition.transform.translate =
  //    Sub(cp->leftHipPos, cp->bellyPos);

  // Vector3 leftThighDir = Sub(cp->leftKneePos, cp->leftHipPos);
  // leftThighDir = Normalize(leftThighDir);

  // float leftThighAngleZ = atan2(leftThighDir.x, -leftThighDir.y);

  // leftThigh->mainPosition.transform.rotate = {-leftLegBend_ * 0.7f, 0.0f,
  //                                             leftThighAngleZ};

  // 右腿
  // rightThigh->mainPosition.transform.translate =
  //    Sub(cp->rightHipPos, cp->bellyPos);

  // Vector3 rightThighDir = Sub(cp->rightKneePos, cp->rightHipPos);
  // rightThighDir = Normalize(rightThighDir);

  // float rightThighAngleZ = atan2(rightThighDir.x, -rightThighDir.y);

  // rightThigh->mainPosition.transform.rotate = {-rightLegBend_ * 0.7f, 0.0f,
  //                                              rightThighAngleZ};

  //==============================
  // 2段目 : 親の子
  //==============================

  // Vector3 headVector = {0.0f, 1.0f, 0.0f};
  // float headLength = 0.0001f;
  // float headAngleZ = 0.0f;

  // if (hasBaseUpperNeck && hasBaseHeadCenter) {
  //   headVector = Sub(baseHeadCenterPos, baseUpperNeckPos);
  //   headLength = Length(headVector);

  //  if (headLength < 0.0001f) {
  //    headLength = 0.0001f;
  //    headVector = {0.0f, 1.0f, 0.0f};
  //  }

  //  Vector3 headDir = Normalize(headVector);
  //  headAngleZ = atan2(headDir.x, -headDir.y);

  //  head->mainPosition.transform.translate =
  //      Sub(baseUpperNeckPos, cp->chestPos) + chestOffset;
  //} else {
  //  headVector = Sub(cp->headCenterPos, cp->upperNeckPos);
  //  headLength = Length(headVector);

  //  if (headLength < 0.0001f) {
  //    headLength = 0.0001f;
  //    headVector = {0.0f, 1.0f, 0.0f};
  //  }

  //  Vector3 headDir = Normalize(headVector);
  //  headAngleZ = atan2(headDir.x, -headDir.y);

  //  head->mainPosition.transform.translate =
  //      Sub(cp->upperNeckPos, cp->chestPos) + chestOffset;
  //}

  // head->mainPosition.transform.rotate = {0.0f, 0.0f, headAngleZ};

  //================================
  // 左前腕：上腕アニメ後の肘位置
  //================================
  // float animatedLeftUpperArmAngleX = -rightLegBend_ * armSwingScale;

  // Vector3 leftAnimatedElbowPos =
  // leftUpperArm->mainPosition.transform.translate; leftAnimatedElbowPos.x +=
  // std::sin(leftUpperArmAngleZ) *
  //                           std::cos(animatedLeftUpperArmAngleX) *
  //                           leftUpperArmLength;
  // leftAnimatedElbowPos.y += -std::cos(leftUpperArmAngleZ) *
  //                           std::cos(animatedLeftUpperArmAngleX) *
  //                           leftUpperArmLength;
  // leftAnimatedElbowPos.z +=
  //     -std::sin(animatedLeftUpperArmAngleX) * leftUpperArmLength;

  // Vector3 leftForeArmDir = Sub(cp->leftWristPos, cp->leftElbowPos);
  // leftForeArmDir = Normalize(leftForeArmDir);

  // float leftForeArmAngleZ = atan2(leftForeArmDir.x, -leftForeArmDir.y);

  // float leftUpperArmSwing = -rightLegBend_ * armSwingScale;
  // float leftElbowFold = std::clamp((rightLegBend_ - legKickAngle_) /
  //                                      (legRecoverAngle_ - legKickAngle_),
  //                                  0.0f, 1.0f);

  // float leftForeArmX =
  //     -(leftUpperArmSwing * 0.35f + leftElbowFold * 0.45f + 0.20f);

  // leftForeArm->mainPosition.transform.translate = leftAnimatedElbowPos;
  // leftForeArm->mainPosition.transform.rotate = {leftForeArmX, 0.0f,
  //                                               leftForeArmAngleZ};

  //================================
  // 脛：腿アニメ後の膝位置
  //================================
  // float animatedLeftThighAngleX = -leftLegBend_ * 0.7f;
  // float animatedRightThighAngleX = -rightLegBend_ * 0.7f;

  // Vector3 leftAnimatedKneePos = leftThigh->mainPosition.transform.translate;
  // leftAnimatedKneePos.x += std::sin(leftThighAngleZ) *
  //                          std::cos(animatedLeftThighAngleX) *
  //                          leftThighLength;
  // leftAnimatedKneePos.y += -std::cos(leftThighAngleZ) *
  //                          std::cos(animatedLeftThighAngleX) *
  //                          leftThighLength;
  // leftAnimatedKneePos.z += -std::sin(animatedLeftThighAngleX) *
  // leftThighLength;

  // Vector3 rightAnimatedKneePos =
  // rightThigh->mainPosition.transform.translate; rightAnimatedKneePos.x +=
  // std::sin(rightThighAngleZ) *
  //                           std::cos(animatedRightThighAngleX) *
  //                           rightThighLength;
  // rightAnimatedKneePos.y += -std::cos(rightThighAngleZ) *
  //                           std::cos(animatedRightThighAngleX) *
  //                           rightThighLength;
  // rightAnimatedKneePos.z +=
  //     -std::sin(animatedRightThighAngleX) * rightThighLength;

  // Vector3 leftShinDir = Sub(cp->leftAnklePos, cp->leftKneePos);
  // leftShinDir = Normalize(leftShinDir);

  // float leftShinAngleZ = atan2(leftShinDir.x, -leftShinDir.y);

  // float leftThighSwing = -leftLegBend_ * 0.7f;
  // float leftKneeFold = std::clamp((leftLegBend_ - legKickAngle_) /
  //                                     (legRecoverAngle_ - legKickAngle_),
  //                                 0.0f, 1.0f);

  // float leftShinX = leftThighSwing * 0.35f + leftKneeFold * 0.6f + 0.3f;

  // leftShin->mainPosition.transform.translate = leftAnimatedKneePos;
  // leftShin->mainPosition.transform.rotate = {leftShinX, 0.0f,
  // leftShinAngleZ};

  // Vector3 rightShinDir = Sub(cp->rightAnklePos, cp->rightKneePos);
  // rightShinDir = Normalize(rightShinDir);

  // float rightShinAngleZ = atan2(rightShinDir.x, -rightShinDir.y);

  // float rightThighSwing = -rightLegBend_ * 0.7f;
  // float rightKneeFold = std::clamp((rightLegBend_ - legKickAngle_) /
  //                                      (legRecoverAngle_ - legKickAngle_),
  //                                  0.0f, 1.0f);

  // float rightShinX = rightThighSwing * 0.35f + rightKneeFold * 0.6f + 0.3f;

  // rightShin->mainPosition.transform.translate = rightAnimatedKneePos;
  // rightShin->mainPosition.transform.rotate = {rightShinX, 0.0f,
  //                                             rightShinAngleZ};

  if (leftUpperArm->objectParts_.empty() || leftForeArm->objectParts_.empty() ||
      rightUpperArm->objectParts_.empty() ||
      rightForeArm->objectParts_.empty() || leftThigh->objectParts_.empty() ||
      leftShin->objectParts_.empty() || rightThigh->objectParts_.empty() ||
      rightShin->objectParts_.empty()) {
    return;
  }

  //================================
  // 太さパラメータ取得
  //================================
  const auto &chestParam =
      modBodies_[ToIndex(ModBodyPart::ChestBody)].GetParam();
  const auto &stomachParam =
      modBodies_[ToIndex(ModBodyPart::StomachBody)].GetParam();
  const auto &headParam = modBodies_[ToIndex(ModBodyPart::Head)].GetParam();
  const auto &leftUpperArmParam =
      modBodies_[ToIndex(ModBodyPart::LeftUpperArm)].GetParam();
  const auto &leftForeArmParam =
      modBodies_[ToIndex(ModBodyPart::LeftForeArm)].GetParam();
  const auto &rightUpperArmParam =
      modBodies_[ToIndex(ModBodyPart::RightUpperArm)].GetParam();
  const auto &rightForeArmParam =
      modBodies_[ToIndex(ModBodyPart::RightForeArm)].GetParam();

  const auto &leftThighParam =
      modBodies_[ToIndex(ModBodyPart::LeftThigh)].GetParam();
  const auto &leftShinParam =
      modBodies_[ToIndex(ModBodyPart::LeftShin)].GetParam();
  const auto &rightThighParam =
      modBodies_[ToIndex(ModBodyPart::RightThigh)].GetParam();
  const auto &rightShinParam =
      modBodies_[ToIndex(ModBodyPart::RightShin)].GetParam();

  const float leftUpperArmStartR =
      GetSnapshotRadius(ModBodyPart::LeftUpperArm, 1);
  const float leftUpperArmBendR =
      GetSnapshotRadius(ModBodyPart::LeftUpperArm, 2);
  const float leftUpperArmEndR =
      GetSnapshotRadius(ModBodyPart::LeftUpperArm, 3);

  const float rightUpperArmStartR =
      GetSnapshotRadius(ModBodyPart::RightUpperArm, 1);
  const float rightUpperArmBendR =
      GetSnapshotRadius(ModBodyPart::RightUpperArm, 2);
  const float rightUpperArmEndR =
      GetSnapshotRadius(ModBodyPart::RightUpperArm, 3);

  const float leftThighStartR = GetSnapshotRadius(ModBodyPart::LeftThigh, 1);
  const float leftThighBendR = GetSnapshotRadius(ModBodyPart::LeftThigh, 2);
  const float leftThighEndR = GetSnapshotRadius(ModBodyPart::LeftThigh, 3);

  const float rightThighStartR = GetSnapshotRadius(ModBodyPart::RightThigh, 1);
  const float rightThighBendR = GetSnapshotRadius(ModBodyPart::RightThigh, 2);
  const float rightThighEndR = GetSnapshotRadius(ModBodyPart::RightThigh, 3);

  const float chestR = GetControlPointRadius(ModControlPointRole::Chest);
  const float bellyR = GetControlPointRadius(ModControlPointRole::Belly);
  const float stomachR = GetControlPointRadius(ModControlPointRole::Waist);

  const float baseRadius = 0.1f;

  const float leftUpperArmThicknessScale =
      (std::max)(leftUpperArmStartR, leftUpperArmBendR) / baseRadius;
  const float leftForeArmThicknessScale =
      (std::max)(leftUpperArmBendR, leftUpperArmEndR) / baseRadius;

  const float rightUpperArmThicknessScale =
      (std::max)(rightUpperArmStartR, rightUpperArmBendR) / baseRadius;
  const float rightForeArmThicknessScale =
      (std::max)(rightUpperArmBendR, rightUpperArmEndR) / baseRadius;

  const float leftThighThicknessScale =
      (std::max)(leftThighStartR, leftThighBendR) / baseRadius;
  const float leftShinThicknessScale =
      (std::max)(leftThighBendR, leftThighEndR) / baseRadius;

  const float rightThighThicknessScale =
      (std::max)(rightThighStartR, rightThighBendR) / baseRadius;
  const float rightShinThicknessScale =
      (std::max)(rightThighBendR, rightThighEndR) / baseRadius;

  const float neckThicknessScale =
      (std::max)(lowerNeckR, upperNeckR) / baseRadius;
  const float headThicknessScale =
      (std::max)(headRadius, upperNeckR) / baseRadius;
  const float chestThicknessScale = (std::max)(chestR, bellyR) / baseRadius;
  const float stomachThicknessScale = (std::max)(stomachR, bellyR) / baseRadius;

  const float leftShinLength = Length(Sub(cp->leftAnklePos, cp->leftKneePos));
  const float rightShinLength =
      Length(Sub(cp->rightAnklePos, cp->rightKneePos));

  const auto &neckParam = modBodies_[ToIndex(ModBodyPart::Neck)].GetParam();

  ////============================
  //// 胸部
  //// root = Chest
  //// mesh = Chest -> Belly
  ////============================
  // chestBody->mainPosition.transform.translate = {
  //     chestTopWorld.x, moveY_ + chestTopWorld.y, moveX_ + chestTopWorld.z};

  // chestBody->mainPosition.transform.rotate = {-bodyTilt_, 0.0f, 0.0f};

  // if (!chestBody->objectParts_.empty()) {
  //   chestBody->objectParts_[0].transform.translate = {0.0f, -chestLength *
  //   0.5f,
  //                                                     0.0f};

  //  chestBody->objectParts_[0].transform.scale.x =
  //      chestThicknessScale * chestParam.scale.x;
  //  chestBody->objectParts_[0].transform.scale.y =
  //      chestLength * chestParam.scale.y * chestParam.length;
  //  chestBody->objectParts_[0].transform.scale.z =
  //      chestThicknessScale * chestParam.scale.z;

  //  chestBody->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  //}

  //============================
  // 胸部
  // root = Chest
  // mesh = Chest -> Belly
  //============================
  // Vector3 chestVec = Sub(bellyWorld, chestTopWorld);
  // float chestLength = Length(chestVec);

  // if (chestLength < 0.0001f) {
  //   chestLength = 0.0001f;
  //   chestVec = {0.0f, -1.0f, 0.0f};
  // }

  // Vector3 chestDir = Normalize(chestVec);
  // float chestAngleZ = atan2f(chestDir.x, -chestDir.y);
  // float chestAngleX = -asinf(std::clamp(chestDir.z, -1.0f, 1.0f));

  // chestBody->mainPosition.transform.translate = {
  //     chestTopWorld.x, moveY_ + chestTopWorld.y, moveX_ + chestTopWorld.z};

  // chestBody->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  // if (!chestBody->objectParts_.empty()) {
  //   chestBody->objectParts_[0].transform.translate = {0.0f, -chestLength *
  //   0.5f,
  //                                                     0.0f};

  //  chestBody->objectParts_[0].transform.scale.x =
  //      chestThicknessScale * chestParam.scale.x;
  //  chestBody->objectParts_[0].transform.scale.y =
  //      chestLength * chestParam.scale.y * chestParam.length;
  //  chestBody->objectParts_[0].transform.scale.z =
  //      chestThicknessScale * chestParam.scale.z;

  //  chestBody->objectParts_[0].transform.rotate = {chestAngleX - bodyTilt_,
  //                                                 0.0f, chestAngleZ};
  //}

  ////============================
  //// 腹部
  //// root = Belly
  //// mesh = Belly -> Waist
  ////============================
  // stomachBody->mainPosition.transform.translate =
  //     Sub(bellyWorld, chestTopWorld);
  // stomachBody->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  // if (!stomachBody->objectParts_.empty()) {
  //   stomachBody->objectParts_[0].transform.translate = {
  //       0.0f, -stomachLength * 0.5f, 0.0f};

  //  stomachBody->objectParts_[0].transform.scale.x =
  //      stomachThicknessScale * stomachParam.scale.x;
  //  stomachBody->objectParts_[0].transform.scale.y =
  //      stomachLength * stomachParam.scale.y * stomachParam.length;
  //  stomachBody->objectParts_[0].transform.scale.z =
  //      stomachThicknessScale * stomachParam.scale.z;

  //  stomachBody->objectParts_[0].transform.rotate = {0.0f, 0.0f, 0.0f};
  //}

  //============================
  // 腹部
  // root = Belly
  // mesh = Belly -> Waist
  //============================
  // Vector3 stomachVec = Sub(waistWorld, bellyWorld);
  // float stomachLength = Length(stomachVec);

  // if (stomachLength < 0.0001f) {
  //   stomachLength = 0.0001f;
  //   stomachVec = {0.0f, -1.0f, 0.0f};
  // }

  // Vector3 stomachDir = Normalize(stomachVec);
  // float stomachAngleZ = atan2f(stomachDir.x, -stomachDir.y);
  // float stomachAngleX = -asinf(std::clamp(stomachDir.z, -1.0f, 1.0f));

  // stomachBody->mainPosition.transform.translate =
  //     Sub(bellyWorld, chestTopWorld);

  // stomachBody->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  // if (!stomachBody->objectParts_.empty()) {
  //   stomachBody->objectParts_[0].transform.translate = {
  //       0.0f, -stomachLength * 0.5f, 0.0f};

  //  stomachBody->objectParts_[0].transform.scale.x =
  //      stomachThicknessScale * stomachParam.scale.x;
  //  stomachBody->objectParts_[0].transform.scale.y =
  //      stomachLength * stomachParam.scale.y * stomachParam.length;
  //  stomachBody->objectParts_[0].transform.scale.z =
  //      stomachThicknessScale * stomachParam.scale.z;

  //  stomachBody->objectParts_[0].transform.rotate = {stomachAngleX, 0.0f,
  //                                                   stomachAngleZ};
  //}

  ////==============================
  //// 胸・腹も snapshot ベースで置く
  ////==============================
  // int chestOwnerId = -1;
  // int stomachOwnerId = -1;

  // if (customizeData_ != nullptr) {
  //   for (const auto &instance : customizeData_->partInstances) {
  //     if (chestOwnerId < 0 && instance.partType == ModBodyPart::ChestBody) {
  //       chestOwnerId = instance.partId;
  //     } else if (stomachOwnerId < 0 &&
  //                instance.partType == ModBodyPart::StomachBody) {
  //       stomachOwnerId = instance.partId;
  //     }
  //   }
  // }

  // SegmentVisual chestSeg{};
  // SegmentVisual stomachSeg{};

  // bool hasChestSeg = false;
  // bool hasStomachSeg = false;

  // if (chestOwnerId >= 0) {
  //   hasChestSeg = BuildSegmentFromSnapshot(ModBodyPart::ChestBody,
  //   chestOwnerId,
  //                                          chestSeg);
  // }

  // if (stomachOwnerId >= 0) {
  //   hasStomachSeg = BuildSegmentFromSnapshot(ModBodyPart::StomachBody,
  //                                            stomachOwnerId, stomachSeg);
  // }

  // static bool chestSegCheckOnce = false;
  // if (!chestSegCheckOnce) {
  //   Logger::Log("CHEST_SEG_CHECK hasChestSeg=%d hasStomachSeg=%d",
  //               hasChestSeg ? 1 : 0, hasStomachSeg ? 1 : 0);

  //  Logger::Log("CHEST_SEG_ROOT : (%.3f, %.3f, %.3f)", chestSeg.root.x,
  //              chestSeg.root.y, chestSeg.root.z);
  //  Logger::Log("CHEST_TOP_CP   : (%.3f, %.3f, %.3f)", chestTopWorld.x,
  //              chestTopWorld.y, chestTopWorld.z);
  //  Logger::Log("BELLY_CP       : (%.3f, %.3f, %.3f)", bellyWorld.x,
  //              bellyWorld.y, bellyWorld.z);

  //  chestSegCheckOnce = true;
  //}

  //// chest snapshot が無い場合だけ従来CPにフォールバック
  // if (!hasChestSeg) {
  //   Vector3 chestVec = Sub(bellyWorld, chestTopWorld);
  //   chestSeg.length = Length(chestVec);

  //  if (chestSeg.length < 0.0001f) {
  //    chestSeg.length = 0.0001f;
  //    chestVec = {0.0f, -1.0f, 0.0f};
  //  }

  //  Vector3 chestDir = Normalize(chestVec);
  //  chestSeg.angleZ = atan2f(chestDir.x, -chestDir.y);
  //  chestSeg.angleX = -asinf(std::clamp(chestDir.z, -1.0f, 1.0f));
  //  chestSeg.root = chestTopWorld;
  //}

  //// stomach snapshot が無い場合だけ従来CPにフォールバック
  // if (!hasStomachSeg) {
  //   Vector3 stomachVec = Sub(waistWorld, bellyWorld);
  //   stomachSeg.length = Length(stomachVec);

  //  if (stomachSeg.length < 0.0001f) {
  //    stomachSeg.length = 0.0001f;
  //    stomachVec = {0.0f, -1.0f, 0.0f};
  //  }

  //  Vector3 stomachDir = Normalize(stomachVec);
  //  stomachSeg.angleZ = atan2f(stomachDir.x, -stomachDir.y);
  //  stomachSeg.angleX = -asinf(std::clamp(stomachDir.z, -1.0f, 1.0f));
  //  stomachSeg.root = bellyWorld;
  //}

  if (!useModBodyApplyTorso_) {

    //==============================
    // torso は ChestBody owner からまとめて読む
    // chest  = Chest -> Belly
    // stomach = Belly -> Waist
    //==============================
    int torsoOwnerId = -1;

    if (customizeData_ != nullptr) {
      for (const auto &instance : customizeData_->partInstances) {
        if (instance.partType == ModBodyPart::ChestBody) {
          torsoOwnerId = instance.partId;
          break;
        }
      }
    }

    SegmentVisual chestSeg{};
    SegmentVisual stomachSeg{};

    bool hasChestSeg = false;
    bool hasStomachSeg = false;

    // ChestBody owner に入っている torso snapshot を直接読む
    if (customizeData_ != nullptr && torsoOwnerId >= 0) {
      const ModControlPointSnapshot *chestSnap = nullptr;
      const ModControlPointSnapshot *bellySnap = nullptr;
      const ModControlPointSnapshot *waistSnap = nullptr;

      for (const auto &snap : customizeData_->controlPointSnapshots) {
        if (snap.ownerPartId != torsoOwnerId) {
          continue;
        }

        if (snap.role == ModControlPointRole::Chest) {
          chestSnap = &snap;
        } else if (snap.role == ModControlPointRole::Belly) {
          bellySnap = &snap;
        } else if (snap.role == ModControlPointRole::Waist) {
          waistSnap = &snap;
        }
      }

      if (chestSnap != nullptr && bellySnap != nullptr) {
        Vector3 chestVec =
            Sub(bellySnap->localPosition, chestSnap->localPosition);
        chestSeg.length = Length(chestVec);

        if (chestSeg.length < 0.0001f) {
          chestSeg.length = 0.0001f;
          chestVec = {0.0f, -1.0f, 0.0f};
        }

        Vector3 chestDir = Normalize(chestVec);
        chestSeg.angleZ = atan2f(chestDir.x, -chestDir.y);
        chestSeg.angleX = -asinf(std::clamp(chestDir.z, -1.0f, 1.0f));
        chestSeg.root = chestSnap->localPosition;
        hasChestSeg = true;
      }

      if (bellySnap != nullptr && waistSnap != nullptr) {
        Vector3 stomachVec =
            Sub(waistSnap->localPosition, bellySnap->localPosition);
        stomachSeg.length = Length(stomachVec);

        if (stomachSeg.length < 0.0001f) {
          stomachSeg.length = 0.0001f;
          stomachVec = {0.0f, -1.0f, 0.0f};
        }

        Vector3 stomachDir = Normalize(stomachVec);
        stomachSeg.angleZ = atan2f(stomachDir.x, -stomachDir.y);
        stomachSeg.angleX = -asinf(std::clamp(stomachDir.z, -1.0f, 1.0f));
        stomachSeg.root = bellySnap->localPosition;
        hasStomachSeg = true;
      }
    }

    // 取れなかったときだけ cp にフォールバック
    if (!hasChestSeg) {
      Vector3 chestVec = Sub(bellyWorld, chestTopWorld);
      chestSeg.length = Length(chestVec);

      if (chestSeg.length < 0.0001f) {
        chestSeg.length = 0.0001f;
        chestVec = {0.0f, -1.0f, 0.0f};
      }

      Vector3 chestDir = Normalize(chestVec);
      chestSeg.angleZ = atan2f(chestDir.x, -chestDir.y);
      chestSeg.angleX = -asinf(std::clamp(chestDir.z, -1.0f, 1.0f));
      chestSeg.root = chestTopWorld;
    }

    if (!hasStomachSeg) {
      Vector3 stomachVec = Sub(waistWorld, bellyWorld);
      stomachSeg.length = Length(stomachVec);

      if (stomachSeg.length < 0.0001f) {
        stomachSeg.length = 0.0001f;
        stomachVec = {0.0f, -1.0f, 0.0f};
      }

      Vector3 stomachDir = Normalize(stomachVec);
      stomachSeg.angleZ = atan2f(stomachDir.x, -stomachDir.y);
      stomachSeg.angleX = -asinf(std::clamp(stomachDir.z, -1.0f, 1.0f));
      stomachSeg.root = bellyWorld;
    }

    static bool torsoReadLogOnce = false;
    if (!torsoReadLogOnce) {

      torsoReadLogOnce = true;
    }

    //============================
    // 胸部
    //============================
    // chestBody->mainPosition.transform.translate = {
    //    chestSeg.root.x, moveY_ + chestSeg.root.y, moveX_ + chestSeg.root.z};
    chestBody->mainPosition.transform.translate = {
        moveX_ + chestSeg.root.x, moveY_ + chestSeg.root.y, chestSeg.root.z};

    chestBody->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

    if (!chestBody->objectParts_.empty()) {
      chestBody->objectParts_[0].transform.translate = {
          0.0f, -chestSeg.length * 0.5f, 0.0f};

      chestBody->objectParts_[0].transform.scale.x =
          chestThicknessScale * chestParam.scale.x;
      chestBody->objectParts_[0].transform.scale.y =
          chestSeg.length * chestParam.scale.y * chestParam.length;
      chestBody->objectParts_[0].transform.scale.z =
          chestThicknessScale * chestParam.scale.z;

      chestBody->objectParts_[0].transform.rotate = {
          chestSeg.angleX - bodyTilt_, 0.0f, chestSeg.angleZ};
    }

    //============================
    // 腹部
    //============================
    stomachBody->mainPosition.transform.translate = {
        stomachSeg.root.x - chestSeg.root.x,
        stomachSeg.root.y - chestSeg.root.y,
        stomachSeg.root.z - chestSeg.root.z};

    stomachBody->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

    if (!stomachBody->objectParts_.empty()) {
      stomachBody->objectParts_[0].transform.translate = {
          0.0f, -stomachSeg.length * 0.5f, 0.0f};

      stomachBody->objectParts_[0].transform.scale.x =
          stomachThicknessScale * stomachParam.scale.x;
      stomachBody->objectParts_[0].transform.scale.y =
          stomachSeg.length * stomachParam.scale.y * stomachParam.length;
      stomachBody->objectParts_[0].transform.scale.z =
          stomachThicknessScale * stomachParam.scale.z;

      stomachBody->objectParts_[0].transform.rotate = {stomachSeg.angleX, 0.0f,
                                                       stomachSeg.angleZ};
    }

    static bool torsoPlacementLogOnce = false;
    if (!torsoPlacementLogOnce) {
      torsoPlacementLogOnce = true;
    }
  }

  //================================================
  // 腕脚の snapshot / anchor 取得
  //================================================

  Vector3 neckBaseAnchorLocal = {0.0f, 0.45f, 0.0f};

  Vector3 neckRootLocal = {0.0f, 0.0f, 0.0f};
  Vector3 neckBendLocal = {0.0f, 0.280f, 0.0f};
  Vector3 neckEndLocal = {0.0f, 0.800f, 0.0f};

  bool hasNeckRoot = false;
  bool hasNeckBend = false;
  bool hasNeckEnd = false;

  //------------------------------
  // 左腕
  //------------------------------
  int leftUpperArmOwnerId = -1;
  Vector3 leftShoulderAnchorLocal = {-1.25f, 1.0f, 0.0f};

  Vector3 leftArmRootLocal = {0.0f, 0.0f, 0.0f};
  Vector3 leftArmBendLocal = {0.0f, -0.55f, 0.0f};
  Vector3 leftArmEndLocal = {0.0f, -1.10f, 0.0f};

  bool hasLeftArmRoot = false;
  bool hasLeftArmBend = false;
  bool hasLeftArmEnd = false;

  //------------------------------
  // 右腕
  //------------------------------
  int rightUpperArmOwnerId = -1;
  Vector3 rightShoulderAnchorLocal = {1.25f, 1.0f, 0.0f};

  Vector3 rightArmRootLocal = {0.0f, 0.0f, 0.0f};
  Vector3 rightArmBendLocal = {0.0f, -0.55f, 0.0f};
  Vector3 rightArmEndLocal = {0.0f, -1.10f, 0.0f};

  bool hasRightArmRoot = false;
  bool hasRightArmBend = false;
  bool hasRightArmEnd = false;

  //------------------------------
  // 左脚
  //------------------------------
  int leftThighOwnerId = -1;
  Vector3 leftHipAnchorLocal = {-0.5f, -1.25f, 0.0f};

  Vector3 leftLegRootLocal = {0.0f, 0.0f, 0.0f};
  Vector3 leftLegBendLocal = {0.0f, -0.70f, 0.0f};
  Vector3 leftLegEndLocal = {0.0f, -1.40f, 0.0f};

  bool hasLeftLegRoot = false;
  bool hasLeftLegBend = false;
  bool hasLeftLegEnd = false;

  //------------------------------
  // 右脚
  //------------------------------
  int rightThighOwnerId = -1;
  Vector3 rightHipAnchorLocal = {0.5f, -1.25f, 0.0f};

  Vector3 rightLegRootLocal = {0.0f, 0.0f, 0.0f};
  Vector3 rightLegBendLocal = {0.0f, -0.70f, 0.0f};
  Vector3 rightLegEndLocal = {0.0f, -1.40f, 0.0f};

  bool hasRightLegRoot = false;
  bool hasRightLegBend = false;
  bool hasRightLegEnd = false;

  if (customizeData_ != nullptr) {
    //==============================
    // 各部位 ownerPartId は従来どおり partInstances から取る
    // ただし肩・股関節 anchor は torso owner の snapshot から取る
    //==============================
    for (const auto &instance : customizeData_->partInstances) {
      if (instance.partType == ModBodyPart::LeftUpperArm &&
          leftUpperArmOwnerId < 0) {
        leftUpperArmOwnerId = instance.partId;
      } else if (instance.partType == ModBodyPart::RightUpperArm &&
                 rightUpperArmOwnerId < 0) {
        rightUpperArmOwnerId = instance.partId;
      } else if (instance.partType == ModBodyPart::LeftThigh &&
                 leftThighOwnerId < 0) {
        leftThighOwnerId = instance.partId;
      } else if (instance.partType == ModBodyPart::RightThigh &&
                 rightThighOwnerId < 0) {
        rightThighOwnerId = instance.partId;
      }
    }

    //==============================
    // torso owner から shoulder / hip anchor を読む
    //==============================
    int torsoAnchorOwnerId = -1;
    for (const auto &instance : customizeData_->partInstances) {
      if (instance.partType == ModBodyPart::ChestBody) {
        torsoAnchorOwnerId = instance.partId;
        break;
      }
    }

    if (torsoAnchorOwnerId >= 0) {
      for (const auto &snap : customizeData_->controlPointSnapshots) {
        if (snap.ownerPartId != torsoAnchorOwnerId) {
          continue;
        }

        if (snap.role == ModControlPointRole::NeckBase) {
          neckBaseAnchorLocal = snap.localPosition;
        } else if (snap.role == ModControlPointRole::LeftShoulder) {
          leftShoulderAnchorLocal = snap.localPosition;
        } else if (snap.role == ModControlPointRole::RightShoulder) {
          rightShoulderAnchorLocal = snap.localPosition;
        } else if (snap.role == ModControlPointRole::LeftHip) {
          leftHipAnchorLocal = snap.localPosition;
        } else if (snap.role == ModControlPointRole::RightHip) {
          rightHipAnchorLocal = snap.localPosition;
        }
      }
    }

    static bool anchorCompareLogOnce = false;
    if (!anchorCompareLogOnce) {

      anchorCompareLogOnce = true;
    }

    //==============================
    // 首 snapshot
    //==============================
    int neckOwnerId = -1;
    for (const auto &instance : customizeData_->partInstances) {
      if (instance.partType == ModBodyPart::Neck) {
        neckOwnerId = instance.partId;
        break;
      }
    }

    if (neckOwnerId >= 0) {
      for (const auto &snap : customizeData_->controlPointSnapshots) {
        if (snap.ownerPartId != neckOwnerId) {
          continue;
        }

        if (snap.role == ModControlPointRole::Root) {
          neckRootLocal = snap.localPosition;
          hasNeckRoot = true;
        } else if (snap.role == ModControlPointRole::Bend) {
          neckBendLocal = snap.localPosition;
          hasNeckBend = true;
        } else if (snap.role == ModControlPointRole::End) {
          neckEndLocal = snap.localPosition;
          hasNeckEnd = true;
        }
      }
    }

    //==============================
    // 左腕 snapshot
    //==============================
    if (leftUpperArmOwnerId >= 0) {
      for (const auto &snap : customizeData_->controlPointSnapshots) {
        if (snap.ownerPartId != leftUpperArmOwnerId) {
          continue;
        }

        if (snap.role == ModControlPointRole::Root) {
          leftArmRootLocal = snap.localPosition;
          hasLeftArmRoot = true;
        } else if (snap.role == ModControlPointRole::Bend) {
          leftArmBendLocal = snap.localPosition;
          hasLeftArmBend = true;
        } else if (snap.role == ModControlPointRole::End) {
          leftArmEndLocal = snap.localPosition;
          hasLeftArmEnd = true;
        }
      }
    }

    //==============================
    // 右腕 snapshot
    //==============================
    if (rightUpperArmOwnerId >= 0) {
      for (const auto &snap : customizeData_->controlPointSnapshots) {
        if (snap.ownerPartId != rightUpperArmOwnerId) {
          continue;
        }

        if (snap.role == ModControlPointRole::Root) {
          rightArmRootLocal = snap.localPosition;
          hasRightArmRoot = true;
        } else if (snap.role == ModControlPointRole::Bend) {
          rightArmBendLocal = snap.localPosition;
          hasRightArmBend = true;
        } else if (snap.role == ModControlPointRole::End) {
          rightArmEndLocal = snap.localPosition;
          hasRightArmEnd = true;
        }
      }
    }

    //==============================
    // 左脚 snapshot
    //==============================
    if (leftThighOwnerId >= 0) {
      for (const auto &snap : customizeData_->controlPointSnapshots) {
        if (snap.ownerPartId != leftThighOwnerId) {
          continue;
        }

        if (snap.role == ModControlPointRole::Root) {
          leftLegRootLocal = snap.localPosition;
          hasLeftLegRoot = true;
        } else if (snap.role == ModControlPointRole::Bend) {
          leftLegBendLocal = snap.localPosition;
          hasLeftLegBend = true;
        } else if (snap.role == ModControlPointRole::End) {
          leftLegEndLocal = snap.localPosition;
          hasLeftLegEnd = true;
        }
      }
    }

    //==============================
    // 右脚 snapshot
    //==============================
    if (rightThighOwnerId >= 0) {
      for (const auto &snap : customizeData_->controlPointSnapshots) {
        if (snap.ownerPartId != rightThighOwnerId) {
          continue;
        }

        if (snap.role == ModControlPointRole::Root) {
          rightLegRootLocal = snap.localPosition;
          hasRightLegRoot = true;
        } else if (snap.role == ModControlPointRole::Bend) {
          rightLegBendLocal = snap.localPosition;
          hasRightLegBend = true;
        } else if (snap.role == ModControlPointRole::End) {
          rightLegEndLocal = snap.localPosition;
          hasRightLegEnd = true;
        }
      }
    }
  }

  Vector3 leftShoulderFromChest = {leftShoulderAnchorLocal.x - chestTopWorld.x,
                                   leftShoulderAnchorLocal.y - chestTopWorld.y,
                                   leftShoulderAnchorLocal.z - chestTopWorld.z};

  Vector3 rightShoulderFromChest = {
      rightShoulderAnchorLocal.x - chestTopWorld.x,
      rightShoulderAnchorLocal.y - chestTopWorld.y,
      rightShoulderAnchorLocal.z - chestTopWorld.z};

  Vector3 leftHipFromBelly = {leftHipAnchorLocal.x - bellyWorld.x,
                              leftHipAnchorLocal.y - bellyWorld.y,
                              leftHipAnchorLocal.z - bellyWorld.z};

  Vector3 rightHipFromBelly = {rightHipAnchorLocal.x - bellyWorld.x,
                               rightHipAnchorLocal.y - bellyWorld.y,
                               rightHipAnchorLocal.z - bellyWorld.z};

  //==============================
  // 首
  // anchor = NeckBase
  // shape  = Neck Root -> Neck Bend
  //==============================
  if (neck != nullptr && hasNeckRoot && hasNeckBend) {
    Vector3 neckShapeVec = Sub(neckBendLocal, neckRootLocal);
    float neckLength = Length(neckShapeVec);

    if (neckLength < 0.0001f) {
      neckLength = 0.0001f;
      neckShapeVec = {0.0f, 1.0f, 0.0f};
    }

    Vector3 neckDir = Normalize(neckShapeVec);
    float neckAngleZ = atan2f(-neckDir.x, neckDir.y);
    float neckAngleX = asinf(std::clamp(neckDir.z, -1.0f, 1.0f));

    const float neckRootR = GetSnapshotRadius(ModBodyPart::Neck, 1);
    const float neckBendR = GetSnapshotRadius(ModBodyPart::Neck, 2);
    const float neckThicknessScale = (std::max)(neckRootR, neckBendR) / 0.1f;

    neck->mainPosition.transform.translate = neckBaseAnchorLocal;
    neck->mainPosition.transform.rotate = {neckAngleX, 0.0f, neckAngleZ};

    if (!neck->objectParts_.empty()) {
      neck->objectParts_[0].transform.translate = {0.0f, neckLength * 0.5f,
                                                   0.0f};
      neck->objectParts_[0].transform.scale.x =
          neckThicknessScale * neckParam.scale.x;
      neck->objectParts_[0].transform.scale.y =
          neckLength * neckParam.scale.y * neckParam.length;
      neck->objectParts_[0].transform.scale.z =
          neckThicknessScale * neckParam.scale.z;
    }

    static bool neckAnchorCheckOnce = false;
    if (!neckAnchorCheckOnce) {
      neckAnchorCheckOnce = true;
    }
  }

  //==============================
  // 頭
  // anchor = 首の先
  // shape  = Neck Bend -> Neck End
  //==============================
  if (head != nullptr && hasNeckRoot && hasNeckBend && hasNeckEnd) {
    Vector3 neckTipOffset = Sub(neckBendLocal, neckRootLocal);
    Vector3 headAnchorLocal = Add(neckBaseAnchorLocal, neckTipOffset);

    Vector3 headShapeVec = Sub(neckEndLocal, neckBendLocal);
    float headLength = Length(headShapeVec);

    if (headLength < 0.0001f) {
      headLength = 0.0001f;
      headShapeVec = {0.0f, 1.0f, 0.0f};
    }

    Vector3 headDir = Normalize(headShapeVec);
    float headAngleZ = atan2f(-headDir.x, headDir.y);
    float headAngleX = asinf(std::clamp(headDir.z, -1.0f, 1.0f));

    const float headThicknessScale =
        (std::max)(GetSnapshotRadius(ModBodyPart::Neck, 2),
                   GetSnapshotRadius(ModBodyPart::Neck, 3)) /
        0.1f;

    head->mainPosition.transform.translate = headAnchorLocal;
    head->mainPosition.transform.rotate = {headAngleX, 0.0f, headAngleZ};

    if (!head->objectParts_.empty()) {
      head->objectParts_[0].transform.translate = {0.0f, headLength * 0.5f,
                                                   0.0f};
      head->objectParts_[0].transform.scale.x =
          headThicknessScale * headParam.scale.x;
      head->objectParts_[0].transform.scale.y =
          headLength * headParam.scale.y * headParam.length;
      head->objectParts_[0].transform.scale.z =
          headThicknessScale * headParam.scale.z;
    }

    static bool headAnchorCheckOnce = false;
    if (!headAnchorCheckOnce) {
      headAnchorCheckOnce = true;
    }
  }

  //==============================
  // 左上腕＋左前腕（snapshot棒立ち）
  // root = shoulder anchor
  // elbow = shoulder anchor + (bend - root)
  //==============================
  if (leftUpperArm != nullptr && leftForeArm != nullptr && hasLeftArmRoot &&
      hasLeftArmBend && hasLeftArmEnd) {

    Vector3 upperDir = Sub(leftArmBendLocal, leftArmRootLocal);
    float upperLength = Length(upperDir);
    if (upperLength < 0.0001f) {
      upperDir = {0.0f, -1.0f, 0.0f};
      upperLength = 0.0001f;
    } else {
      upperDir = Normalize(upperDir);
    }

    float leftUpperArmAngleZ = atan2(upperDir.x, -upperDir.y);
    float leftUpperArmBaseX = -asinf(std::clamp(upperDir.z, -1.0f, 1.0f));

    float leftUpperArmAnimX = (-rightLegBend_ * armSwingScale) * poseAnimScale;

    float leftUpperArmAngleX = leftUpperArmBaseX + leftUpperArmAnimX;

    // leftUpperArm->mainPosition.transform.translate = leftShoulderAnchorLocal;
    leftUpperArm->mainPosition.transform.translate =
        Add(chestTopWorld, leftShoulderFromChest);
    leftUpperArm->mainPosition.transform.rotate = {leftUpperArmAngleX, 0.0f,
                                                   leftUpperArmAngleZ};

    if (!leftUpperArm->objectParts_.empty()) {
      leftUpperArm->objectParts_[0].transform.translate = {
          0.0f, -upperLength * 0.5f, 0.0f};
      leftUpperArm->objectParts_[0].transform.scale.x =
          leftUpperArmThicknessScale * leftUpperArmParam.scale.x;
      leftUpperArm->objectParts_[0].transform.scale.y =
          upperLength * leftUpperArmParam.scale.y * leftUpperArmParam.length;
      leftUpperArm->objectParts_[0].transform.scale.z =
          leftUpperArmThicknessScale * leftUpperArmParam.scale.z;
    }

    Vector3 foreDir = Sub(leftArmEndLocal, leftArmBendLocal);
    float foreLength = Length(foreDir);
    if (foreLength < 0.0001f) {
      foreDir = {0.0f, -1.0f, 0.0f};
      foreLength = 0.0001f;
    } else {
      foreDir = Normalize(foreDir);
    }

    float leftForeArmAngleZ = atan2(foreDir.x, -foreDir.y);
    float leftForeArmBaseX = -asinf(std::clamp(foreDir.z, -1.0f, 1.0f));

    float leftUpperArmSwing = -rightLegBend_ * armSwingScale;
    float leftElbowFold = std::clamp((rightLegBend_ - legKickAngle_) /
                                         (legRecoverAngle_ - legKickAngle_),
                                     0.0f, 1.0f);

    float leftForeArmAnimX =
        (-(leftUpperArmSwing * 0.35f + leftElbowFold * 0.45f)) * poseAnimScale;

    float leftForeArmAngleX = leftForeArmBaseX + leftForeArmAnimX;

    Vector3 leftUpperArmRoot = Add(chestTopWorld, leftShoulderFromChest);
    Vector3 leftForeArmRoot = BuildAnimatedChildRoot(
        leftUpperArmRoot, leftUpperArmAngleZ, leftUpperArmAngleX, upperLength);

    leftForeArm->mainPosition.transform.translate = leftForeArmRoot;
    leftForeArm->mainPosition.transform.rotate = {leftForeArmAngleX, 0.0f,
                                                  leftForeArmAngleZ};

    if (!leftForeArm->objectParts_.empty()) {
      leftForeArm->objectParts_[0].transform.translate = {
          0.0f, -foreLength * 0.5f, 0.0f};
      leftForeArm->objectParts_[0].transform.scale.x =
          leftForeArmThicknessScale * leftForeArmParam.scale.x;
      leftForeArm->objectParts_[0].transform.scale.y =
          foreLength * leftForeArmParam.scale.y * leftForeArmParam.length;
      leftForeArm->objectParts_[0].transform.scale.z =
          leftForeArmThicknessScale * leftForeArmParam.scale.z;
    }
  }

  static bool rightArmCheckOnce = false;
  if (!rightArmCheckOnce) {

    rightArmCheckOnce = true;
  }

  //==============================
  // 右上腕＋右前腕（snapshot棒立ち）
  // root = shoulder anchor
  // elbow = shoulder anchor + (bend - root)
  //==============================
  if (rightUpperArm != nullptr && rightForeArm != nullptr && hasRightArmRoot &&
      hasRightArmBend && hasRightArmEnd) {

    Vector3 upperDir = Sub(rightArmBendLocal, rightArmRootLocal);
    float upperLength = Length(upperDir);
    if (upperLength < 0.0001f) {
      upperDir = {0.0f, -1.0f, 0.0f};
      upperLength = 0.0001f;
    } else {
      upperDir = Normalize(upperDir);
    }

    float rightUpperArmAngleZ = atan2(upperDir.x, -upperDir.y);
    float rightUpperArmBaseX = -asinf(std::clamp(upperDir.z, -1.0f, 1.0f));

    float rightUpperArmAnimX = (-leftLegBend_ * armSwingScale) * poseAnimScale;

    float rightUpperArmAngleX = rightUpperArmBaseX + rightUpperArmAnimX;

    // rightUpperArm->mainPosition.transform.translate =
    // rightShoulderAnchorLocal;
    // rightUpperArm->mainPosition.transform.translate =
    //    Add(chestTopWorld, rightShoulderFromChest);
    // rightUpperArm->mainPosition.transform.translate =
    // rightShoulderAnchorLocal;
    rightUpperArm->mainPosition.transform.translate =
        Add(chestTopWorld, rightShoulderFromChest);
    rightUpperArm->mainPosition.transform.rotate = {rightUpperArmAngleX, 0.0f,
                                                    rightUpperArmAngleZ};

    if (!rightUpperArm->objectParts_.empty()) {
      rightUpperArm->objectParts_[0].transform.translate = {
          0.0f, -upperLength * 0.5f, 0.0f};
      rightUpperArm->objectParts_[0].transform.scale.x =
          rightUpperArmThicknessScale * rightUpperArmParam.scale.x;
      rightUpperArm->objectParts_[0].transform.scale.y =
          upperLength * rightUpperArmParam.scale.y * rightUpperArmParam.length;
      rightUpperArm->objectParts_[0].transform.scale.z =
          rightUpperArmThicknessScale * rightUpperArmParam.scale.z;
    }

    Vector3 foreDir = Sub(rightArmEndLocal, rightArmBendLocal);
    float foreLength = Length(foreDir);
    if (foreLength < 0.0001f) {
      foreDir = {0.0f, -1.0f, 0.0f};
      foreLength = 0.0001f;
    } else {
      foreDir = Normalize(foreDir);
    }

    float rightForeArmAngleZ = atan2(foreDir.x, -foreDir.y);
    float rightForeArmBaseX = -asinf(std::clamp(foreDir.z, -1.0f, 1.0f));

    float rightUpperArmSwing = -leftLegBend_ * armSwingScale;
    float rightElbowFold = std::clamp((leftLegBend_ - legKickAngle_) /
                                          (legRecoverAngle_ - legKickAngle_),
                                      0.0f, 1.0f);

    float rightForeArmAnimX =
        (-(rightUpperArmSwing * 0.35f + rightElbowFold * 0.45f)) *
        poseAnimScale;

    float rightForeArmAngleX = rightForeArmBaseX + rightForeArmAnimX;

    // Vector3 rightElbowOffset = Sub(rightArmBendLocal, rightArmRootLocal);
    // Vector3 rightUpperArmRoot = Add(chestTopWorld, rightShoulderFromChest);
    // Vector3 rightForeArmRoot = Add(rightUpperArmRoot, rightElbowOffset);
    Vector3 rightUpperArmRoot = Add(chestTopWorld, rightShoulderFromChest);
    Vector3 rightForeArmRoot =
        BuildAnimatedChildRoot(rightUpperArmRoot, rightUpperArmAngleZ,
                               rightUpperArmAngleX, upperLength);

    rightForeArm->mainPosition.transform.translate = rightForeArmRoot;
    rightForeArm->mainPosition.transform.rotate = {rightForeArmAngleX, 0.0f,
                                                   rightForeArmAngleZ};

    if (!rightForeArm->objectParts_.empty()) {
      rightForeArm->objectParts_[0].transform.translate = {
          0.0f, -foreLength * 0.5f, 0.0f};
      rightForeArm->objectParts_[0].transform.scale.x =
          rightForeArmThicknessScale * rightForeArmParam.scale.x;
      rightForeArm->objectParts_[0].transform.scale.y =
          foreLength * rightForeArmParam.scale.y * rightForeArmParam.length;
      rightForeArm->objectParts_[0].transform.scale.z =
          rightForeArmThicknessScale * rightForeArmParam.scale.z;
    }
  }

  //==============================
  // 左腿＋左脛（snapshot棒立ち）
  // root = hip anchor
  // knee = hip anchor + (bend - root)
  //==============================
  if (leftThigh != nullptr && leftShin != nullptr && hasLeftLegRoot &&
      hasLeftLegBend && hasLeftLegEnd) {

    Vector3 thighDir = Sub(leftLegBendLocal, leftLegRootLocal);
    float thighLength = Length(thighDir);
    if (thighLength < 0.0001f) {
      thighDir = {0.0f, -1.0f, 0.0f};
      thighLength = 0.0001f;
    } else {
      thighDir = Normalize(thighDir);
    }

    // float leftThighAngleZ = atan2(thighDir.x, -thighDir.y);
    // float leftThighAngleX = -asinf(std::clamp(thighDir.z, -1.0f, 1.0f));
    float leftThighAngleZ = atan2(thighDir.x, -thighDir.y);
    float leftThighBaseX = -asinf(std::clamp(thighDir.z, -1.0f, 1.0f));

    const float thighSwingScale = 0.70f;
    float leftThighAnimX = (-leftLegBend_ * thighSwingScale) * poseAnimScale;
    float leftThighAngleX = leftThighBaseX + leftThighAnimX;

    // 脚は stomachRoot の子なので、belly 基準の相対座標を使う
    leftThigh->mainPosition.transform.translate = leftHipFromBelly;
    leftThigh->mainPosition.transform.rotate = {leftThighAngleX, 0.0f,
                                                leftThighAngleZ};

    if (!leftThigh->objectParts_.empty()) {
      leftThigh->objectParts_[0].transform.translate = {
          0.0f, -thighLength * 0.5f, 0.0f};
      leftThigh->objectParts_[0].transform.scale.x =
          leftThighThicknessScale * leftThighParam.scale.x;
      leftThigh->objectParts_[0].transform.scale.y =
          thighLength * leftThighParam.scale.y * leftThighParam.length;
      leftThigh->objectParts_[0].transform.scale.z =
          leftThighThicknessScale * leftThighParam.scale.z;
    }

    Vector3 shinDir = Sub(leftLegEndLocal, leftLegBendLocal);
    float shinLength = Length(shinDir);
    if (shinLength < 0.0001f) {
      shinDir = {0.0f, -1.0f, 0.0f};
      shinLength = 0.0001f;
    } else {
      shinDir = Normalize(shinDir);
    }

    float leftShinAngleZ = atan2(shinDir.x, -shinDir.y);
    float leftShinBaseX = -asinf(std::clamp(shinDir.z, -1.0f, 1.0f));

    float leftThighSwing = -leftLegBend_ * thighSwingScale;
    float leftKneeFold = std::clamp((leftLegBend_ - legKickAngle_) /
                                        (legRecoverAngle_ - legKickAngle_),
                                    0.0f, 1.0f);

    float leftShinAnimX =
        (leftThighSwing * 0.35f + leftKneeFold * 0.6f) * poseAnimScale;

    float leftShinAngleX = leftShinBaseX + leftShinAnimX;

    // Vector3 leftKneeOffset = Sub(leftLegBendLocal, leftLegRootLocal);
    // Vector3 leftShinRoot = Add(leftHipFromBelly, leftKneeOffset);
    Vector3 leftThighRoot = leftHipFromBelly;
    Vector3 leftShinRoot = BuildAnimatedChildRoot(
        leftThighRoot, leftThighAngleZ, leftThighAngleX, thighLength);

    leftShin->mainPosition.transform.translate = leftShinRoot;
    leftShin->mainPosition.transform.rotate = {leftShinAngleX, 0.0f,
                                               leftShinAngleZ};

    if (!leftShin->objectParts_.empty()) {
      leftShin->objectParts_[0].transform.translate = {0.0f, -shinLength * 0.5f,
                                                       0.0f};
      leftShin->objectParts_[0].transform.scale.x =
          leftShinThicknessScale * leftShinParam.scale.x;
      leftShin->objectParts_[0].transform.scale.y =
          shinLength * leftShinParam.scale.y * leftShinParam.length;
      leftShin->objectParts_[0].transform.scale.z =
          leftShinThicknessScale * leftShinParam.scale.z;
    }
  }

  //==============================
  // 右腿＋右脛（snapshot棒立ち）
  // root = hip anchor
  // knee = hip anchor + (bend - root)
  //==============================
  if (rightThigh != nullptr && rightShin != nullptr && hasRightLegRoot &&
      hasRightLegBend && hasRightLegEnd) {

    Vector3 thighDir = Sub(rightLegBendLocal, rightLegRootLocal);
    float thighLength = Length(thighDir);
    if (thighLength < 0.0001f) {
      thighDir = {0.0f, -1.0f, 0.0f};
      thighLength = 0.0001f;
    } else {
      thighDir = Normalize(thighDir);
    }

    // float rightThighAngleZ = atan2(thighDir.x, -thighDir.y);
    // float rightThighAngleX = -asinf(std::clamp(thighDir.z, -1.0f, 1.0f));
    float rightThighAngleZ = atan2(thighDir.x, -thighDir.y);
    float rightThighBaseX = -asinf(std::clamp(thighDir.z, -1.0f, 1.0f));

    const float thighSwingScale = 0.70f;
    float rightThighAnimX = (-rightLegBend_ * thighSwingScale) * poseAnimScale;
    float rightThighAngleX = rightThighBaseX + rightThighAnimX;

    // 脚は stomachRoot の子なので、belly 基準の相対座標を使う
    rightThigh->mainPosition.transform.translate = rightHipFromBelly;
    rightThigh->mainPosition.transform.rotate = {rightThighAngleX, 0.0f,
                                                 rightThighAngleZ};

    if (!rightThigh->objectParts_.empty()) {
      rightThigh->objectParts_[0].transform.translate = {
          0.0f, -thighLength * 0.5f, 0.0f};
      rightThigh->objectParts_[0].transform.scale.x =
          rightThighThicknessScale * rightThighParam.scale.x;
      rightThigh->objectParts_[0].transform.scale.y =
          thighLength * rightThighParam.scale.y * rightThighParam.length;
      rightThigh->objectParts_[0].transform.scale.z =
          rightThighThicknessScale * rightThighParam.scale.z;
    }

    Vector3 shinDir = Sub(rightLegEndLocal, rightLegBendLocal);
    float shinLength = Length(shinDir);
    if (shinLength < 0.0001f) {
      shinDir = {0.0f, -1.0f, 0.0f};
      shinLength = 0.0001f;
    } else {
      shinDir = Normalize(shinDir);
    }

    float rightShinAngleZ = atan2(shinDir.x, -shinDir.y);
    float rightShinBaseX = -asinf(std::clamp(shinDir.z, -1.0f, 1.0f));

    float rightThighSwing = -rightLegBend_ * thighSwingScale;
    float rightKneeFold = std::clamp((rightLegBend_ - legKickAngle_) /
                                         (legRecoverAngle_ - legKickAngle_),
                                     0.0f, 1.0f);

    float rightShinAnimX =
        (rightThighSwing * 0.35f + rightKneeFold * 0.6f) * poseAnimScale;

    float rightShinAngleX = rightShinBaseX + rightShinAnimX;

    // Vector3 rightKneeOffset = Sub(rightLegBendLocal, rightLegRootLocal);
    // Vector3 rightShinRoot = Add(rightHipFromBelly, rightKneeOffset);
    Vector3 rightThighRoot = rightHipFromBelly;
    Vector3 rightShinRoot = BuildAnimatedChildRoot(
        rightThighRoot, rightThighAngleZ, rightThighAngleX, thighLength);

    rightShin->mainPosition.transform.translate = rightShinRoot;
    rightShin->mainPosition.transform.rotate = {rightShinAngleX, 0.0f,
                                                rightShinAngleZ};

    if (!rightShin->objectParts_.empty()) {
      rightShin->objectParts_[0].transform.translate = {
          0.0f, -shinLength * 0.5f, 0.0f};
      rightShin->objectParts_[0].transform.scale.x =
          rightShinThicknessScale * rightShinParam.scale.x;
      rightShin->objectParts_[0].transform.scale.y =
          shinLength * rightShinParam.scale.y * rightShinParam.length;
      rightShin->objectParts_[0].transform.scale.z =
          rightShinThicknessScale * rightShinParam.scale.z;
    }
  }

  // if (!neck->objectParts_.empty()) {
  //   neck->objectParts_[0].transform.translate = {0.0f, -neckLength * 0.5f,
  //                                                0.0f};
  //   neck->objectParts_[0].transform.scale.y =
  //       neckLength * neckParam.scale.y * neckParam.length;
  // }

  // if (!neck->objectParts_.empty()) {
  //   neck->objectParts_[0].transform.translate = {0.0f, -neckLength * 0.5f,
  //                                                0.0f};

  //  neck->objectParts_[0].transform.scale.x =
  //      neckThicknessScale * neckParam.scale.x;
  //  neck->objectParts_[0].transform.scale.y =
  //      neckLength * neckParam.scale.y * neckParam.length;
  //  neck->objectParts_[0].transform.scale.z =
  //      neckThicknessScale * neckParam.scale.z;
  //}

  // float headHeight = 0.0001f;

  // if (hasBaseUpperNeck && hasBaseHeadCenter) {
  //   headHeight = Length(Sub(baseHeadCenterPos, baseUpperNeckPos) * 2.0f);
  // } else {
  //   headHeight = Length(Sub(cp->headCenterPos, cp->upperNeckPos) * 2.0f);
  // }

  // if (headHeight < 0.0001f) {
  //   headHeight = 0.0001f;
  // }

  // if (!head->objectParts_.empty()) {
  //   head->objectParts_[0].transform.translate = {0.0f, -headHeight * 0.5f,
  //                                                0.0f};

  //  head->objectParts_[0].transform.scale.x =
  //      headThicknessScale * headParam.scale.x;
  //  head->objectParts_[0].transform.scale.y =
  //      headHeight * headParam.scale.y * headParam.length;
  //  head->objectParts_[0].transform.scale.z =
  //      headThicknessScale * headParam.scale.z;
  //}

  //==============================
  // 頭：snapshotベース
  // root = UpperNeck
  // mesh = UpperNeck -> HeadCenter
  //==============================
  // Vector3 headVector = {0.0f, 1.0f, 0.0f};
  // float headHeight = 0.0001f;
  // float headAngleZ = 0.0f;

  // if (hasBaseUpperNeck && hasBaseHeadCenter) {
  //   headVector = Sub(baseHeadCenterPos, baseUpperNeckPos);
  //   float headHalfLength = Length(headVector);

  //  if (headHalfLength < 0.0001f) {
  //    headHalfLength = 0.0001f;
  //    headVector = {0.0f, 1.0f, 0.0f};
  //  }

  //  Vector3 headDir = Normalize(headVector);
  //  headAngleZ = atan2(headDir.x, -headDir.y);

  //  headHeight = headHalfLength * 2.0f;

  //  head->mainPosition.transform.translate = baseUpperNeckPos;
  //  head->mainPosition.transform.rotate = {0.0f, 0.0f, headAngleZ};

  //  if (!head->objectParts_.empty()) {
  //    head->objectParts_[0].transform.translate = {0.0f, -headHeight * 0.5f,
  //                                                 0.0f};

  //    head->objectParts_[0].transform.scale.x =
  //        headThicknessScale * headParam.scale.x;
  //    head->objectParts_[0].transform.scale.y =
  //        headHeight * headParam.scale.y * headParam.length;
  //    head->objectParts_[0].transform.scale.z =
  //        headThicknessScale * headParam.scale.z;
  //  }
  //}

  //================================================
  // 左上腕
  //================================================
  // leftUpperArm->objectParts_[0].transform.translate = {
  //    0.0f, -leftUpperArmLength * 0.5f, 0.0f};
  // leftUpperArm->objectParts_[0].transform.scale.x =
  //    leftUpperArmThicknessScale * leftUpperArmParam.scale.x;
  // leftUpperArm->objectParts_[0].transform.scale.y =
  //    leftUpperArmLength * leftUpperArmParam.scale.y *
  //    leftUpperArmParam.length;
  // leftUpperArm->objectParts_[0].transform.scale.z =
  //    leftUpperArmThicknessScale * leftUpperArmParam.scale.z;

  //================================================
  // 左前腕
  //================================================
  // leftForeArm->objectParts_[0].transform.translate = {
  //    0.0f, -leftForeArmLength * 0.5f, 0.0f};
  // leftForeArm->objectParts_[0].transform.scale.x =
  //    leftForeArmThicknessScale * leftForeArmParam.scale.x;
  // leftForeArm->objectParts_[0].transform.scale.y =
  //    leftForeArmLength * leftForeArmParam.scale.y * leftForeArmParam.length;
  // leftForeArm->objectParts_[0].transform.scale.z =
  //    leftForeArmThicknessScale * leftForeArmParam.scale.z;

  ////================================================
  //// 左上腕＋左前腕（snapshotベース）
  ////================================================
  // int leftUpperArmOwnerId = -1;
  // Vector3 leftShoulderAnchorLocal = {-1.25f, 1.0f, 0.0f};

  // if (customizeData_ != nullptr) {
  //   for (const auto &instance : customizeData_->partInstances) {
  //     if (instance.partType == ModBodyPart::LeftUpperArm) {
  //       leftUpperArmOwnerId = instance.partId;
  //       leftShoulderAnchorLocal = instance.localTransform.translate;
  //       break;
  //     }
  //   }
  // }

  // Vector3 leftArmRootLocal = {0.0f, 0.0f, 0.0f};
  // Vector3 leftArmBendLocal = {0.0f, -0.55f, 0.0f};
  // Vector3 leftArmEndLocal = {0.0f, -1.10f, 0.0f};

  // bool hasLeftArmRoot = false;
  // bool hasLeftArmBend = false;
  // bool hasLeftArmEnd = false;

  // if (customizeData_ != nullptr && leftUpperArmOwnerId >= 0) {
  //   for (const auto &snap : customizeData_->controlPointSnapshots) {
  //     if (snap.ownerPartId != leftUpperArmOwnerId) {
  //       continue;
  //     }

  //    if (snap.role == ModControlPointRole::Root) {
  //      leftArmRootLocal = snap.localPosition;
  //      hasLeftArmRoot = true;
  //    } else if (snap.role == ModControlPointRole::Bend) {
  //      leftArmBendLocal = snap.localPosition;
  //      hasLeftArmBend = true;
  //    } else if (snap.role == ModControlPointRole::End) {
  //      leftArmEndLocal = snap.localPosition;
  //      hasLeftArmEnd = true;
  //    }
  //  }
  //}

  // if (hasLeftArmRoot && hasLeftArmBend) {
  //   Vector3 upperVec = Sub(leftArmBendLocal, leftArmRootLocal);
  //   float upperLength = Length(upperVec);
  //   if (upperLength < 0.0001f) {
  //     upperLength = 0.0001f;
  //     upperVec = {0.0f, -1.0f, 0.0f};
  //   }

  //  Vector3 upperDir = Normalize(upperVec);
  //  float leftUpperArmAngleZ = atan2(upperDir.x, -upperDir.y);

  //  // leftUpperArm->mainPosition.transform.translate =
  //  leftShoulderAnchorLocal; leftUpperArm->mainPosition.transform.translate =
  //  {
  //      -chestThicknessScale * chestParam.scale.x, chestOffset.y, 0.0f};
  //  float leftUpperArmAngleX = -asinf(std::clamp(upperDir.z, -1.0f, 1.0f));
  //  float leftUpperArmAnimX = (-rightLegBend_ * armSwingScale) *
  //  poseAnimScale;

  //  leftUpperArm->mainPosition.transform.rotate = {
  //      leftUpperArmAngleX + leftUpperArmAnimX, 0.0f, leftUpperArmAngleZ};

  //  leftUpperArm->objectParts_[0].transform.translate = {
  //      0.0f, -upperLength * 0.5f, 0.0f};
  //  leftUpperArm->objectParts_[0].transform.scale.x =
  //      leftUpperArmThicknessScale * leftUpperArmParam.scale.x;
  //  leftUpperArm->objectParts_[0].transform.scale.y =
  //      upperLength * leftUpperArmParam.scale.y * leftUpperArmParam.length;
  //  leftUpperArm->objectParts_[0].transform.scale.z =
  //      leftUpperArmThicknessScale * leftUpperArmParam.scale.z;

  //  if (hasLeftArmEnd) {
  //    Vector3 foreVec = Sub(leftArmEndLocal, leftArmBendLocal);
  //    float foreLength = Length(foreVec);
  //    if (foreLength < 0.0001f) {
  //      foreLength = 0.0001f;
  //      foreVec = {0.0f, -1.0f, 0.0f};
  //    }

  //    Vector3 foreDir = Normalize(foreVec);
  //    float leftForeArmAngleZ = atan2(foreDir.x, -foreDir.y);

  //    float leftUpperArmSwing = -rightLegBend_ * armSwingScale;
  //    float leftElbowFold = std::clamp((rightLegBend_ - legKickAngle_) /
  //                                         (legRecoverAngle_ - legKickAngle_),
  //                                     0.0f, 1.0f);

  //    float leftForeArmX =
  //        -(leftUpperArmSwing * 0.35f + leftElbowFold * 0.45f + 0.20f);

  //    // Vector3 leftAnimatedElbowPos = leftShoulderAnchorLocal;
  //    Vector3 leftAnimatedElbowPos =
  //        leftUpperArm->mainPosition.transform.translate;
  //    leftAnimatedElbowPos.x += std::sin(leftUpperArmAngleZ) *
  //                              std::cos(-rightLegBend_ * armSwingScale) *
  //                              upperLength;
  //    leftAnimatedElbowPos.y += -std::cos(leftUpperArmAngleZ) *
  //                              std::cos(-rightLegBend_ * armSwingScale) *
  //                              upperLength;
  //    leftAnimatedElbowPos.z +=
  //        -std::sin(-rightLegBend_ * armSwingScale) * upperLength;

  //    leftForeArm->mainPosition.transform.translate = leftAnimatedElbowPos;
  //    float leftForeArmAngleX = -asinf(std::clamp(foreDir.z, -1.0f, 1.0f));
  //    float leftForeArmAnimX =
  //        (-(leftUpperArmSwing * 0.35f + leftElbowFold * 0.45f)) *
  //        poseAnimScale;

  //    leftForeArm->mainPosition.transform.rotate = {
  //        leftForeArmAngleX + leftForeArmAnimX, 0.0f, leftForeArmAngleZ};

  //    leftForeArm->objectParts_[0].transform.translate = {
  //        0.0f, -foreLength * 0.5f, 0.0f};
  //    leftForeArm->objectParts_[0].transform.scale.x =
  //        leftForeArmThicknessScale * leftForeArmParam.scale.x;
  //    leftForeArm->objectParts_[0].transform.scale.y =
  //        foreLength * leftForeArmParam.scale.y * leftForeArmParam.length;
  //    leftForeArm->objectParts_[0].transform.scale.z =
  //        leftForeArmThicknessScale * leftForeArmParam.scale.z;
  //  }
  //}

  ////================================================
  //// 右上腕＋右前腕（snapshotベース）
  ////================================================
  // int rightUpperArmOwnerId = -1;
  // Vector3 rightShoulderAnchorLocal = {1.25f, 1.0f, 0.0f};

  // if (customizeData_ != nullptr) {
  //   for (const auto &instance : customizeData_->partInstances) {
  //     if (instance.partType == ModBodyPart::RightUpperArm) {
  //       rightUpperArmOwnerId = instance.partId;
  //       rightShoulderAnchorLocal = instance.localTransform.translate;
  //       break;
  //     }
  //   }
  // }

  // Vector3 rightArmRootLocal = {0.0f, 0.0f, 0.0f};
  // Vector3 rightArmBendLocal = {0.0f, -0.55f, 0.0f};
  // Vector3 rightArmEndLocal = {0.0f, -1.10f, 0.0f};

  // bool hasRightArmRoot = false;
  // bool hasRightArmBend = false;
  // bool hasRightArmEnd = false;

  // if (customizeData_ != nullptr && rightUpperArmOwnerId >= 0) {
  //   for (const auto &snap : customizeData_->controlPointSnapshots) {
  //     if (snap.ownerPartId != rightUpperArmOwnerId) {
  //       continue;
  //     }

  //    if (snap.role == ModControlPointRole::Root) {
  //      rightArmRootLocal = snap.localPosition;
  //      hasRightArmRoot = true;
  //    } else if (snap.role == ModControlPointRole::Bend) {
  //      rightArmBendLocal = snap.localPosition;
  //      hasRightArmBend = true;
  //    } else if (snap.role == ModControlPointRole::End) {
  //      rightArmEndLocal = snap.localPosition;
  //      hasRightArmEnd = true;
  //    }
  //  }
  //}

  // if (hasRightArmRoot && hasRightArmBend) {
  //   Vector3 upperVec = Sub(rightArmBendLocal, rightArmRootLocal);
  //   float upperLength = Length(upperVec);
  //   if (upperLength < 0.0001f) {
  //     upperLength = 0.0001f;
  //     upperVec = {0.0f, -1.0f, 0.0f};
  //   }

  //  Vector3 upperDir = Normalize(upperVec);
  //  float rightUpperArmAngleZ = atan2(upperDir.x, -upperDir.y);

  //  // rightUpperArm->mainPosition.transform.translate =
  //  // rightShoulderAnchorLocal;
  //  // rightUpperArm->mainPosition.transform.translate =
  //  //    Sub(rightShoulderAnchorLocal, cp->chestPos) + chestOffset;
  //  rightUpperArm->mainPosition.transform.translate = {
  //      chestThicknessScale * chestParam.scale.x, chestOffset.y, 0.0f};
  //  float rightUpperArmAngleX = -asinf(std::clamp(upperDir.z, -1.0f, 1.0f));
  //  float rightUpperArmAnimX = (-leftLegBend_ * armSwingScale) *
  //  poseAnimScale;

  //  rightUpperArm->mainPosition.transform.rotate = {
  //      rightUpperArmAngleX + rightUpperArmAnimX, 0.0f, rightUpperArmAngleZ};

  //  rightUpperArm->objectParts_[0].transform.translate = {
  //      0.0f, -upperLength * 0.5f, 0.0f};
  //  rightUpperArm->objectParts_[0].transform.scale.x =
  //      rightUpperArmThicknessScale * rightUpperArmParam.scale.x;
  //  rightUpperArm->objectParts_[0].transform.scale.y =
  //      upperLength * rightUpperArmParam.scale.y * rightUpperArmParam.length;
  //  rightUpperArm->objectParts_[0].transform.scale.z =
  //      rightUpperArmThicknessScale * rightUpperArmParam.scale.z;

  //  if (hasRightArmEnd) {
  //    Vector3 foreVec = Sub(rightArmEndLocal, rightArmBendLocal);
  //    float foreLength = Length(foreVec);
  //    if (foreLength < 0.0001f) {
  //      foreLength = 0.0001f;
  //      foreVec = {0.0f, -1.0f, 0.0f};
  //    }

  //    Vector3 foreDir = Normalize(foreVec);
  //    float rightForeArmAngleZ = atan2(foreDir.x, -foreDir.y);

  //    float rightUpperArmSwing = -leftLegBend_ * armSwingScale;
  //    float rightElbowFold = std::clamp((leftLegBend_ - legKickAngle_) /
  //                                          (legRecoverAngle_ -
  //                                          legKickAngle_),
  //                                      0.0f, 1.0f);

  //    float rightForeArmX =
  //        -(rightUpperArmSwing * 0.35f + rightElbowFold * 0.45f + 0.20f);

  //    // Vector3 rightAnimatedElbowPos = rightShoulderAnchorLocal;
  //    Vector3 rightAnimatedElbowPos =
  //        rightUpperArm->mainPosition.transform.translate;
  //    rightAnimatedElbowPos.x += std::sin(rightUpperArmAngleZ) *
  //                               std::cos(-leftLegBend_ * armSwingScale) *
  //                               upperLength;
  //    rightAnimatedElbowPos.y += -std::cos(rightUpperArmAngleZ) *
  //                               std::cos(-leftLegBend_ * armSwingScale) *
  //                               upperLength;
  //    rightAnimatedElbowPos.z +=
  //        -std::sin(-leftLegBend_ * armSwingScale) * upperLength;

  //    rightForeArm->mainPosition.transform.translate = rightAnimatedElbowPos;
  //    float rightForeArmAngleX = -asinf(std::clamp(foreDir.z, -1.0f, 1.0f));
  //    float rightForeArmAnimX =
  //        (-(rightUpperArmSwing * 0.35f + rightElbowFold * 0.45f)) *
  //        poseAnimScale;

  //    rightForeArm->mainPosition.transform.rotate = {
  //        rightForeArmAngleX + rightForeArmAnimX, 0.0f, rightForeArmAngleZ};

  //    rightForeArm->objectParts_[0].transform.translate = {
  //        0.0f, -foreLength * 0.5f, 0.0f};
  //    rightForeArm->objectParts_[0].transform.scale.x =
  //        rightForeArmThicknessScale * rightForeArmParam.scale.x;
  //    rightForeArm->objectParts_[0].transform.scale.y =
  //        foreLength * rightForeArmParam.scale.y * rightForeArmParam.length;
  //    rightForeArm->objectParts_[0].transform.scale.z =
  //        rightForeArmThicknessScale * rightForeArmParam.scale.z;
  //  }
  //}

  //================================================
  // 左腿
  //================================================
  // leftThigh->objectParts_[0].transform.translate = {
  //    0.0f, -leftThighLength * 0.5f, 0.0f};
  // leftThigh->objectParts_[0].transform.scale.x =
  //    leftThighThicknessScale * leftThighParam.scale.x;
  // leftThigh->objectParts_[0].transform.scale.y =
  //    leftThighLength * leftThighParam.scale.y * leftThighParam.length;
  // leftThigh->objectParts_[0].transform.scale.z =
  //    leftThighThicknessScale * leftThighParam.scale.z;

  //================================================
  // 左脛
  //================================================
  // leftShin->objectParts_[0].transform.translate = {0.0f, -leftShinLength *
  // 0.5f,
  //                                                 0.0f};
  // leftShin->objectParts_[0].transform.scale.x =
  //    leftShinThicknessScale * leftShinParam.scale.x;
  // leftShin->objectParts_[0].transform.scale.y =
  //    leftShinLength * leftShinParam.scale.y * leftShinParam.length;
  // leftShin->objectParts_[0].transform.scale.z =
  //    leftShinThicknessScale * leftShinParam.scale.z;

  //================================================
  // 右腿
  //================================================
  // rightThigh->objectParts_[0].transform.translate = {
  //    0.0f, -rightThighLength * 0.5f, 0.0f};
  // rightThigh->objectParts_[0].transform.scale.x =
  //    rightThighThicknessScale * rightThighParam.scale.x;
  // rightThigh->objectParts_[0].transform.scale.y =
  //    rightThighLength * rightThighParam.scale.y * rightThighParam.length;
  // rightThigh->objectParts_[0].transform.scale.z =
  //    rightThighThicknessScale * rightThighParam.scale.z;

  //================================================
  // 右脛
  //================================================
  // rightShin->objectParts_[0].transform.translate = {
  //    0.0f, -rightShinLength * 0.5f, 0.0f};
  // rightShin->objectParts_[0].transform.scale.x =
  //    rightShinThicknessScale * rightShinParam.scale.x;
  // rightShin->objectParts_[0].transform.scale.y =
  //    rightShinLength * rightShinParam.scale.y * rightShinParam.length;
  // rightShin->objectParts_[0].transform.scale.z =
  //    rightShinThicknessScale * rightShinParam.scale.z;

  ////================================================
  //// 左腿＋左脛（snapshotベース）
  ////================================================
  // int leftThighOwnerId = -1;
  // Vector3 leftHipAnchorLocal = {-0.5f, -1.25f, 0.0f};

  // if (customizeData_ != nullptr) {
  //   for (const auto &instance : customizeData_->partInstances) {
  //     if (instance.partType == ModBodyPart::LeftThigh) {
  //       leftThighOwnerId = instance.partId;
  //       leftHipAnchorLocal = instance.localTransform.translate;
  //       break;
  //     }
  //   }
  // }

  // Vector3 leftLegRootLocal = {0.0f, 0.0f, 0.0f};
  // Vector3 leftLegBendLocal = {0.0f, -0.70f, 0.0f};
  // Vector3 leftLegEndLocal = {0.0f, -1.40f, 0.0f};

  // bool hasLeftLegRoot = false;
  // bool hasLeftLegBend = false;
  // bool hasLeftLegEnd = false;

  // if (customizeData_ != nullptr && leftThighOwnerId >= 0) {
  //   for (const auto &snap : customizeData_->controlPointSnapshots) {
  //     if (snap.ownerPartId != leftThighOwnerId) {
  //       continue;
  //     }

  //    if (snap.role == ModControlPointRole::Root) {
  //      leftLegRootLocal = snap.localPosition;
  //      hasLeftLegRoot = true;
  //    } else if (snap.role == ModControlPointRole::Bend) {
  //      leftLegBendLocal = snap.localPosition;
  //      hasLeftLegBend = true;
  //    } else if (snap.role == ModControlPointRole::End) {
  //      leftLegEndLocal = snap.localPosition;
  //      hasLeftLegEnd = true;
  //    }
  //  }
  //}

  // if (hasLeftLegRoot && hasLeftLegBend) {
  //   Vector3 thighVec = Sub(leftLegBendLocal, leftLegRootLocal);
  //   float thighLength = Length(thighVec);
  //   if (thighLength < 0.0001f) {
  //     thighLength = 0.0001f;
  //     thighVec = {0.0f, -1.0f, 0.0f};
  //   }

  //  Vector3 thighDir = Normalize(thighVec);
  //  float leftThighAngleZ = atan2(thighDir.x, -thighDir.y);

  //  leftThigh->mainPosition.transform.translate = leftHipAnchorLocal;
  //  // leftThigh->mainPosition.transform.rotate = {-leftLegBend_ * 0.7f, 0.0f,
  //  //                                             leftThighAngleZ};
  //  float leftThighAnimX = (-leftLegBend_ * 0.7f) * poseAnimScale;

  //  leftThigh->mainPosition.transform.rotate = {leftThighAnimX, 0.0f,
  //                                              leftThighAngleZ};

  //  leftThigh->objectParts_[0].transform.translate = {0.0f, -thighLength *
  //  0.5f,
  //                                                    0.0f};
  //  leftThigh->objectParts_[0].transform.scale.x =
  //      leftThighThicknessScale * leftThighParam.scale.x;
  //  leftThigh->objectParts_[0].transform.scale.y =
  //      thighLength * leftThighParam.scale.y * leftThighParam.length;
  //  leftThigh->objectParts_[0].transform.scale.z =
  //      leftThighThicknessScale * leftThighParam.scale.z;

  //  if (hasLeftLegEnd) {
  //    Vector3 shinVec = Sub(leftLegEndLocal, leftLegBendLocal);
  //    float shinLength = Length(shinVec);
  //    if (shinLength < 0.0001f) {
  //      shinLength = 0.0001f;
  //      shinVec = {0.0f, -1.0f, 0.0f};
  //    }

  //    Vector3 shinDir = Normalize(shinVec);
  //    float leftShinAngleZ = atan2(shinDir.x, -shinDir.y);

  //    float leftThighSwing = -leftLegBend_ * 0.7f;
  //    float leftKneeFold = std::clamp((leftLegBend_ - legKickAngle_) /
  //                                        (legRecoverAngle_ - legKickAngle_),
  //                                    0.0f, 1.0f);

  //    float leftShinX = leftThighSwing * 0.35f + leftKneeFold * 0.6f + 0.3f;

  //    Vector3 leftAnimatedKneePos = leftHipAnchorLocal;
  //    leftAnimatedKneePos.x += std::sin(leftThighAngleZ) *
  //                             std::cos(-leftLegBend_ * 0.7f) * thighLength;
  //    leftAnimatedKneePos.y += -std::cos(leftThighAngleZ) *
  //                             std::cos(-leftLegBend_ * 0.7f) * thighLength;
  //    leftAnimatedKneePos.z += -std::sin(-leftLegBend_ * 0.7f) * thighLength;

  //    leftShin->mainPosition.transform.translate = leftAnimatedKneePos;
  //    // leftShin->mainPosition.transform.rotate = {leftShinX, 0.0f,
  //    //                                            leftShinAngleZ};
  //    float leftShinAnimX =
  //        (leftThighSwing * 0.35f + leftKneeFold * 0.6f) * poseAnimScale;
  //    leftShin->mainPosition.transform.rotate = {leftShinAnimX, 0.0f,
  //                                               leftShinAngleZ};

  //    leftShin->objectParts_[0].transform.translate = {0.0f, -shinLength *
  //    0.5f,
  //                                                     0.0f};
  //    leftShin->objectParts_[0].transform.scale.x =
  //        leftShinThicknessScale * leftShinParam.scale.x;
  //    leftShin->objectParts_[0].transform.scale.y =
  //        shinLength * leftShinParam.scale.y * leftShinParam.length;
  //    leftShin->objectParts_[0].transform.scale.z =
  //        leftShinThicknessScale * leftShinParam.scale.z;
  //  }
  //}

  ////================================================
  //// 右腿＋右脛（snapshotベース）
  ////================================================
  // int rightThighOwnerId = -1;
  // Vector3 rightHipAnchorLocal = {0.5f, -1.25f, 0.0f};

  // if (customizeData_ != nullptr) {
  //   for (const auto &instance : customizeData_->partInstances) {
  //     if (instance.partType == ModBodyPart::RightThigh) {
  //       rightThighOwnerId = instance.partId;
  //       rightHipAnchorLocal = instance.localTransform.translate;
  //       break;
  //     }
  //   }
  // }

  // Vector3 rightLegRootLocal = {0.0f, 0.0f, 0.0f};
  // Vector3 rightLegBendLocal = {0.0f, -0.70f, 0.0f};
  // Vector3 rightLegEndLocal = {0.0f, -1.40f, 0.0f};

  // bool hasRightLegRoot = false;
  // bool hasRightLegBend = false;
  // bool hasRightLegEnd = false;

  // if (customizeData_ != nullptr && rightThighOwnerId >= 0) {
  //   for (const auto &snap : customizeData_->controlPointSnapshots) {
  //     if (snap.ownerPartId != rightThighOwnerId) {
  //       continue;
  //     }

  //    if (snap.role == ModControlPointRole::Root) {
  //      rightLegRootLocal = snap.localPosition;
  //      hasRightLegRoot = true;
  //    } else if (snap.role == ModControlPointRole::Bend) {
  //      rightLegBendLocal = snap.localPosition;
  //      hasRightLegBend = true;
  //    } else if (snap.role == ModControlPointRole::End) {
  //      rightLegEndLocal = snap.localPosition;
  //      hasRightLegEnd = true;
  //    }
  //  }
  //}

  // if (hasRightLegRoot && hasRightLegBend) {
  //   Vector3 thighVec = Sub(rightLegBendLocal, rightLegRootLocal);
  //   float thighLength = Length(thighVec);
  //   if (thighLength < 0.0001f) {
  //     thighLength = 0.0001f;
  //     thighVec = {0.0f, -1.0f, 0.0f};
  //   }

  //  Vector3 thighDir = Normalize(thighVec);
  //  float rightThighAngleZ = atan2(thighDir.x, -thighDir.y);

  //  rightThigh->mainPosition.transform.translate = rightHipAnchorLocal;
  //  // rightThigh->mainPosition.transform.rotate = {-rightLegBend_ * 0.7f,
  //  0.0f,
  //  //                                              rightThighAngleZ};
  //  float rightThighAnimX = (-rightLegBend_ * 0.7f) * poseAnimScale;

  //  rightThigh->mainPosition.transform.rotate = {rightThighAnimX, 0.0f,
  //                                               rightThighAngleZ};

  //  rightThigh->objectParts_[0].transform.translate = {
  //      0.0f, -thighLength * 0.5f, 0.0f};
  //  rightThigh->objectParts_[0].transform.scale.x =
  //      rightThighThicknessScale * rightThighParam.scale.x;
  //  rightThigh->objectParts_[0].transform.scale.y =
  //      thighLength * rightThighParam.scale.y * rightThighParam.length;
  //  rightThigh->objectParts_[0].transform.scale.z =
  //      rightThighThicknessScale * rightThighParam.scale.z;

  //  if (hasRightLegEnd) {
  //    Vector3 shinVec = Sub(rightLegEndLocal, rightLegBendLocal);
  //    float shinLength = Length(shinVec);
  //    if (shinLength < 0.0001f) {
  //      shinLength = 0.0001f;
  //      shinVec = {0.0f, -1.0f, 0.0f};
  //    }

  //    Vector3 shinDir = Normalize(shinVec);
  //    float rightShinAngleZ = atan2(shinDir.x, -shinDir.y);

  //    float rightThighSwing = -rightLegBend_ * 0.7f;
  //    float rightKneeFold = std::clamp((rightLegBend_ - legKickAngle_) /
  //                                         (legRecoverAngle_ - legKickAngle_),
  //                                     0.0f, 1.0f);

  //    float rightShinX = rightThighSwing * 0.35f + rightKneeFold * 0.6f +
  //    0.3f;

  //    Vector3 rightAnimatedKneePos = rightHipAnchorLocal;
  //    rightAnimatedKneePos.x += std::sin(rightThighAngleZ) *
  //                              std::cos(-rightLegBend_ * 0.7f) * thighLength;
  //    rightAnimatedKneePos.y += -std::cos(rightThighAngleZ) *
  //                              std::cos(-rightLegBend_ * 0.7f) * thighLength;
  //    rightAnimatedKneePos.z += -std::sin(-rightLegBend_ * 0.7f) *
  //    thighLength;

  //    rightShin->mainPosition.transform.translate = rightAnimatedKneePos;
  //    // rightShin->mainPosition.transform.rotate = {rightShinX, 0.0f,
  //    //                                             rightShinAngleZ};
  //    float rightShinAnimX =
  //        (rightThighSwing * 0.35f + rightKneeFold * 0.6f) * poseAnimScale;

  //    rightShin->mainPosition.transform.rotate = {rightShinAnimX, 0.0f,
  //                                                rightShinAngleZ};

  //    rightShin->objectParts_[0].transform.translate = {
  //        0.0f, -shinLength * 0.5f, 0.0f};
  //    rightShin->objectParts_[0].transform.scale.x =
  //        rightShinThicknessScale * rightShinParam.scale.x;
  //    rightShin->objectParts_[0].transform.scale.y =
  //        shinLength * rightShinParam.scale.y * rightShinParam.length;
  //    rightShin->objectParts_[0].transform.scale.z =
  //        rightShinThicknessScale * rightShinParam.scale.z;
  //  }
  //}
}

float TravelScene::GetControlPointRadius(ModControlPointRole role) const {
  if (customizeData_ == nullptr) {
    return 0.1f;
  }

  for (const auto &snap : customizeData_->controlPointSnapshots) {
    if (snap.role == role) {
      return snap.radius;
    }
  }

  return 1.0f;
}

float TravelScene::GetSnapshotRadius(ModBodyPart ownerPart,
                                     int localRole) const {
  if (customizeData_ == nullptr) {
    return 0.1f;
  }

  for (const auto &snap : customizeData_->controlPointSnapshots) {
    if (snap.ownerPartType == ownerPart &&
        static_cast<int>(snap.role) == localRole) {
      return snap.radius;
    }
  }

  return 0.1f;
}

bool TravelScene::BuildSegmentFromSnapshot(ModBodyPart partType, int partId,
                                           SegmentVisual &out) {
  if (!customizeData_)
    return false;

  const auto &snaps = customizeData_->controlPointSnapshots;

  Vector3 rootPos{};
  Vector3 midPos{};
  Vector3 endPos{};

  bool hasRoot = false;
  bool hasBend = false;
  bool hasEnd = false;

  // --- 必要なスナップショット収集 ---
  for (const auto &s : snaps) {
    if (s.ownerPartId != partId)
      continue;

    switch (s.role) {
    case ModControlPointRole::Root:
      rootPos = s.localPosition;
      hasRoot = true;
      break;
    case ModControlPointRole::Bend:
      midPos = s.localPosition;
      hasBend = true;
      break;
    case ModControlPointRole::End:
      endPos = s.localPosition;
      hasEnd = true;
      break;
    default:
      break;
    }
  }

  // --- パーツごとに使う区間を決定 ---
  Vector3 start{};
  Vector3 end{};

  if (partType == ModBodyPart::LeftUpperArm ||
      partType == ModBodyPart::RightUpperArm ||
      partType == ModBodyPart::LeftThigh ||
      partType == ModBodyPart::RightThigh || partType == ModBodyPart::Neck) {
    if (!hasRoot || !hasBend)
      return false;
    start = rootPos;
    end = midPos;
  } else if (partType == ModBodyPart::LeftForeArm ||
             partType == ModBodyPart::RightForeArm ||
             partType == ModBodyPart::LeftShin ||
             partType == ModBodyPart::RightShin ||
             partType == ModBodyPart::Head) {
    if (!hasBend || !hasEnd)
      return false;
    start = midPos;
    end = endPos;
  } else {
    return false;
  }

  Vector3 diff = {end.x - start.x, end.y - start.y, end.z - start.z};

  float length = sqrtf(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
  if (length < 0.0001f) {
    return false;
  }

  Vector3 dir = {diff.x / length, diff.y / length, diff.z / length};

  float angleZ = atan2f(dir.x, -dir.y);
  float angleX = -asinf(std::clamp(dir.z, -1.0f, 1.0f));

  out.root = start;
  out.length = length;
  out.angleX = angleX;
  out.angleZ = angleZ;
  out.thickness = 1.0f;

  return true;
}

void TravelScene::PrepareTorsoApplySource() {
  if (!useModBodyApplyTorso_) {
    return;
  }

  Object *chestBody = modObjects_[ToIndex(ModBodyPart::ChestBody)];
  Object *stomachBody = modObjects_[ToIndex(ModBodyPart::StomachBody)];
  if (chestBody == nullptr || stomachBody == nullptr) {
    return;
  }

  int torsoOwnerId = -1;
  if (customizeData_ != nullptr) {
    for (const auto &instance : customizeData_->partInstances) {
      if (instance.partType == ModBodyPart::ChestBody) {
        torsoOwnerId = instance.partId;
        break;
      }
    }
  }

  if (torsoOwnerId < 0) {
    return;
  }

  std::vector<ModControlPoint> torsoPoints;
  torsoPoints.reserve(3);

  const ModControlPointSnapshot *chestSnap = nullptr;
  const ModControlPointSnapshot *bellySnap = nullptr;
  const ModControlPointSnapshot *waistSnap = nullptr;

  for (const auto &snap : customizeData_->controlPointSnapshots) {
    if (snap.ownerPartId != torsoOwnerId) {
      continue;
    }

    if (snap.role == ModControlPointRole::Chest) {
      chestSnap = &snap;
    } else if (snap.role == ModControlPointRole::Belly) {
      bellySnap = &snap;
    } else if (snap.role == ModControlPointRole::Waist) {
      waistSnap = &snap;
    }
  }

  if (chestSnap == nullptr || bellySnap == nullptr || waistSnap == nullptr) {
    return;
  }

  auto PushPoint = [&](const ModControlPointSnapshot *snap) {
    ModControlPoint point{};
    point.role = snap->role;
    point.localPosition = snap->localPosition;
    point.radius = snap->radius;
    point.movable = snap->movable;
    point.isConnectionPoint = snap->isConnectionPoint;
    point.acceptsParent = snap->acceptsParent;
    point.acceptsChild = snap->acceptsChild;
    torsoPoints.push_back(point);
  };

  PushPoint(chestSnap);
  PushPoint(bellySnap);
  PushPoint(waistSnap);

  torsoSharedPointsBuffer_.clear();
  torsoSharedPointsBuffer_ = torsoPoints;

  modBodies_[ToIndex(ModBodyPart::ChestBody)].ClearExternalSegmentSource();
  modBodies_[ToIndex(ModBodyPart::StomachBody)].ClearExternalSegmentSource();

  modBodies_[ToIndex(ModBodyPart::ChestBody)].SetExternalSegmentSource(
      &torsoSharedPointsBuffer_, ModControlPointRole::Chest,
      ModControlPointRole::Belly);

  modBodies_[ToIndex(ModBodyPart::StomachBody)].SetExternalSegmentSource(
      &torsoSharedPointsBuffer_, ModControlPointRole::Belly,
      ModControlPointRole::Waist);
}

float TravelScene::GetLowestVisualBodyY(LowestBodyPart *outPart) const {
  auto UpdateLowest = [&](float candidateY, LowestBodyPart part, float &lowestY,
                          LowestBodyPart &lowestPart) {
    if (candidateY < lowestY) {
      lowestY = candidateY;
      lowestPart = part;
    }
  };

  float lowestY = 999999.0f;
  LowestBodyPart lowestPart = LowestBodyPart::None;

  auto PartBottomY = [&](Object *obj, float radius) -> float {
    if (obj == nullptr) {
      return 999999.0f;
    }

    const Vector3 &p = obj->mainPosition.transform.translate;
    return p.y - radius;
  };

  auto Add = [](const Vector3 &a, const Vector3 &b) -> Vector3 {
    return Vector3{a.x + b.x, a.y + b.y, a.z + b.z};
  };

  auto Sub = [](const Vector3 &a, const Vector3 &b) -> Vector3 {
    return Vector3{a.x - b.x, a.y - b.y, a.z - b.z};
  };

  if (customizeData_ != nullptr) {
    //================================
    // anchor と snapshot を集める
    //================================
    int torsoAnchorOwnerId = -1;
    int leftUpperArmOwnerId = -1;
    int rightUpperArmOwnerId = -1;
    int leftThighOwnerId = -1;
    int rightThighOwnerId = -1;

    Vector3 leftShoulderAnchorLocal = {-1.25f, 1.0f, 0.0f};
    Vector3 rightShoulderAnchorLocal = {1.25f, 1.0f, 0.0f};
    Vector3 leftHipAnchorLocal = {-0.5f, -1.25f, 0.0f};
    Vector3 rightHipAnchorLocal = {0.5f, -1.25f, 0.0f};

    Vector3 leftArmRootLocal = {0.0f, 0.0f, 0.0f};
    Vector3 leftArmEndLocal = {0.0f, -1.10f, 0.0f};

    Vector3 rightArmRootLocal = {0.0f, 0.0f, 0.0f};
    Vector3 rightArmEndLocal = {0.0f, -1.10f, 0.0f};

    Vector3 leftLegRootLocal = {0.0f, 0.0f, 0.0f};
    Vector3 leftLegEndLocal = {0.0f, -1.40f, 0.0f};

    Vector3 rightLegRootLocal = {0.0f, 0.0f, 0.0f};
    Vector3 rightLegEndLocal = {0.0f, -1.40f, 0.0f};

    bool hasLeftArmRoot = false;
    bool hasLeftArmEnd = false;
    bool hasRightArmRoot = false;
    bool hasRightArmEnd = false;
    bool hasLeftLegRoot = false;
    bool hasLeftLegEnd = false;
    bool hasRightLegRoot = false;
    bool hasRightLegEnd = false;

    for (const auto &instance : customizeData_->partInstances) {
      if (torsoAnchorOwnerId < 0 &&
          instance.partType == ModBodyPart::ChestBody) {
        torsoAnchorOwnerId = instance.partId;
      } else if (leftUpperArmOwnerId < 0 &&
                 instance.partType == ModBodyPart::LeftUpperArm) {
        leftUpperArmOwnerId = instance.partId;
      } else if (rightUpperArmOwnerId < 0 &&
                 instance.partType == ModBodyPart::RightUpperArm) {
        rightUpperArmOwnerId = instance.partId;
      } else if (leftThighOwnerId < 0 &&
                 instance.partType == ModBodyPart::LeftThigh) {
        leftThighOwnerId = instance.partId;
      } else if (rightThighOwnerId < 0 &&
                 instance.partType == ModBodyPart::RightThigh) {
        rightThighOwnerId = instance.partId;
      }
    }

    for (const auto &snap : customizeData_->controlPointSnapshots) {
      if (snap.ownerPartId == torsoAnchorOwnerId) {
        if (snap.role == ModControlPointRole::LeftShoulder) {
          leftShoulderAnchorLocal = snap.localPosition;
        } else if (snap.role == ModControlPointRole::RightShoulder) {
          rightShoulderAnchorLocal = snap.localPosition;
        } else if (snap.role == ModControlPointRole::LeftHip) {
          leftHipAnchorLocal = snap.localPosition;
        } else if (snap.role == ModControlPointRole::RightHip) {
          rightHipAnchorLocal = snap.localPosition;
        }
      }

      if (snap.ownerPartId == leftUpperArmOwnerId) {
        if (snap.role == ModControlPointRole::Root) {
          leftArmRootLocal = snap.localPosition;
          hasLeftArmRoot = true;
        } else if (snap.role == ModControlPointRole::End) {
          leftArmEndLocal = snap.localPosition;
          hasLeftArmEnd = true;
        }
      }

      if (snap.ownerPartId == rightUpperArmOwnerId) {
        if (snap.role == ModControlPointRole::Root) {
          rightArmRootLocal = snap.localPosition;
          hasRightArmRoot = true;
        } else if (snap.role == ModControlPointRole::End) {
          rightArmEndLocal = snap.localPosition;
          hasRightArmEnd = true;
        }
      }

      if (snap.ownerPartId == leftThighOwnerId) {
        if (snap.role == ModControlPointRole::Root) {
          leftLegRootLocal = snap.localPosition;
          hasLeftLegRoot = true;
        } else if (snap.role == ModControlPointRole::End) {
          leftLegEndLocal = snap.localPosition;
          hasLeftLegEnd = true;
        }
      }

      if (snap.ownerPartId == rightThighOwnerId) {
        if (snap.role == ModControlPointRole::Root) {
          rightLegRootLocal = snap.localPosition;
          hasRightLegRoot = true;
        } else if (snap.role == ModControlPointRole::End) {
          rightLegEndLocal = snap.localPosition;
          hasRightLegEnd = true;
        }
      }
    }

    //================================
    // 前腕：anchor + (end - root) - radius
    // grounded 側と同じ考え方
    //================================
    if (hasLeftArmRoot && hasLeftArmEnd) {
      const float r = GetSnapshotRadius(ModBodyPart::LeftUpperArm, 3);
      float y = leftShoulderAnchorLocal.y +
                (leftArmEndLocal.y - leftArmRootLocal.y) - r;
      UpdateLowest(y, LowestBodyPart::LeftForeArm, lowestY, lowestPart);
    }

    if (hasRightArmRoot && hasRightArmEnd) {
      const float r = GetSnapshotRadius(ModBodyPart::RightUpperArm, 3);
      float y = rightShoulderAnchorLocal.y +
                (rightArmEndLocal.y - rightArmRootLocal.y) - r;
      UpdateLowest(y, LowestBodyPart::RightForeArm, lowestY, lowestPart);
    }

    //================================
    // 脚：anchor + (end - root) - radius
    // これは grounded 側と完全に揃える
    //================================
    if (hasLeftLegRoot && hasLeftLegEnd) {
      const float r = GetSnapshotRadius(ModBodyPart::LeftThigh, 3);
      float y =
          leftHipAnchorLocal.y + (leftLegEndLocal.y - leftLegRootLocal.y) - r;
      UpdateLowest(y, LowestBodyPart::LeftShin, lowestY, lowestPart);
    }

    if (hasRightLegRoot && hasRightLegEnd) {
      const float r = GetSnapshotRadius(ModBodyPart::RightThigh, 3);
      float y = rightHipAnchorLocal.y +
                (rightLegEndLocal.y - rightLegRootLocal.y) - r;
      UpdateLowest(y, LowestBodyPart::RightShin, lowestY, lowestPart);
    }
  }

  //================================
  // 頭・胴体は今はいったん簡易でOK
  //================================
  {
    Object *head = modObjects_[ToIndex(ModBodyPart::Head)];
    const float upperR = GetSnapshotRadius(ModBodyPart::Neck, 2);
    const float endR = GetSnapshotRadius(ModBodyPart::Neck, 3);
    const float r = (std::max)(upperR, endR);
    float y = PartBottomY(head, r);
    UpdateLowest(y, LowestBodyPart::Head, lowestY, lowestPart);
  }

  {
    Object *chest = modObjects_[ToIndex(ModBodyPart::ChestBody)];
    const float chestR = GetControlPointRadius(ModControlPointRole::Chest);
    const float bellyR = GetControlPointRadius(ModControlPointRole::Belly);
    const float r = (std::max)(chestR, bellyR);
    float y = PartBottomY(chest, r);
    UpdateLowest(y, LowestBodyPart::Chest, lowestY, lowestPart);
  }

  {
    Object *stomach = modObjects_[ToIndex(ModBodyPart::StomachBody)];
    const float bellyR = GetControlPointRadius(ModControlPointRole::Belly);
    const float waistR = GetControlPointRadius(ModControlPointRole::Waist);
    const float r = (std::max)(bellyR, waistR);
    float y = PartBottomY(stomach, r);
    UpdateLowest(y, LowestBodyPart::Stomach, lowestY, lowestPart);
  }

  if (outPart != nullptr) {
    *outPart = lowestPart;
  }

  return lowestY;
}

const char *TravelScene::GetLowestBodyPartName(LowestBodyPart part) const {
  switch (part) {
  case LowestBodyPart::LeftForeArm:
    return "LeftForeArm";
  case LowestBodyPart::RightForeArm:
    return "RightForeArm";
  case LowestBodyPart::LeftShin:
    return "LeftShin";
  case LowestBodyPart::RightShin:
    return "RightShin";
  case LowestBodyPart::Head:
    return "Head";
  case LowestBodyPart::Chest:
    return "Chest";
  case LowestBodyPart::Stomach:
    return "Stomach";
  default:
    return "None";
  }
}

void TravelScene::ResolveVisualGroundPenetration() {

  LowestBodyPart lowestPart = LowestBodyPart::None;
  float lowestBodyLocalY = GetLowestVisualBodyY(&lowestPart);
  float lowestBodyWorldY = moveY_ + visualLiftY_ + lowestBodyLocalY;
  float penetration = groundY_ - lowestBodyWorldY;

  const bool lowestIsLeg = (lowestPart == LowestBodyPart::LeftShin ||
                            lowestPart == LowestBodyPart::RightShin);

  static int visualGroundCheckFrame = 0;
  visualGroundCheckFrame++;

  if (lowestIsLeg) {
    visualLiftY_ *= 0.60f;

    if (std::abs(visualLiftY_) < 0.0001f) {
      visualLiftY_ = 0.0f;
    }
    return;
  }

  if (penetration <= 0.0f) {
    visualLiftY_ *= 0.80f;
    if (std::abs(visualLiftY_) < 0.0001f) {
      visualLiftY_ = 0.0f;
    }
    return;
  }

  float follow = 0.40f;
  if (penetration > 0.30f) {
    follow = 0.55f;
  }
  if (penetration > 0.80f) {
    follow = 0.75f;
  }

  const float maxVisualLift = 2.50f;
  float targetLift = std::min(penetration, maxVisualLift);
  visualLiftY_ += (targetLift - visualLiftY_) * follow;
}

void TravelScene::InitializeNpcRunners() {
  npcRunners_.clear();
  npcRunners_.resize(4);

  for (size_t i = 0; i < npcRunners_.size(); ++i) {
    npcRunners_[i].moveX = -18.0f;
    npcRunners_[i].laneX = -3.0f + static_cast<float>(i) * 2.0f;

    npcRunners_[i].moveY = 1.94f;
    npcRunners_[i].velocityX = 0.0f;
    npcRunners_[i].velocityY = 0.0f;
    npcRunners_[i].leftLegBend = 0.0f;
    npcRunners_[i].rightLegBend = 0.0f;
    npcRunners_[i].leftLegBendSpeed = 0.0f;
    npcRunners_[i].rightLegBendSpeed = 0.0f;
    npcRunners_[i].leftInput = false;
    npcRunners_[i].rightInput = false;
    npcRunners_[i].isGrounded = true;
    npcRunners_[i].finished = false;
    npcRunners_[i].started = true;
    npcRunners_[i].startDelay = 0.0f;
    npcRunners_[i].finishRank = -1;

    npcRunners_[i].phaseTimer = 0.0f;
    npcRunners_[i].phase = static_cast<int>(i % 4);

    switch (i) {
    case 0:
      npcRunners_[i].timingSkill = 0.85f;
      break;
    case 1:
      npcRunners_[i].timingSkill = 0.95f;
      break;
    case 2:
      npcRunners_[i].timingSkill = 1.00f;
      break;
    default:
      npcRunners_[i].timingSkill = 1.10f;
      break;
    }

    // 既存debugObject
    npcRunners_[i].debugObject = std::make_unique<Object>();
    npcRunners_[i].debugObject->IntObject(system_);
    npcRunners_[i].debugObject->CreateModelData(npcModelHandle_);
    npcRunners_[i].debugObject->mainPosition.transform =
        CreateDefaultTransform();
    npcRunners_[i].debugObject->mainPosition.transform.scale = {0.6f, 0.6f,
                                                                0.6f};

    // 0番NPCだけ改造体化
    if (i == 0) {
      npcRunners_[i].customizeData = CreateNpcPresetHeadBig();
    } else if (i == 1) {
      npcRunners_[i].customizeData = CreateNpcPresetLongLeg();
    } else if (i == 2) {
      npcRunners_[i].customizeData = CreateNpcPresetBigTorso();
    } else {
      npcRunners_[i].customizeData = CreateNpcPresetDefault();
    }

    SetupNpcCustomizedVisual(npcRunners_[i]);
  }
}

void TravelScene::UpdateNpcInput(NpcRunner &npc, float deltaTime) {
  npc.leftInput = false;
  npc.rightInput = false;

  const float perfectTimingEnd = 0.08f * 0.45f;
  const float bestTimingEnd = 0.08f;
  const float lateTimingEnd = 0.22f;

  bool justLanded = (!npc.prevGrounded && npc.isGrounded);

  //==============================
  // 着地した瞬間に「今回いつ蹴るか」を決める
  // timingSkill が高いほど Perfect を引きやすい
  //==============================
  if (justLanded) {
    npc.isKickHolding = false;
    npc.hasKickPlan = true;

    float skill = std::clamp(npc.timingSkill, 0.70f, 1.30f);
    float skill01 = std::clamp((skill - 0.70f) / (1.30f - 0.70f), 0.0f, 1.0f);

    float perfectWeight = 0.15f + skill01 * 0.65f;
    float goodWeight = 0.55f - skill01 * 0.10f;
    float badWeight = 1.0f - perfectWeight - goodWeight;

    perfectWeight = std::clamp(perfectWeight, 0.05f, 0.85f);
    goodWeight = std::clamp(goodWeight, 0.10f, 0.75f);
    badWeight = std::clamp(badWeight, 0.05f, 0.70f);

    float roll = RandomFloat(0.0f, 1.0f);

    if (roll < perfectWeight) {
      npc.targetKickTime = RandomFloat(0.0f, perfectTimingEnd * 0.95f);
      npc.lastTimingResult = 1; // Perfect狙い
    } else if (roll < perfectWeight + goodWeight) {
      npc.targetKickTime = RandomFloat(perfectTimingEnd, bestTimingEnd);
      npc.lastTimingResult = 2; // Good狙い
    } else {
      npc.targetKickTime = RandomFloat(bestTimingEnd, lateTimingEnd + 0.05f);
      npc.lastTimingResult = 3; // Bad狙い
    }
  }

  //==============================
  // いま蹴った足を着地まで保持中
  //==============================
  if (npc.isKickHolding) {
    if (npc.kickHoldLeft) {
      npc.leftInput = true;
      npc.rightInput = false;
    } else {
      npc.leftInput = false;
      npc.rightInput = true;
    }

    // 着地した瞬間だけ保持終了
    if (justLanded) {
      npc.isKickHolding = false;
      npc.leftInput = false;
      npc.rightInput = false;
    }

    npc.prevGrounded = npc.isGrounded;
    return;
  }

  //==============================
  // 空中では新しい蹴りを出さない
  //==============================
  if (!npc.isGrounded) {
    npc.prevGrounded = npc.isGrounded;
    return;
  }

  //==============================
  // まだ今回の蹴り予定が無いなら、最低限作る
  // （初回や保険）
  //==============================
  if (!npc.hasKickPlan) {
    npc.hasKickPlan = true;
    npc.targetKickTime = RandomFloat(perfectTimingEnd, bestTimingEnd);
    npc.lastTimingResult = 2;
  }

  //==============================
  // 狙いタイミングが来たら蹴る
  // 左右交互
  //==============================
  if (npc.landTimer >= npc.targetKickTime) {
    npc.kickHoldLeft = !npc.kickHoldLeft;
    npc.isKickHolding = true;
    npc.hasKickPlan = false;

    if (npc.kickHoldLeft) {
      npc.leftInput = true;
      npc.rightInput = false;
    } else {
      npc.leftInput = false;
      npc.rightInput = true;
    }
  }

  npc.prevGrounded = npc.isGrounded;
}

void TravelScene::UpdateNpcMovement(NpcRunner &npc) {
  float leftTargetLegAngle = npc.leftInput ? legKickAngle_ : legRecoverAngle_;
  float rightTargetLegAngle = npc.rightInput ? legKickAngle_ : legRecoverAngle_;

  npc.leftLegBendSpeed +=
      (leftTargetLegAngle - npc.leftLegBend) * legFollowPower_;
  npc.rightLegBendSpeed +=
      (rightTargetLegAngle - npc.rightLegBend) * legFollowPower_;

  npc.leftLegBendSpeed =
      std::clamp(npc.leftLegBendSpeed, -legMaxSpeed_, legMaxSpeed_);
  npc.rightLegBendSpeed =
      std::clamp(npc.rightLegBendSpeed, -legMaxSpeed_, legMaxSpeed_);

  npc.leftLegBendSpeed *= jointDamping_;
  npc.rightLegBendSpeed *= jointDamping_;

  npc.leftLegBend += npc.leftLegBendSpeed;
  npc.rightLegBend += npc.rightLegBendSpeed;

  npc.leftLegBend = std::clamp(npc.leftLegBend, -0.8f, 1.0f);
  npc.rightLegBend = std::clamp(npc.rightLegBend, -0.8f, 1.0f);

  //================================
  // 接地高さ
  //================================
  float npcGroundY = 1.94f;

  if (npc.useCustomizedVisual) {
    Object *leftThigh = npc.modObjects[ToIndex(ModBodyPart::LeftThigh)];
    Object *leftShin = npc.modObjects[ToIndex(ModBodyPart::LeftShin)];
    Object *rightThigh = npc.modObjects[ToIndex(ModBodyPart::RightThigh)];
    Object *rightShin = npc.modObjects[ToIndex(ModBodyPart::RightShin)];
    Object *chestBody = npc.modObjects[ToIndex(ModBodyPart::ChestBody)];
    Object *stomachBody = npc.modObjects[ToIndex(ModBodyPart::StomachBody)];

    auto SafePartLength = [](Object *obj) -> float {
      if (obj == nullptr || obj->objectParts_.empty()) {
        return 1.0f;
      }
      return (std::max)(0.05f, obj->objectParts_[0].transform.scale.y);
    };

    const float leftThighLength = SafePartLength(leftThigh);
    const float leftShinLength = SafePartLength(leftShin);
    const float rightThighLength = SafePartLength(rightThigh);
    const float rightShinLength = SafePartLength(rightShin);

    const float chestLength = SafePartLength(chestBody);
    const float stomachLength = SafePartLength(stomachBody);

    const float avgLegLength = (leftThighLength + leftShinLength +
                                rightThighLength + rightShinLength) *
                               0.25f;

    const float avgTorsoLength = (chestLength + stomachLength) * 0.5f;

    const float baseLegLength = 1.0f;
    const float baseTorsoLength = 1.0f;

    npcGroundY = 1.94f;
    npcGroundY += (avgLegLength - baseLegLength) * 2.0f;
    npcGroundY += (avgTorsoLength - baseTorsoLength) * 1.2f;
  }

  bool wasGrounded = npc.isGrounded;

  if (npc.moveY <= npcGroundY) {
    npc.moveY = npcGroundY;
    npc.velocityY = 0.0f;
    npc.isGrounded = true;
  } else {
    npc.isGrounded = false;
  }

  //================================
  // 改造から NPC 用の簡易 tuning を作る
  // 脚長はここで直接強化せず、一歩の大きさだけに使う
  //================================
  float runPower = 1.0f;
  float lift = 1.0f;
  float maxSpeed = 1.0f;

  float headScale = 1.0f;
  float avgTorsoScale = 1.0f;

  if (npc.customizeData != nullptr) {
    const auto &chest =
        npc.customizeData->partParams[ToIndex(ModBodyPart::ChestBody)];
    const auto &stomach =
        npc.customizeData->partParams[ToIndex(ModBodyPart::StomachBody)];
    const auto &head =
        npc.customizeData->partParams[ToIndex(ModBodyPart::Head)];

    avgTorsoScale = (chest.scale.x + chest.scale.y + chest.scale.z +
                     stomach.scale.x + stomach.scale.y + stomach.scale.z) /
                    6.0f;

    headScale = (head.scale.x + head.scale.y + head.scale.z) / 3.0f;

    // 頭デカ：弱体
    runPower -= (headScale - 1.0f) * 0.6f;
    lift -= (headScale - 1.0f) * 0.4f;
    maxSpeed -= (headScale - 1.0f) * 0.5f;

    // 胴体デカ：少し鈍くする
    maxSpeed -= (avgTorsoScale - 1.0f) * 0.1f;

    runPower = std::clamp(runPower, 0.4f, 2.5f);
    lift = std::clamp(lift, 0.4f, 2.5f);
    maxSpeed = std::clamp(maxSpeed, 0.4f, 3.0f);
  }

  //================================
  // 姿勢：通常時は少し戻すだけ
  // プレイヤーと同じく、蹴った瞬間にだけ tiltImpulse を入れる
  //================================
  const float npcIdealRunTilt = -0.12f;
  const float npcNeutralTilt = -0.03f;
  const float npcRecoverStartTilt = 0.42f;
  const float npcRecoverTargetTilt = -0.12f;
  const float npcHeavyFallTilt = 0.65f;

  float headRecoveryPenalty =
      std::clamp(1.0f - (headScale - 1.0f) * 0.55f, 0.25f, 1.0f);

  float torsoStabilityScale =
      std::clamp(1.0f + (avgTorsoScale - 1.0f) * 3.0f, 0.45f, 4.2f);

  float torsoTiltResistance =
      std::clamp(1.0f - (avgTorsoScale - 1.0f) * 1.8f, 0.10f, 1.15f);

  float neutralReturnPower = 0.002f * headRecoveryPenalty * torsoStabilityScale;

  if (!npc.isGrounded) {
    neutralReturnPower *= 0.35f;
  }

  npc.bodyTiltVelocity += (npcNeutralTilt - npc.bodyTilt) * neutralReturnPower;

  //================================
  // NPC の蹴り
  // 脚長は「一歩の大きさ」だけに影響
  // 姿勢は landTimer に応じた蹴りタイミングで決める
  //================================
  bool doKick = npc.isGrounded && (npc.leftInput || npc.rightInput);

  if (doKick) {
    float avgLegScaleY = 1.0f;

    if (npc.customizeData != nullptr) {
      const auto &leftThigh =
          npc.customizeData->partParams[ToIndex(ModBodyPart::LeftThigh)];
      const auto &rightThigh =
          npc.customizeData->partParams[ToIndex(ModBodyPart::RightThigh)];
      const auto &leftShin =
          npc.customizeData->partParams[ToIndex(ModBodyPart::LeftShin)];
      const auto &rightShin =
          npc.customizeData->partParams[ToIndex(ModBodyPart::RightShin)];

      avgLegScaleY = (leftThigh.scale.y + rightThigh.scale.y +
                      leftShin.scale.y + rightShin.scale.y) *
                     0.25f;
    }

    float legLengthScale =
        std::clamp(1.0f + (avgLegScaleY - 1.0f) * 0.45f, 0.70f, 2.0f);

    float kickLegBend = 0.0f;
    if (npc.leftInput) {
      kickLegBend = npc.leftLegBend;
    } else if (npc.rightInput) {
      kickLegBend = npc.rightLegBend;
    }

    float kickLegForwardness =
        1.0f - std::clamp((kickLegBend - legKickAngle_) /
                              (legRecoverAngle_ - legKickAngle_),
                          0.0f, 1.0f);

    float kickEfficiency = 0.75f + kickLegForwardness * 0.75f;
    float groundBoost = npc.isGrounded ? 1.2f : 0.6f;

    float totalPush = 0.09f;

    //================================
    // 蹴ったタイミングで姿勢インパルス
    // Perfect / Good / Bad をちゃんと分ける
    //================================
    const float bestTimingEnd = 0.08f;
    const float lateTimingEnd = 0.22f;
    const float perfectTimingEnd = bestTimingEnd * 0.45f;

    float tiltImpulse = 0.0f;

    if (npc.landTimer <= perfectTimingEnd) {
      // Perfect：はっきり前傾
      tiltImpulse = -0.055f;
      npc.lastTimingResult = 1;

    } else if (npc.landTimer <= bestTimingEnd) {
      // Good：少し前傾
      float goodRatio = (npc.landTimer - perfectTimingEnd) /
                        (bestTimingEnd - perfectTimingEnd);
      goodRatio = std::clamp(goodRatio, 0.0f, 1.0f);

      tiltImpulse = -0.035f + goodRatio * 0.015f;
      npc.lastTimingResult = 2;

    } else if (npc.landTimer <= lateTimingEnd) {
      // Bad前半：少しずつ後傾
      float badRatio =
          (npc.landTimer - bestTimingEnd) / (lateTimingEnd - bestTimingEnd);
      badRatio = std::clamp(badRatio, 0.0f, 1.0f);

      tiltImpulse = 0.020f + badRatio * 0.035f;
      npc.lastTimingResult = 3;

    } else {
      // Bad後半：かなり後傾
      tiltImpulse = 0.075f;
      npc.lastTimingResult = 3;
    }

    float headHeavyFactor =
        std::clamp(1.0f + (headScale - 1.0f) * 1.6f, 1.0f, 2.4f);

    float turnResponse = 1.0f;

    // 頭が大きいほど振られやすい
    turnResponse += (headScale - 1.0f) * 1.2f;

    // 胴体が大きいほど少し落ち着く
    turnResponse -= (avgTorsoScale - 1.0f) * 0.5f;

    turnResponse = std::clamp(turnResponse, 0.7f, 2.5f);

    float npcBaseTiltResponse = 2.2f;

    npc.bodyTiltVelocity += tiltImpulse * npcBaseTiltResponse *
                            headHeavyFactor * torsoTiltResistance;

    float pushMagnitude = totalPush *
                          (0.55f + runPower * 1.10f + lift * 0.75f) *
                          legLengthScale * kickEfficiency * groundBoost;

    //================================
    // 姿勢から方向だけ決める
    //================================
    float tiltDiff = npc.bodyTilt - npcIdealRunTilt;

    float forwardRatio = 0.5f - tiltDiff * 0.25f;
    float upwardRatio = 0.5f + tiltDiff * 0.25f;

    if (tiltDiff > 0.0f) {
      float backwardPenalty = std::clamp(tiltDiff / 0.22f, 0.0f, 1.0f);
      forwardRatio *= (1.0f - backwardPenalty * 0.55f);
    }

    forwardRatio = std::clamp(forwardRatio, 0.15f, 1.50f);
    upwardRatio = std::clamp(upwardRatio, 0.15f, 1.50f);

    float dirLen =
        std::sqrt(forwardRatio * forwardRatio + upwardRatio * upwardRatio);

    if (dirLen > 0.0001f) {
      forwardRatio /= dirLen;
      upwardRatio /= dirLen;
    } else {
      forwardRatio = 0.707f;
      upwardRatio = 0.707f;
    }

    float pushX = pushMagnitude * forwardRatio;
    float pushY = pushMagnitude * upwardRatio;

    npc.velocityX += pushX;
    npc.velocityY += pushY;
  }

  //================================
  // 姿勢更新
  //================================
  float tiltDamping =
      std::clamp(0.82f + (avgTorsoScale - 1.0f) * 0.08f, 0.76f, 0.92f);
  npc.bodyTiltVelocity *= tiltDamping;

  float headTiltRangeFactor =
      std::clamp(1.0f + (headScale - 1.0f) * 1.60f, 1.0f, 3.20f);

  float torsoTiltRangeFactor =
      std::clamp(1.0f - (avgTorsoScale - 1.0f) * 0.45f, 0.65f, 1.10f);

  float dynamicForwardTilt =
      maxForwardTilt_ * headTiltRangeFactor * torsoTiltRangeFactor;
  float dynamicBackwardTilt =
      maxBackwardTilt_ * headTiltRangeFactor * torsoTiltRangeFactor;

  npc.bodyTilt += npc.bodyTiltVelocity;
  npc.bodyTilt =
      std::clamp(npc.bodyTilt, dynamicForwardTilt, dynamicBackwardTilt);

  if (std::abs(npc.bodyTilt) > npcRecoverStartTilt) {
    float recoverPower = 0.010f * headRecoveryPenalty * torsoStabilityScale;

    if (std::abs(npc.bodyTilt) > npcHeavyFallTilt) {
      recoverPower *= 1.4f;
      npc.velocityX *= 0.96f;
    }

    npc.bodyTiltVelocity +=
        (npcRecoverTargetTilt - npc.bodyTilt) * recoverPower;
  }

  //================================
  // 速度更新
  //================================
  npc.velocityX *= inertia_;
  npc.velocityX = std::clamp(npc.velocityX, -1.2f * maxSpeed, 1.2f * maxSpeed);

  npc.moveX += npc.velocityX;

  npc.velocityY -= gravity_;
  npc.moveY += npc.velocityY;

  //================================
  // 着地更新
  //================================
  if (npc.moveY <= npcGroundY) {
    npc.moveY = npcGroundY;
    npc.velocityY = 0.0f;
    npc.isGrounded = true;
  } else {
    npc.isGrounded = false;
  }

  bool justLanded = (!wasGrounded && npc.isGrounded);

  if (justLanded) {
    npc.landTimer = 0.0f;
    npc.leftLegBendSpeed = 0.0f;
    npc.rightLegBendSpeed = 0.0f;
  } else if (npc.isGrounded) {
    npc.landTimer += system_->GetDeltaTime();
  } else {
    npc.landTimer = 999.0f;
  }
}

void TravelScene::UpdateNpcRunners(float deltaTime) {
  for (auto &npc : npcRunners_) {

    if (!npc.started) {
      npc.startDelay -= deltaTime;
      if (npc.startDelay <= 0.0f) {
        npc.started = true;
      }
      continue;
    }

    if (npc.finished) {
      continue;
    }

    UpdateNpcInput(npc, deltaTime);
    UpdateNpcMovement(npc);

    if (npc.moveX >= goalX_) {
      npc.finished = true;
    }

    if (npc.useCustomizedVisual) {
      if (!npc.visualInitialized) {
        BuildNpcCustomizedVisual(npc);
        npc.visualInitialized = true;
      }
      UpdateNpcCustomizedVisual(npc);
    } else if (npc.debugObject) {
      npc.debugObject->mainPosition.transform.translate = {npc.laneX, npc.moveY,
                                                           npc.moveX};
      npc.debugObject->Update(usingCamera_);
    }
  }
}

void TravelScene::UpdateRaceRanking() {
  std::vector<RaceEntry> entries;
  entries.reserve(npcRunners_.size() + 1);

  // プレイヤー
  {
    RaceEntry entry;
    entry.isPlayer = true;
    entry.npcIndex = -1;
    entry.progress = moveX_;
    entries.push_back(entry);
  }

  // NPC
  for (size_t i = 0; i < npcRunners_.size(); ++i) {
    RaceEntry entry;
    entry.isPlayer = false;
    entry.npcIndex = static_cast<int>(i);
    entry.progress = npcRunners_[i].moveX;
    entries.push_back(entry);
  }

  std::sort(entries.begin(), entries.end(),
            [](const RaceEntry &a, const RaceEntry &b) {
              return a.progress > b.progress;
            });

  playerRank_ = 1;

  for (size_t i = 0; i < entries.size(); ++i) {
    if (entries[i].isPlayer) {
      playerRank_ = static_cast<int>(i) + 1;
      isPlayerQualified_ = (playerRank_ <= qualifyCount_);
      break;
    }
  }

  goalCount_ = 0;

  if (moveX_ >= goalX_) {
    goalCount_++;
  }

  for (const auto &npc : npcRunners_) {
    if (npc.moveX >= goalX_) {
      goalCount_++;
    }
  }
}

void TravelScene::UpdateRaceFinishState() {
  if (isRaceFinished_) {
    return;
  }

  //==============================
  // プレイヤーのゴール判定
  //==============================
  if (!isPlayerFinished_ && moveX_ >= goalX_) {
    isPlayerFinished_ = true;
    finishCount_++;
    playerFinishRank_ = finishCount_;

    if (playerFinishRank_ <= qualifyCount_) {
      raceResultState_ = RaceResultState::Clear;
    } else {
      raceResultState_ = RaceResultState::GameOver;
    }

    isRaceFinished_ = true;
    return;
  }

  //==============================
  // NPCのゴール判定
  //==============================
  for (auto &npc : npcRunners_) {
    if (npc.finished && npc.finishRank < 0) {
      finishCount_++;
      npc.finishRank = finishCount_;
    }
  }

  //==============================
  // プレイヤー到着前に枠が埋まったらゲームオーバー
  //==============================
  if (!isPlayerFinished_ && finishCount_ >= qualifyCount_) {
    raceResultState_ = RaceResultState::GameOver;
    isRaceFinished_ = true;
    return;
  }
}

std::unique_ptr<ModBodyCustomizeData> TravelScene::CreateNpcPresetDefault() {
  std::unique_ptr<ModBodyCustomizeData> data =
      ModBody::CreateDefaultCustomizeData();

  if (data == nullptr) {
    return nullptr;
  }

  Logger::Log("NPC PRESET DEFAULT partInstances=%d snapshots=%d",
              (int)data->partInstances.size(),
              (int)data->controlPointSnapshots.size());

  for (const auto &inst : data->partInstances) {
    Logger::Log("NPC PRESET partType=%d partId=%d parentId=%d",
                (int)inst.partType, inst.partId, inst.parentId);
  }

  return data;
}

std::unique_ptr<ModBodyCustomizeData> TravelScene::CreateNpcPresetHeadBig() {
  std::unique_ptr<ModBodyCustomizeData> data = CreateNpcPresetDefault();

  if (data == nullptr) {
    return nullptr;
  }

  auto &headParam = data->partParams[ToIndex(ModBodyPart::Head)];
  headParam.scale.x *= 2.0f;
  headParam.scale.y *= 2.0f;
  headParam.scale.z *= 2.0f;

  auto &neckParam = data->partParams[ToIndex(ModBodyPart::Neck)];
  neckParam.scale.x *= 1.15f;
  neckParam.scale.z *= 1.15f;

  return data;
}

std::unique_ptr<ModBodyCustomizeData> TravelScene::CreateNpcPresetLongLeg() {
  std::unique_ptr<ModBodyCustomizeData> data = CreateNpcPresetDefault();

  if (data == nullptr) {
    return nullptr;
  }

  // 太もも
  auto &leftThigh = data->partParams[ToIndex(ModBodyPart::LeftThigh)];
  auto &rightThigh = data->partParams[ToIndex(ModBodyPart::RightThigh)];

  leftThigh.scale.y *= 2.0f;
  rightThigh.scale.y *= 2.0f;

  // 脛
  auto &leftShin = data->partParams[ToIndex(ModBodyPart::LeftShin)];
  auto &rightShin = data->partParams[ToIndex(ModBodyPart::RightShin)];

  leftShin.scale.y *= 2.0f;
  rightShin.scale.y *= 2.0f;

  return data;
}

std::unique_ptr<ModBodyCustomizeData> TravelScene::CreateNpcPresetBigTorso() {
  std::unique_ptr<ModBodyCustomizeData> data = CreateNpcPresetDefault();

  if (data == nullptr) {
    return nullptr;
  }

  auto &chest = data->partParams[ToIndex(ModBodyPart::ChestBody)];
  auto &stomach = data->partParams[ToIndex(ModBodyPart::StomachBody)];

  chest.scale.x *= 2.0f;
  chest.scale.y *= 2.0f;
  chest.scale.z *= 2.0f;

  stomach.scale.x *= 2.0f;
  stomach.scale.y *= 2.0f;
  stomach.scale.z *= 2.0f;

  return data;
}

void TravelScene::SetupNpcPartObject(
    std::array<Object *, static_cast<size_t>(ModBodyPart::Count)> &objects,
    std::array<ModBody, static_cast<size_t>(ModBodyPart::Count)> &bodies,
    ModBodyPart part, const std::string &path) {
  const size_t index = ToIndex(part);

  int modelHandle = system_->SetModelObj(path);

  objects[index] = new Object;
  objects[index]->IntObject(system_);
  objects[index]->CreateModelData(modelHandle);
  objects[index]->mainPosition.transform = CreateDefaultTransform();

  bodies[index].Initialize(objects[index], part);
}

void TravelScene::SetupNpcCustomizedVisual(NpcRunner &npc) {
  npc.modObjects.fill(nullptr);

  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::ChestBody,
                     "GAME/resources/modBody/chest/chest.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::StomachBody,
                     "GAME/resources/modBody/stomach/stomach.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::Neck,
                     "GAME/resources/modBody/neck/neck.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::Head,
                     "GAME/resources/modBody/head/head.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::LeftUpperArm,
                     "GAME/resources/modBody/leftUpperArm/leftUpperArm.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::LeftForeArm,
                     "GAME/resources/modBody/leftForeArm/leftForeArm.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::RightUpperArm,
                     "GAME/resources/modBody/rightUpperArm/rightUpperArm.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::RightForeArm,
                     "GAME/resources/modBody/rightForeArm/rightForeArm.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::LeftThigh,
                     "GAME/resources/modBody/leftThighs/leftThighs.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::LeftShin,
                     "GAME/resources/modBody/leftShin/leftShin.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::RightThigh,
                     "GAME/resources/modBody/rightThighs/rightThighs.obj");
  SetupNpcPartObject(npc.modObjects, npc.modBodies, ModBodyPart::RightShin,
                     "GAME/resources/modBody/rightShin/rightShin.obj");

  if (npc.customizeData == nullptr) {
    return;
  }

  for (size_t i = 0; i < npc.modBodies.size(); ++i) {
    npc.modBodies[i].SetParam(npc.customizeData->partParams[i]);
  }

  npc.useCustomizedVisual = true;

  Object *chestBody = npc.modObjects[ToIndex(ModBodyPart::ChestBody)];
  Object *stomachBody = npc.modObjects[ToIndex(ModBodyPart::StomachBody)];
  Object *leftUpperArm = npc.modObjects[ToIndex(ModBodyPart::LeftUpperArm)];
  Object *rightUpperArm = npc.modObjects[ToIndex(ModBodyPart::RightUpperArm)];
  Object *leftThigh = npc.modObjects[ToIndex(ModBodyPart::LeftThigh)];
  Object *rightThigh = npc.modObjects[ToIndex(ModBodyPart::RightThigh)];

  if (chestBody == nullptr || stomachBody == nullptr ||
      leftUpperArm == nullptr || rightUpperArm == nullptr ||
      leftThigh == nullptr || rightThigh == nullptr) {
    return;
  }

  ObjectPart *chestRoot = &chestBody->mainPosition;
  ObjectPart *stomachRoot = &stomachBody->mainPosition;
  ObjectPart *leftUpperArmRoot = &leftUpperArm->mainPosition;
  ObjectPart *rightUpperArmRoot = &rightUpperArm->mainPosition;
  ObjectPart *leftThighRoot = &leftThigh->mainPosition;
  ObjectPart *rightThighRoot = &rightThigh->mainPosition;

  stomachBody->followObject_ = chestRoot;
  stomachBody->mainPosition.parentPart = chestRoot;

  npc.modObjects[ToIndex(ModBodyPart::Neck)]->followObject_ = chestRoot;
  npc.modObjects[ToIndex(ModBodyPart::Neck)]->mainPosition.parentPart =
      chestRoot;

  npc.modObjects[ToIndex(ModBodyPart::Head)]->followObject_ =
      &npc.modObjects[ToIndex(ModBodyPart::Neck)]->mainPosition;
  npc.modObjects[ToIndex(ModBodyPart::Head)]->mainPosition.parentPart =
      &npc.modObjects[ToIndex(ModBodyPart::Neck)]->mainPosition;

  npc.modObjects[ToIndex(ModBodyPart::LeftUpperArm)]->followObject_ = chestRoot;
  npc.modObjects[ToIndex(ModBodyPart::LeftUpperArm)]->mainPosition.parentPart =
      chestRoot;

  npc.modObjects[ToIndex(ModBodyPart::RightUpperArm)]->followObject_ =
      chestRoot;
  npc.modObjects[ToIndex(ModBodyPart::RightUpperArm)]->mainPosition.parentPart =
      chestRoot;

  npc.modObjects[ToIndex(ModBodyPart::LeftForeArm)]->followObject_ = chestRoot;
  npc.modObjects[ToIndex(ModBodyPart::LeftForeArm)]->mainPosition.parentPart =
      chestRoot;

  npc.modObjects[ToIndex(ModBodyPart::RightForeArm)]->followObject_ = chestRoot;
  npc.modObjects[ToIndex(ModBodyPart::RightForeArm)]->mainPosition.parentPart =
      chestRoot;

  npc.modObjects[ToIndex(ModBodyPart::LeftThigh)]->followObject_ = stomachRoot;
  npc.modObjects[ToIndex(ModBodyPart::LeftThigh)]->mainPosition.parentPart =
      stomachRoot;

  npc.modObjects[ToIndex(ModBodyPart::RightThigh)]->followObject_ = stomachRoot;
  npc.modObjects[ToIndex(ModBodyPart::RightThigh)]->mainPosition.parentPart =
      stomachRoot;

  npc.modObjects[ToIndex(ModBodyPart::LeftShin)]->followObject_ = stomachRoot;
  npc.modObjects[ToIndex(ModBodyPart::LeftShin)]->mainPosition.parentPart =
      stomachRoot;

  npc.modObjects[ToIndex(ModBodyPart::RightShin)]->followObject_ = stomachRoot;
  npc.modObjects[ToIndex(ModBodyPart::RightShin)]->mainPosition.parentPart =
      stomachRoot;
}

void TravelScene::DrawNpcCustomizedVisual(NpcRunner &npc) {
  for (auto *obj : npc.modObjects) {
    if (obj != nullptr) {
      obj->Draw();
    }
  }

  for (auto *obj : npc.extraObjects) {
    if (obj != nullptr) {
      obj->Draw();
    }
  }
}

void TravelScene::ClearNpcCustomizedVisual(NpcRunner &npc) {}

void TravelScene::BuildNpcCustomizedVisual(NpcRunner &npc) {
  for (size_t i = 0; i < npc.modObjects.size(); ++i) {
    if (npc.modObjects[i] != nullptr) {
      npc.modBodies[i].Apply(npc.modObjects[i]);
    }
  }
}

void TravelScene::UpdateNpcCustomizedVisual(NpcRunner &npc) {
  Object *chestBody = npc.modObjects[ToIndex(ModBodyPart::ChestBody)];
  Object *stomachBody = npc.modObjects[ToIndex(ModBodyPart::StomachBody)];
  Object *neck = npc.modObjects[ToIndex(ModBodyPart::Neck)];
  Object *head = npc.modObjects[ToIndex(ModBodyPart::Head)];

  Object *leftUpperArm = npc.modObjects[ToIndex(ModBodyPart::LeftUpperArm)];
  Object *leftForeArm = npc.modObjects[ToIndex(ModBodyPart::LeftForeArm)];
  Object *rightUpperArm = npc.modObjects[ToIndex(ModBodyPart::RightUpperArm)];
  Object *rightForeArm = npc.modObjects[ToIndex(ModBodyPart::RightForeArm)];

  Object *leftThigh = npc.modObjects[ToIndex(ModBodyPart::LeftThigh)];
  Object *leftShin = npc.modObjects[ToIndex(ModBodyPart::LeftShin)];
  Object *rightThigh = npc.modObjects[ToIndex(ModBodyPart::RightThigh)];
  Object *rightShin = npc.modObjects[ToIndex(ModBodyPart::RightShin)];

  if (chestBody == nullptr || stomachBody == nullptr || neck == nullptr ||
      head == nullptr || leftUpperArm == nullptr || leftForeArm == nullptr ||
      rightUpperArm == nullptr || rightForeArm == nullptr ||
      leftThigh == nullptr || leftShin == nullptr || rightThigh == nullptr ||
      rightShin == nullptr) {
    return;
  }

  auto SafePartLength = [](Object *obj) -> float {
    if (obj == nullptr || obj->objectParts_.empty()) {
      return 1.0f;
    }
    return (std::max)(0.05f, obj->objectParts_[0].transform.scale.y);
  };

  auto BuildAnimatedChildRoot = [](const Vector3 &root, float angleX,
                                   float length) -> Vector3 {
    return {root.x, root.y - std::cos(angleX) * length,
            root.z - std::sin(angleX) * length};
  };

  const float chestLength = SafePartLength(chestBody);
  const float stomachLength = SafePartLength(stomachBody);
  const float neckLength = SafePartLength(neck);

  const float leftUpperArmLength = SafePartLength(leftUpperArm);
  const float leftForeArmLength = SafePartLength(leftForeArm);
  const float rightUpperArmLength = SafePartLength(rightUpperArm);
  const float rightForeArmLength = SafePartLength(rightForeArm);

  const float leftThighLength = SafePartLength(leftThigh);
  const float leftShinLength = SafePartLength(leftShin);
  const float rightThighLength = SafePartLength(rightThigh);
  const float rightShinLength = SafePartLength(rightShin);

  //==============================
  // bend の中間を基準に使う
  // これをしないと「初期姿勢だけ強い」になりやすい
  //==============================
  const float legAnimCenter = (legKickAngle_ + legRecoverAngle_) * 0.5f;

  float leftLegAnim = npc.leftLegBend - legAnimCenter;
  float rightLegAnim = npc.rightLegBend - legAnimCenter;

  const float armSwingScale = 1.4f;
  const float thighSwingScale = 1.4;

  const float bodyTiltArmAssist = npc.bodyTilt * 0.35f;

  const float leftUpperArmRotX =
      -rightLegAnim * armSwingScale + bodyTiltArmAssist;
  const float rightUpperArmRotX =
      -leftLegAnim * armSwingScale + bodyTiltArmAssist;

  const float leftThighRotX = -leftLegAnim * thighSwingScale;
  const float rightThighRotX = -rightLegAnim * thighSwingScale;

  //==============================
  // 胴体スケール取得
  //==============================
  float chestWidthScale = 1.0f;
  float chestHeightScale = 1.0f;
  float stomachWidthScale = 1.0f;
  float stomachHeightScale = 1.0f;
  float neckHeightScale = 1.0f;

  if (npc.customizeData != nullptr) {
    const auto &chestParam =
        npc.customizeData->partParams[ToIndex(ModBodyPart::ChestBody)];
    const auto &stomachParam =
        npc.customizeData->partParams[ToIndex(ModBodyPart::StomachBody)];
    const auto &neckParam =
        npc.customizeData->partParams[ToIndex(ModBodyPart::Neck)];

    chestWidthScale = chestParam.scale.x;
    chestHeightScale = chestParam.scale.y;
    stomachWidthScale = stomachParam.scale.x;
    stomachHeightScale = stomachParam.scale.y;
    neckHeightScale = neckParam.scale.y;
  }

  //==============================
  // 胸
  //==============================
  chestBody->mainPosition.transform.translate.x = npc.laneX;
  chestBody->mainPosition.transform.translate.y = npc.moveY;
  chestBody->mainPosition.transform.translate.z = npc.moveX;
  chestBody->mainPosition.transform.rotate = {-npc.bodyTilt, 0.0f, 0.0f};

  //==============================
  // 腹
  //==============================
  stomachBody->mainPosition.transform.translate = {0.0f, -chestLength * 0.78f,
                                                   0.0f};
  stomachBody->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  //==============================
  // 首・頭
  //==============================
  const Vector3 neckBaseLocal = {0.0f, chestLength * 0.72f, 0.0f};

  neck->mainPosition.transform.translate = neckBaseLocal;
  neck->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  head->mainPosition.transform.translate = {
      0.0f, neckLength * 0.62f * neckHeightScale, 0.0f};
  head->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};

  //==============================
  // 肩位置
  //==============================
  const Vector3 leftShoulderLocal = {
      -0.52f * chestWidthScale, chestLength * 0.32f * chestHeightScale, 0.0f};

  const Vector3 rightShoulderLocal = {
      0.52f * chestWidthScale, chestLength * 0.32f * chestHeightScale, 0.0f};

  //==============================
  // 股関節位置
  //==============================
  const Vector3 leftHipLocal = {-0.30f * stomachWidthScale,
                                -stomachLength * 0.28f * stomachHeightScale,
                                0.0f};

  const Vector3 rightHipLocal = {0.30f * stomachWidthScale,
                                 -stomachLength * 0.28f * stomachHeightScale,
                                 0.0f};

  //==============================
  // 擬似CP
  // 肩→肘、股関節→膝を毎フレーム作る
  //==============================
  Vector3 leftElbowLocal = BuildAnimatedChildRoot(
      leftShoulderLocal, leftUpperArmRotX, leftUpperArmLength);

  Vector3 rightElbowLocal = BuildAnimatedChildRoot(
      rightShoulderLocal, rightUpperArmRotX, rightUpperArmLength);

  Vector3 leftKneeLocal =
      BuildAnimatedChildRoot(leftHipLocal, leftThighRotX, leftThighLength);

  Vector3 rightKneeLocal =
      BuildAnimatedChildRoot(rightHipLocal, rightThighRotX, rightThighLength);

  //==============================
  // 上腕
  //==============================
  leftUpperArm->mainPosition.transform.translate = leftShoulderLocal;
  leftUpperArm->mainPosition.transform.rotate = {leftUpperArmRotX, 0.0f, 0.0f};

  rightUpperArm->mainPosition.transform.translate = rightShoulderLocal;
  rightUpperArm->mainPosition.transform.rotate = {rightUpperArmRotX, 0.0f,
                                                  0.0f};

  if (!leftUpperArm->objectParts_.empty()) {
    leftUpperArm->objectParts_[0].transform.translate = {
        0.0f, -leftUpperArmLength * 0.5f, 0.0f};
  }

  if (!rightUpperArm->objectParts_.empty()) {
    rightUpperArm->objectParts_[0].transform.translate = {
        0.0f, -rightUpperArmLength * 0.5f, 0.0f};
  }

  //==============================
  // 前腕
  //==============================
  leftForeArm->mainPosition.transform.translate = leftElbowLocal;
  leftForeArm->mainPosition.transform.rotate = {leftUpperArmRotX * 0.45f, 0.0f,
                                                0.0f};

  rightForeArm->mainPosition.transform.translate = rightElbowLocal;
  rightForeArm->mainPosition.transform.rotate = {rightUpperArmRotX * 0.45f,
                                                 0.0f, 0.0f};

  if (!leftForeArm->objectParts_.empty()) {
    leftForeArm->objectParts_[0].transform.translate = {
        0.0f, -leftForeArmLength * 0.5f, 0.0f};
  }

  if (!rightForeArm->objectParts_.empty()) {
    rightForeArm->objectParts_[0].transform.translate = {
        0.0f, -rightForeArmLength * 0.5f, 0.0f};
  }

  //==============================
  // 腿
  //==============================
  leftThigh->mainPosition.transform.translate = leftHipLocal;
  leftThigh->mainPosition.transform.rotate = {leftThighRotX, 0.0f, 0.0f};

  rightThigh->mainPosition.transform.translate = rightHipLocal;
  rightThigh->mainPosition.transform.rotate = {rightThighRotX, 0.0f, 0.0f};

  if (!leftThigh->objectParts_.empty()) {
    leftThigh->objectParts_[0].transform.translate = {
        0.0f, -leftThighLength * 0.5f, 0.0f};
  }

  if (!rightThigh->objectParts_.empty()) {
    rightThigh->objectParts_[0].transform.translate = {
        0.0f, -rightThighLength * 0.5f, 0.0f};
  }

  //==============================
  // 脛
  //==============================
  leftShin->mainPosition.transform.translate = leftKneeLocal;
  leftShin->mainPosition.transform.rotate = {leftThighRotX * 0.55f, 0.0f, 0.0f};

  rightShin->mainPosition.transform.translate = rightKneeLocal;
  rightShin->mainPosition.transform.rotate = {rightThighRotX * 0.55f, 0.0f,
                                              0.0f};

  if (!leftShin->objectParts_.empty()) {
    leftShin->objectParts_[0].transform.translate = {
        0.0f, -leftShinLength * 0.5f, 0.0f};
  }

  if (!rightShin->objectParts_.empty()) {
    rightShin->objectParts_[0].transform.translate = {
        0.0f, -rightShinLength * 0.5f, 0.0f};
  }

  //==============================
  // 反映
  //==============================
  for (size_t i = 0; i < npc.modObjects.size(); ++i) {
    if (npc.modObjects[i] != nullptr) {
      npc.modObjects[i]->Update(usingCamera_);
    }
  }
}