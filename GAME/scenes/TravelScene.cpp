#include "GAME/actor/TravelPlayer.h"
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
  player_ = std::make_unique<TravelPlayer>(system);
  npcManager_ = std::make_unique<TravelNpcManager>(system);
  player_->Initialize(-18.0f);
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
  player_->SetupModObjects();

  customizeData_ = ModBody::CopySharedCustomizeData();
  if (customizeData_ == nullptr) {
    customizeData_ = ModBody::CreateDefaultCustomizeData();
  }

  player_->SetCustomizeData(customizeData_.get());
  player_->LoadCustomizeData();

  player_->BuildFeaturesFromCustomizeData();
  BuildExtraVisualParts();

  player_->ApplyCustomizeToMovementParam();

  // UpdateChildRootsFromBody();

  // player_->leftLegBend_ = player_->legRecoverAngle_;
  // player_->rightLegBend_ = player_->legRecoverAngle_;
  player_->leftLegBend_ = 0.0f;
  player_->rightLegBend_ = 0.0f;

  player_->leftLegPrevBend_ = player_->leftLegBend_;
  player_->rightLegPrevBend_ = player_->rightLegBend_;

  player_->leftLegBendSpeed_ = 0.0f;
  player_->rightLegBendSpeed_ = 0.0f;
  player_->leftLegPrevBendSpeed_ = 0.0f;
  player_->rightLegPrevBendSpeed_ = 0.0f;

  player_->bodyTilt_ = 0.0f;
  player_->bodyTiltVelocity_ = 0.0f;

  player_->leftDriveAccum_ = 0.0f;
  player_->rightDriveAccum_ = 0.0f;
  player_->leftHoldTime_ = 0.0f;
  player_->rightHoldTime_ = 0.0f;
  player_->lastKickSide_ = 0;

  player_->moveX_ = -18.0f;

  player_->gaitTiltTarget_ = 0.0f;
  player_->landTimer_ = 999.0f;

  // bodyで代用中
  groundModelHandle_ =
      system_->SetModelObj("GAME/resources/TravelScene/BGObject.obj");
  // groundModelHandle_ =
  //     system_->SetModelObj("GAME/resources/modBody/body/body.obj");

  grounds_.clear();

  const int groundCount = 5;   // 個数
  const float spacing = 80.0f; // 間隔

  for (int i = 0; i < groundCount; ++i) {
    auto ground = std::make_unique<Object>();
    ground->IntObject(system_);
    ground->CreateModelData(groundModelHandle_);
    ground->mainPosition.transform = CreateDefaultTransform();

    ground->mainPosition.transform.translate = {
        0.0f, 0.0f, static_cast<float>(i) * spacing};

    ground->mainPosition.transform.rotate = {0.0f, -1.57f, 0.0f};
    ground->mainPosition.transform.scale = {8.0f, 8.0f, 8.0f};

    grounds_.push_back(std::move(ground));
  }

  // 影
  shadowModelHandle_ =
      system_->SetModelObj("GAME/resources/object/Plane/plane.gltf");

  player_->shadow_ = std::make_unique<Object>();
  player_->shadow_->IntObject(system_);
  player_->shadow_->CreateModelData(shadowModelHandle_);
  player_->shadow_->mainPosition.transform = CreateDefaultTransform();

  player_->shadow_->mainPosition.transform.translate = {0.0f, player_->groundY_ + 0.01f, player_->moveX_};
  player_->shadow_->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  player_->shadow_->mainPosition.transform.scale = {1.2f, 0.02f, 1.2f};

  player_->shadow_->objectParts_[0].materialConfig->textureColor = {0.0f, 0.0f, 0.0f,
                                                           0.9f};

  // ゴール
  goalModelHandle_ =
      system_->SetModelObj("GAME/resources/TravelScene/Goal.obj");

  goalObject_ = std::make_unique<Object>();
  goalObject_->IntObject(system_);
  goalObject_->CreateModelData(goalModelHandle_);
  goalObject_->mainPosition.transform = CreateDefaultTransform();

  goalObject_->mainPosition.transform.translate = {0.0f, 0.0f, goalX_};
  goalObject_->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  goalObject_->mainPosition.transform.scale = {2.0f, 2.0f, 2.0f};

  //===============================
  // 2D
  //===============================
  fade_.Initialize(system_);
  fade_.StartFadeIn();

  spriteAHandle_ = TextureManager::GetInstance()->LoadModelTexture(
      "GAME/resources/TravelScene/UI_A.png");
  spriteDHandle_ = TextureManager::GetInstance()->LoadModelTexture(
      "GAME/resources/TravelScene/UI_D.png");

  spriteA_ = std::make_unique<SimpleSprite>();
  spriteA_->IntObject(system_);
  spriteA_->CreateDefaultData();
  spriteA_->objectParts_[0].materialConfig->textureHandle = spriteAHandle_;
  spriteA_->mainPosition.transform.translate = {10.0f, 600.0f, 0.0f};
  spriteA_->mainPosition.transform.scale = {0.6f, 0.6f, 1.0f};

  spriteD_ = std::make_unique<SimpleSprite>();
  spriteD_->IntObject(system_);
  spriteD_->CreateDefaultData();
  spriteD_->objectParts_[0].materialConfig->textureHandle = spriteDHandle_;
  spriteD_->mainPosition.transform.translate = {100.0f, 600.0f, 0.0f};
  spriteD_->mainPosition.transform.scale = {0.6f, 0.6f, 1.0f};

  startUITextTimer_ = 4.0f; // 表示時間

  //===============================
  // NPC
  //===============================
  npcManager_->npcModelHandle_ =
      system_->SetModelObj("GAME/resources/modBody/body/body.obj");
  npcManager_->InitializeNpcRunners(customizeData_.get(), player_.get(), goalX_);

  //===============================
  // パーティクル
  //===============================


  for (auto &obj : npcDebugCpObjects_) {
    obj = new Object;
    obj->IntObject(system_);
    obj->CreateModelData(modModelHandles_[ToIndex(ModBodyPart::Head)]);
    obj->mainPosition.transform = CreateDefaultTransform();
    obj->mainPosition.transform.scale = {0.08f, 0.08f, 0.08f};
  }

  pendingFailureOutcome_ = SceneOutcome::NONE;
  isFailureMenuOpen_ = false;
  failureMenuInputCooldown_ = 0.0f;
  selectedRetryChoiceTravel_ = RetryChoiceTravel::RetryTravel;
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

  for (auto &npc : npcManager_->npcRunners_) {
    npcManager_->ClearNpcCustomizedVisual(npc);
  }

  player_->ClearParticle();

  ClearExtraVisualParts();

  system_->RemoveLight(light1_);

  delete light1_;

  bitmapFont.Cleanup();

  ResourceManager::GetInstance()->CleanupUnusedMaterials();

  for (auto &obj : npcDebugCpObjects_) {
    delete obj;
    obj = nullptr;
  }
}

void TravelScene::Update() {
  //===============================
  // カメラ更新
  //===============================
  CameraPart();

  //===============================
  // フェード中はレース進行を止める
  //===============================
  if (isStartTransition_) {
    UpdateSceneTransition();
    return;
  }

  //===============================
  // 失敗時のリトライ更新
  //===============================
  if (isFailureMenuOpen_) {
    UpdateFailureMenuInputTravel();
    player_->UpdateParticle(camera_);
    fade_.Update(usingCamera_);
    return;
  }

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

  player_->SavePreviousFrameState();

  UpdateTimeLimit(deltaTime);

  if (!isRaceFinished_) {
    player_->UpdateHoldState(leftNowInput, rightNowInput, deltaTime);
    player_->UpdateLegBendState(leftNowInput, rightNowInput);
    player_->UpdateMovementState(leftNowInput, rightNowInput);

    UpdateRaceRanking();
    UpdateRaceFinishState();
  }
  npcManager_->UpdateNpcRunners(deltaTime, goalX_, usingCamera_);

  if (kickFeedbackTimer_ > 0.0f) {
    kickFeedbackTimer_ -= deltaTime;
    if (kickFeedbackTimer_ <= 0.0f) {
      kickFeedbackTimer_ = 0.0f;
      kickFeedbackType_ = KickFeedbackType::None;
    }
  }

  //-------------------------------
  // キーUI点灯タイマー更新
  //-------------------------------
  if (aKeyFlashTimer_ > 0.0f) {
    aKeyFlashTimer_ -= deltaTime;
    if (aKeyFlashTimer_ < 0.0f) {
      aKeyFlashTimer_ = 0.0f;
    }
  }

  if (dKeyFlashTimer_ > 0.0f) {
    dKeyFlashTimer_ -= deltaTime;
    if (dKeyFlashTimer_ < 0.0f) {
      dKeyFlashTimer_ = 0.0f;
    }
  }

  player_->ApplyVisualState();

  //-------------------------------
  // キーUIの透明度反映
  //-------------------------------
  spriteA_->objectParts_[0].materialConfig->textureColor.w =
      leftNowInput ? 0.4f : 1.0f;

  spriteD_->objectParts_[0].materialConfig->textureColor.w =
      rightNowInput ? 0.4f : 1.0f;

  player_->UpdateParticle(camera_);

  UpdateSceneTransition();

  if (startUITextTimer_ > 0.0f) {
    startUITextTimer_ -= system_->GetDeltaTime();
    if (startUITextTimer_ < 0.0f) {
      startUITextTimer_ = 0.0f;
    }
  }
}

void TravelScene::Draw() {

  player_->DrawModObjects(usingCamera_);

  DrawExtraVisualParts();

  for (auto &npc : npcManager_->npcRunners_) {
    if (!npc.started) {
      continue;
    }

    if (!showNpcModel_) {
      continue;
    }

    if (npc.finished && npc.moveX > goalX_ + 20.0f) {
      continue;
    }

    if (npc.useCustomizedVisual) {
      npcManager_->DrawNpcCustomizedVisual(npc);
    } else if (npc.debugObject != nullptr) {
      npc.debugObject->Draw();
    }
  }

  for (auto &ground : grounds_) {
    ground->Draw();
  }

  if (player_->shadow_ != nullptr) {
    player_->shadow_->Draw();
  }

  if (goalObject_ != nullptr) {
    goalObject_->Draw();
  }

  // for (auto *obj : npcDebugCpObjects_) {
  //   if (obj != nullptr) {
  //     obj->Draw();
  //   }
  // }

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
  ImGui::Text("MoveX : %.3f", player_->moveX_);
  ImGui::Text("MoveY : %.3f", player_->moveY_);
  ImGui::Text("VelocityX : %.3f", player_->velocityX_);
  ImGui::Text("VelocityY : %.3f", player_->velocityY_);

  //==============================
  // 姿勢確認
  //==============================
  float legDiffTilt = (player_->leftLegBend_ - player_->rightLegBend_) * player_->legDiffTiltPower_;

  float postureError = std::abs(player_->bodyTilt_ - player_->idealRunTilt_);
  float badPosture = std::clamp(postureError / player_->postureTolerance_, 0.0f, 1.0f);
  float forwardRate = 1.0f - badPosture;
  float upwardRate = 0.30f + badPosture * 0.70f;

  ImGui::Separator();
  ImGui::Text("BodyTilt : %.4f", player_->bodyTilt_);
  ImGui::Text("LegDiffTilt : %.4f", legDiffTilt);

  ImGui::Text("LeftLegBend : %.4f", player_->leftLegBend_);
  ImGui::Text("RightLegBend : %.4f", player_->rightLegBend_);

  ImGui::Text("ForwardRate : %.4f", forwardRate);
  ImGui::Text("UpwardRate : %.4f", upwardRate);

  ImGui::Text("LeftDriveAccum : %.4f", player_->leftDriveAccum_);
  ImGui::Text("RightDriveAccum : %.4f", player_->rightDriveAccum_);

  ImGui::Text("LeftHoldTime : %.4f", player_->leftHoldTime_);
  ImGui::Text("RightHoldTime : %.4f", player_->rightHoldTime_);

  ImGui::Text("BodyTiltVelocity : %.4f", player_->bodyTiltVelocity_);
  ImGui::Text("LeftLegBendSpeed : %.4f", player_->leftLegBendSpeed_);
  ImGui::Text("RightLegBendSpeed : %.4f", player_->rightLegBendSpeed_);

  ImGui::Checkbox("Force Tilt", &player_->debugForceTilt_);
  ImGui::SliderFloat("Tilt Value", &player_->debugTiltValue_, -0.4f, 0.4f);

  ImGui::Checkbox("Use Customize Move", &player_->useCustomizeMove_);
  ImGui::Text("runPower: %.2f", player_->tuning_.runPower);
  ImGui::Text("lift: %.2f", player_->tuning_.lift);
  ImGui::Text("maxSpeed: %.2f", player_->tuning_.maxSpeed);
  ImGui::Text("stability: %.2f", player_->tuning_.stability);
  ImGui::Text("bodyTilt: %.4f", player_->bodyTilt_);
  ImGui::Text("turnResponse: %.2f", player_->tuning_.turnResponse);

  {
    TravelPlayer::LowestBodyPart lowestPart = TravelPlayer::LowestBodyPart::None;
    float lowestBodyLocalY = player_->GetLowestVisualBodyY(&lowestPart);
    float lowestBodyWorldY = player_->moveY_ + player_->visualLiftY_ + lowestBodyLocalY;
    float penetration = player_->groundY_ - lowestBodyWorldY;

    ImGui::Text("--- Ground Penetration ---");
    ImGui::Text("Player Y: %.3f", player_->moveY_);
    ImGui::Text("Lift Y: %.3f", player_->visualLiftY_);
    ImGui::Text("Lowest Part: %s (%.3f)", player_->GetLowestBodyPartName(lowestPart),
                lowestBodyLocalY);
  }

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
    TravelPlayer::LowestBodyPart lowestPart = TravelPlayer::LowestBodyPart::None;
    float lowestBodyLocalY = player_->GetLowestVisualBodyY(&lowestPart);
    float lowestBodyWorldY = player_->moveY_ + player_->visualLiftY_ + lowestBodyLocalY;
    float penetration = player_->groundY_ - lowestBodyWorldY;

    ImGui::Begin("LowestBodyCheck");
    ImGui::Text("LowestBodyLocalY : %.3f", lowestBodyLocalY);
    ImGui::Text("LowestBodyWorldY : %.3f", lowestBodyWorldY);
    ImGui::Text("LowestPart       : %s", player_->GetLowestBodyPartName(lowestPart));
    ImGui::Text("GroundY          : %.3f", player_->groundY_);
    ImGui::Text("Penetration      : %.3f", penetration);
    ImGui::Text("VisualLiftY      : %.3f", player_->visualLiftY_);
    ImGui::End();
  }
#endif

#ifdef USE_IMGUI
  ImGui::Begin("NpcDebug");

  ImGui::Checkbox("Show NPC Model", &showNpcModel_);

  for (size_t i = 0; i < npcManager_->npcRunners_.size(); ++i) {
    const auto &npc = npcManager_->npcRunners_[i];

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

  bitmapFont.RenderText(rankText, {1100, 600}, 96, BitmapFont::Align::Left,
                        5.0f, {0.0f, 0.0f, 0.0f, 1.0f});

  // 残り枠表示
  std::string goalText = "GOAL " + std::to_string(goalCount_) + "/" +
                         std::to_string(qualifyCount_);

  bitmapFont.RenderText(goalText, {1000, 60}, 48, BitmapFont::Align::Left);

  // if (raceResultState_ == RaceResultState::Clear) {
  //   bitmapFont.RenderText("CLEAR", {900, 300}, 96, BitmapFont::Align::Left);
  // } else
  // if (raceResultState_ == RaceResultState::GameOver) {
  //  bitmapFont.RenderText("GAME OVER", {800, 300}, 96,
  //  BitmapFont::Align::Left);
  //}

  if (startUITextTimer_ > 0.0f) {

    float alpha = 1.0f;
    const float fadeOutTime = 0.5f;

    if (startUITextTimer_ < fadeOutTime) {
      alpha = startUITextTimer_ / fadeOutTime;
    }

    alpha = std::clamp(alpha, 0.0f, 1.0f);

    bitmapFont.RenderText("せんちゃく3にん！ゴールまでいそげ！", {150, 100}, 64,
                          BitmapFont::Align::Left, 5.0f,
                          {1.0f, 1.0f, 1.0f, alpha});
  }

  DrawFailureMenuTravel();

  player_->DrawParticle();

  spriteA_->Draw();
  spriteD_->Draw();

  // フェード描画
  fade_.Draw();
}

void TravelScene::CameraPart() {
  if (useDebugCamera_) {
    usingCamera_ = debugCamera_;
    debugCamera_->MouseControlUpdate();
  } else {
    usingCamera_ = camera_;

    //===============================
    // プレイヤー追従
    //===============================
    Vector3 camPos;

    camPos.x = 48.0f;
    camPos.y = 5.0f;

    if (player_->moveX_ + 10.0f <= goalX_ - 10.0f) {
      camPos.z = player_->moveX_ + 10.0f;
    } else {
      camPos.z = goalX_ - 10.0f;
    }

    camera_->SetTranslate(camPos);

    // 向き（横から見る）
    // camera_->SetRotation({0.0f, -1.57f, 0.0f});
  }

  system_->SetCamera(usingCamera_);
}



/*   指定した部位のObjectを1つ生成する   */


/*   部位同士の親子関係を設定する   */


/*   各部位の初期配置を設定する   */


/*   各部位Objectの更新をまとめて行う   */
/*   各部位Objectの更新をまとめて行う   */
// //
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














bool TravelScene::HasRequiredParts() const {
  return player_->HasRequiredParts();
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

  /*if (customizeData_ != nullptr) {
    customizeData_->timeLimit_ = timeLimit_;
    customizeData_->isTimeUp_ = isTimeUp_;
  }*/
}







void TravelScene::UpdateSceneTransition() {

  //================================
  // レース結果による遷移
  // Clear    -> 次シーン
  // GameOver -> リトライ
  //================================
  // クリアで次シーンへ
  if (isRaceFinished_ && !fade_.IsBusy() && !isStartTransition_) {
    if (raceResultState_ == RaceResultState::Clear) {
      fade_.StartFadeOut();
      isStartTransition_ = true;
      nextOutcome_ = SceneOutcome::NEXT;
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

    if (nextOutcome_ != SceneOutcome::RETRY_MOD) {
      if (customizeData_ != nullptr) {
        ModBody::SetSharedCustomizeData(*customizeData_);
      }
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

bool TravelScene::GetExtraInstanceLocalTranslate(int partId,
                                                 Vector3 &outLocal) const {
  if (customizeData_ == nullptr) {
    return false;
  }

  for (const auto &instance : customizeData_->partInstances) {
    if (instance.partId == partId) {
      outLocal = instance.localTransform.translate;
      return true;
    }
  }

  return false;
}

bool TravelScene::GetFirstPartTypePartId(ModBodyPart partType,
                                         int &outPartId) const {
  outPartId = -1;

  if (customizeData_ == nullptr) {
    return false;
  }

  for (const auto &instance : customizeData_->partInstances) {
    if (instance.partType == partType) {
      outPartId = instance.partId;
      return true;
    }
  }

  return false;
}

float TravelScene::GetSnapshotSegmentLength(ModBodyPart partType,
                                            int ownerPartId) const {
  Vector3 snapRoot = {0.0f, 0.0f, 0.0f};
  Vector3 snapBend = {0.0f, 0.0f, 0.0f};
  Vector3 snapEnd = {0.0f, 0.0f, 0.0f};

  if (!GetExtraPartSnapshotPositions(ownerPartId, snapRoot, snapBend,
                                     snapEnd)) {
    return 0.0f;
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

  return Length({to.x - from.x, to.y - from.y, to.z - from.z});
}

bool TravelScene::GetPartInstanceParentId(int partId, int &outParentId) const {
  outParentId = -1;

  if (customizeData_ == nullptr) {
    return false;
  }

  for (const auto &instance : customizeData_->partInstances) {
    if (instance.partId == partId) {
      outParentId = instance.parentId;
      return true;
    }
  }

  return false;
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

bool TravelScene::GetPartInstanceLocalTranslate(int partId,
                                                Vector3 &outLocal) const {
  if (customizeData_ == nullptr) {
    return false;
  }

  for (const auto &instance : customizeData_->partInstances) {
    if (instance.partId == partId) {
      outLocal = instance.localTransform.translate;
      return true;
    }
  }

  return false;
}

bool TravelScene::GetPartInstanceLocalRotate(int partId,
                                             Vector3 &outRotate) const {
  if (customizeData_ == nullptr) {
    return false;
  }

  for (const auto &instance : customizeData_->partInstances) {
    if (instance.partId != partId) {
      continue;
    }

    outRotate = instance.localTransform.rotate;
    return true;
  }

  return false;
}

bool TravelScene::GetFirstPartTypeLocalTranslate(ModBodyPart partType,
                                                 Vector3 &outLocal) const {
  if (customizeData_ == nullptr) {
    return false;
  }

  for (const auto &instance : customizeData_->partInstances) {
    if (instance.partType == partType) {
      outLocal = instance.localTransform.translate;
      return true;
    }
  }

  return false;
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
    return -player_->rightLegBend_ * armSwingScale;

  case ModBodyPart::RightUpperArm:
    return -player_->leftLegBend_ * armSwingScale;

  case ModBodyPart::LeftForeArm: {
    float upperArmSwing = -player_->rightLegBend_ * armSwingScale;
    float elbowFold = std::clamp((player_->rightLegBend_ - player_->legKickAngle_) /
                                     (player_->legRecoverAngle_ - player_->legKickAngle_),
                                 0.0f, 1.0f);
    return -(upperArmSwing * 0.35f + elbowFold * 0.45f);
  }

  case ModBodyPart::RightForeArm: {
    float upperArmSwing = -player_->leftLegBend_ * armSwingScale;
    float elbowFold = std::clamp((player_->leftLegBend_ - player_->legKickAngle_) /
                                     (player_->legRecoverAngle_ - player_->legKickAngle_),
                                 0.0f, 1.0f);
    return -(upperArmSwing * 0.35f + elbowFold * 0.45f);
  }

  case ModBodyPart::LeftThigh:
    return -player_->leftLegBend_ * thighSwingScale;

  case ModBodyPart::RightThigh:
    return -player_->rightLegBend_ * thighSwingScale;

  case ModBodyPart::LeftShin: {
    float thighSwing = -player_->leftLegBend_ * thighSwingScale;
    float kneeFold = std::clamp((player_->leftLegBend_ - player_->legKickAngle_) /
                                    (player_->legRecoverAngle_ - player_->legKickAngle_),
                                0.0f, 1.0f);
    return thighSwing * 0.35f + kneeFold * 0.6f;
  }

  case ModBodyPart::RightShin: {
    float thighSwing = -player_->rightLegBend_ * thighSwingScale;
    float kneeFold = std::clamp((player_->rightLegBend_ - player_->legKickAngle_) /
                                    (player_->legRecoverAngle_ - player_->legKickAngle_),
                                0.0f, 1.0f);
    return thighSwing * 0.35f + kneeFold * 0.6f;
  }

  default:
    return 0.0f;
  }
}








void TravelScene::UpdateRaceRanking() {
  std::vector<RaceEntry> entries;
  entries.reserve(npcManager_->npcRunners_.size() + 1);

  // プレイヤー
  {
    RaceEntry entry;
    entry.isPlayer = true;
    entry.npcIndex = -1;
    entry.progress = player_->moveX_;
    entries.push_back(entry);
  }

  // NPC
  for (size_t i = 0; i < npcManager_->npcRunners_.size(); ++i) {
    RaceEntry entry;
    entry.isPlayer = false;
    entry.npcIndex = static_cast<int>(i);
    entry.progress = npcManager_->npcRunners_[i].moveX;
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

  if (player_->moveX_ >= goalX_) {
    goalCount_++;
  }

  for (const auto &npc : npcManager_->npcRunners_) {
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
  if (!isPlayerFinished_ && player_->moveX_ >= goalX_) {
    isPlayerFinished_ = true;
    finishCount_++;
    playerFinishRank_ = finishCount_;

    if (playerFinishRank_ <= qualifyCount_) {
      raceResultState_ = RaceResultState::Clear;
      isRaceFinished_ = true;
    } else {
      raceResultState_ = RaceResultState::GameOver;
      isRaceFinished_ = true;
      OpenFailureMenuTravel();
    }
    return;
  }

  for (auto &npc : npcManager_->npcRunners_) {
    if (npc.finished && npc.finishRank < 0) {
      finishCount_++;
      npc.finishRank = finishCount_;
    }
  }

  if (!isPlayerFinished_ && finishCount_ >= qualifyCount_) {
    raceResultState_ = RaceResultState::GameOver;
    isRaceFinished_ = true;

    if (!isFailureMenuOpen_) {
      OpenFailureMenuTravel();
    }
    return;
  }

  //==============================
  // NPCのゴール判定
  //==============================
  for (auto &npc : npcManager_->npcRunners_) {
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























void TravelScene::OpenFailureMenuTravel() {
  isFailureMenuOpen_ = true;
  selectedRetryChoiceTravel_ = RetryChoiceTravel::RetryTravel;
  failureMenuInputCooldown_ = 0.15f;
}

void TravelScene::DecideFailureMenuTravel() {
  if (fade_.IsBusy() || isStartTransition_) {
    return;
  }

  switch (selectedRetryChoiceTravel_) {
  case RetryChoiceTravel::BackToPrompt:
    pendingFailureOutcome_ = SceneOutcome::RETURN_PROMPT;
    break;

  case RetryChoiceTravel::RetryMod:
    pendingFailureOutcome_ = SceneOutcome::RETRY_MOD;
    ModBody::RequestResetOnNextModSceneEntry();
    break;

  case RetryChoiceTravel::RetryTravel:
    pendingFailureOutcome_ = SceneOutcome::RETRY;
    break;

  default:
    pendingFailureOutcome_ = SceneOutcome::NONE;
    break;
  }

  if (pendingFailureOutcome_ != SceneOutcome::NONE) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
    nextOutcome_ = pendingFailureOutcome_;
  }
}

void TravelScene::UpdateFailureMenuInputTravel() {
  if (!isFailureMenuOpen_) {
    return;
  }

  const float dt = system_->GetDeltaTime();
  if (failureMenuInputCooldown_ > 0.0f) {
    failureMenuInputCooldown_ -= dt;
    if (failureMenuInputCooldown_ < 0.0f) {
      failureMenuInputCooldown_ = 0.0f;
    }
  }

  const Vector2 mouse = system_->GetMousePosVector2();

  struct MenuRect {
    Vector2 center;
    Vector2 size;
  };

  const MenuRect promptRect{{640.0f, 300.0f}, {500.0f, 64.0f}};
  const MenuRect retryModRect{{640.0f, 380.0f}, {500.0f, 64.0f}};
  const MenuRect retryTravelRect{{640.0f, 460.0f}, {500.0f, 64.0f}};

  auto IsInside = [](const Vector2 &p, const MenuRect &r) -> bool {
    const float left = r.center.x - r.size.x * 0.5f;
    const float right = r.center.x + r.size.x * 0.5f;
    const float top = r.center.y - r.size.y * 0.5f;
    const float bottom = r.center.y + r.size.y * 0.5f;
    return p.x >= left && p.x <= right && p.y >= top && p.y <= bottom;
  };

  if (IsInside(mouse, promptRect)) {
    selectedRetryChoiceTravel_ = RetryChoiceTravel::BackToPrompt;
  } else if (IsInside(mouse, retryModRect)) {
    selectedRetryChoiceTravel_ = RetryChoiceTravel::RetryMod;
  } else if (IsInside(mouse, retryTravelRect)) {
    selectedRetryChoiceTravel_ = RetryChoiceTravel::RetryTravel;
  }

  if (failureMenuInputCooldown_ <= 0.0f) {
    if (system_->GetTriggerOn(DIK_UP) || system_->GetTriggerOn(DIK_W)) {
      if (selectedRetryChoiceTravel_ == RetryChoiceTravel::RetryMod) {
        selectedRetryChoiceTravel_ = RetryChoiceTravel::BackToPrompt;
      } else if (selectedRetryChoiceTravel_ == RetryChoiceTravel::RetryTravel) {
        selectedRetryChoiceTravel_ = RetryChoiceTravel::RetryMod;
      }
      failureMenuInputCooldown_ = 0.12f;
    }

    if (system_->GetTriggerOn(DIK_DOWN) || system_->GetTriggerOn(DIK_S)) {
      if (selectedRetryChoiceTravel_ == RetryChoiceTravel::BackToPrompt) {
        selectedRetryChoiceTravel_ = RetryChoiceTravel::RetryMod;
      } else if (selectedRetryChoiceTravel_ == RetryChoiceTravel::RetryMod) {
        selectedRetryChoiceTravel_ = RetryChoiceTravel::RetryTravel;
      }
      failureMenuInputCooldown_ = 0.12f;
    }

    const bool mouseClicked = system_->GetMouseTriggerOn(0);
    const bool keyConfirm =
        system_->GetTriggerOn(DIK_RETURN) || system_->GetTriggerOn(DIK_SPACE);

    if (mouseClicked || keyConfirm) {
      DecideFailureMenuTravel();
    }
  }
}

void TravelScene::DrawFailureMenuTravel() {
  if (!isFailureMenuOpen_) {
    return;
  }

  const Vector4 normalColor = {1.0f, 1.0f, 1.0f, 1.0f};
  const Vector4 selectedColor = {1.0f, 1.0f, 0.2f, 1.0f};

  // bitmapFont.RenderText("game OVER", {640.0f, 220.0f}, 72.0f,
  //                       BitmapFont::Align::Center, 5.0f,
  //                       {1.0f, 0.35f, 0.35f, 1.0f});

  bitmapFont.RenderText(
      "おだいせんたくにもどる", {640.0f, 300.0f},
      selectedRetryChoiceTravel_ == RetryChoiceTravel::BackToPrompt ? 44.0f
                                                                    : 36.0f,
      BitmapFont::Align::Center, 5.0f,
      selectedRetryChoiceTravel_ == RetryChoiceTravel::BackToPrompt
          ? selectedColor
          : normalColor);

  bitmapFont.RenderText(
      "かいぞうにもどる", {640.0f, 380.0f},
      selectedRetryChoiceTravel_ == RetryChoiceTravel::RetryMod ? 44.0f : 36.0f,
      BitmapFont::Align::Center, 5.0f,
      selectedRetryChoiceTravel_ == RetryChoiceTravel::RetryMod ? selectedColor
                                                                : normalColor);

  bitmapFont.RenderText(
      "いどうにもどる", {640.0f, 460.0f},
      selectedRetryChoiceTravel_ == RetryChoiceTravel::RetryTravel ? 44.0f
                                                                   : 36.0f,
      BitmapFont::Align::Center, 5.0f,
      selectedRetryChoiceTravel_ == RetryChoiceTravel::RetryTravel
          ? selectedColor
          : normalColor);
}

void TravelScene::BuildExtraVisualParts() {
  ClearExtraVisualParts();

  if (customizeData_ == nullptr) {
    return;
  }

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

  auto GetModelPath = [](ModBodyPart partType) -> const char * {
    switch (partType) {
    case ModBodyPart::Head:
      return "GAME/resources/modBody/head/head.obj";
    case ModBodyPart::Neck:
      return "GAME/resources/modBody/neck/neck.obj";
    case ModBodyPart::RightUpperArm:
      return "GAME/resources/modBody/rightUpperArm/rightUpperArm.obj";
    case ModBodyPart::RightForeArm:
      return "GAME/resources/modBody/rightForeArm/rightForeArm.obj";
    case ModBodyPart::LeftUpperArm:
      return "GAME/resources/modBody/leftUpperArm/leftUpperArm.obj";
    case ModBodyPart::LeftForeArm:
      return "GAME/resources/modBody/leftForeArm/leftForeArm.obj";
    case ModBodyPart::LeftThigh:
      return "GAME/resources/modBody/leftThighs/leftThighs.obj";
    case ModBodyPart::LeftShin:
      return "GAME/resources/modBody/leftShin/leftShin.obj";
    case ModBodyPart::RightThigh:
      return "GAME/resources/modBody/rightThighs/rightThighs.obj";
    case ModBodyPart::RightShin:
      return "GAME/resources/modBody/rightShin/rightShin.obj";
    default:
      return nullptr;
    }
  };

  for (const auto &instance : customizeData_->partInstances) {
    const char *modelPath = GetModelPath(instance.partType);
    if (modelPath == nullptr) {
      continue;
    }

    // 固定1個目を飛ばす
    bool *skipFlag = nullptr;
    switch (instance.partType) {
    case ModBodyPart::Head:
      skipFlag = &baseHeadSkipped;
      break;
    case ModBodyPart::Neck:
      skipFlag = &baseNeckSkipped;
      break;
    case ModBodyPart::RightUpperArm:
      skipFlag = &baseRightUpperArmSkipped;
      break;
    case ModBodyPart::RightForeArm:
      skipFlag = &baseRightForeArmSkipped;
      break;
    case ModBodyPart::LeftUpperArm:
      skipFlag = &baseLeftUpperArmSkipped;
      break;
    case ModBodyPart::LeftForeArm:
      skipFlag = &baseLeftForeArmSkipped;
      break;
    case ModBodyPart::LeftThigh:
      skipFlag = &baseLeftThighSkipped;
      break;
    case ModBodyPart::LeftShin:
      skipFlag = &baseLeftShinSkipped;
      break;
    case ModBodyPart::RightThigh:
      skipFlag = &baseRightThighSkipped;
      break;
    case ModBodyPart::RightShin:
      skipFlag = &baseRightShinSkipped;
      break;
    default:
      break;
    }

    if (skipFlag != nullptr && !(*skipFlag)) {
      *skipFlag = true;
      continue;
    }

    const int modelHandle = system_->SetModelObj(modelPath);

    Object *obj = new Object;
    obj->IntObject(system_);
    obj->CreateModelData(modelHandle);

    obj->mainPosition.transform = CreateDefaultTransform();

    if (!obj->objectParts_.empty()) {
      obj->objectParts_[0].transform = CreateDefaultTransform();
    }

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

  Vector3 baseHeadOffset = {0.0f, 0.0f, 0.0f};
  if (fixedNeck != nullptr && fixedHead != nullptr) {
    baseHeadOffset = {fixedHead->mainPosition.transform.translate.x -
                          fixedNeck->mainPosition.transform.translate.x,
                      fixedHead->mainPosition.transform.translate.y -
                          fixedNeck->mainPosition.transform.translate.y,
                      fixedHead->mainPosition.transform.translate.z -
                          fixedNeck->mainPosition.transform.translate.z};
  }

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

    //==============================
    // 首は ChestBody 基準のワールド位置で置く
    // 頭は首の先端から生やす
    //==============================
    if (partType == ModBodyPart::Neck) {
      Object *chestBody = modObjects_[ToIndex(ModBodyPart::ChestBody)];

      int extraOwnerId = GetExtraSnapshotOwnerId(partType, partId, parentId);

      Vector3 extraSnapRoot = {0.0f, 0.0f, 0.0f};
      Vector3 extraSnapBend = {0.0f, 0.0f, 0.0f};
      Vector3 extraSnapEnd = {0.0f, 0.0f, 0.0f};

      bool hasExtraSnap = GetExtraPartSnapshotPositions(
          extraOwnerId, extraSnapRoot, extraSnapBend, extraSnapEnd);

      Vector3 baseSnapRoot = {0.0f, 0.0f, 0.0f};
      Vector3 baseSnapBend = {0.0f, 0.0f, 0.0f};
      Vector3 baseSnapEnd = {0.0f, 0.0f, 0.0f};

      int basePartId = -1;
      bool hasBasePartId = GetFirstPartTypePartId(partType, basePartId);

      bool hasBaseSnap = false;
      if (hasBasePartId) {
        int baseParentId = -1;
        GetPartInstanceParentId(basePartId, baseParentId);

        int baseOwnerId =
            GetExtraSnapshotOwnerId(partType, basePartId, baseParentId);

        hasBaseSnap = GetExtraPartSnapshotPositions(baseOwnerId, baseSnapRoot,
                                                    baseSnapBend, baseSnapEnd);
      }

      if (chestBody != nullptr) {
        const Vector3 &chestPos = chestBody->mainPosition.transform.translate;
        const Vector3 &baseNeckLocal = source->mainPosition.transform.translate;

        Vector3 snapDelta = {0.0f, 0.0f, 0.0f};
        if (hasExtraSnap && hasBaseSnap) {
          snapDelta = {extraSnapBend.x - baseSnapBend.x,
                       extraSnapBend.y - baseSnapBend.y,
                       extraSnapBend.z - baseSnapBend.z};
        }

        obj->mainPosition.transform.translate = {
            chestPos.x + baseNeckLocal.x + snapDelta.x,
            chestPos.y + baseNeckLocal.y + snapDelta.y,
            chestPos.z + baseNeckLocal.z + snapDelta.z};

      } else {
        obj->mainPosition.transform.translate =
            source->mainPosition.transform.translate;
      }

      Vector3 extraNeckRotate = {0.0f, 0.0f, 0.0f};
      if (GetPartInstanceLocalRotate(partId, extraNeckRotate)) {
        obj->mainPosition.transform.rotate = extraNeckRotate;
      } else {
        obj->mainPosition.transform.rotate =
            source->mainPosition.transform.rotate;
      }

      if (!obj->objectParts_.empty() && !source->objectParts_.empty()) {
        obj->objectParts_[0].transform.scale =
            source->objectParts_[0].transform.scale;
        obj->objectParts_[0].transform.translate =
            source->objectParts_[0].transform.translate;
        obj->objectParts_[0].transform.rotate =
            source->objectParts_[0].transform.rotate;

        if (hasExtraSnap && hasBaseSnap) {
          float extraSegmentLength =
              Length({extraSnapBend.x - extraSnapRoot.x,
                      extraSnapBend.y - extraSnapRoot.y,
                      extraSnapBend.z - extraSnapRoot.z});

          float baseSegmentLength = Length({baseSnapBend.x - baseSnapRoot.x,
                                            baseSnapBend.y - baseSnapRoot.y,
                                            baseSnapBend.z - baseSnapRoot.z});

          if (baseSegmentLength > 0.0001f) {
            float lengthRatio = extraSegmentLength / baseSegmentLength;
            obj->objectParts_[0].transform.scale.y *= lengthRatio;
            obj->objectParts_[0].transform.translate.y *= lengthRatio;
          }
        }
      }

      obj->Update(usingCamera_);
      continue;
    }

    if (partType == ModBodyPart::Head) {
      Object *parentObj = nullptr;

      auto it = extraPartObjectMap.find(parentId);
      if (it != extraPartObjectMap.end()) {
        parentObj = it->second;
      }

      if (parentObj != nullptr) {
        const Vector3 parentRoot = parentObj->mainPosition.transform.translate;

        obj->mainPosition.transform.translate = {
            parentRoot.x + baseHeadOffset.x, parentRoot.y + baseHeadOffset.y,
            parentRoot.z + baseHeadOffset.z};

      } else {
        Object *chestBody = modObjects_[ToIndex(ModBodyPart::ChestBody)];

        if (chestBody != nullptr && fixedHead != nullptr) {
          const Vector3 &chestPos = chestBody->mainPosition.transform.translate;
          const Vector3 &baseHeadLocal =
              fixedHead->mainPosition.transform.translate;

          obj->mainPosition.transform.translate = {
              chestPos.x + baseHeadLocal.x, chestPos.y + baseHeadLocal.y,
              chestPos.z + baseHeadLocal.z};
        } else {
          obj->mainPosition.transform.translate =
              source->mainPosition.transform.translate;
        }
      }

      Vector3 extraHeadRotate = {0.0f, 0.0f, 0.0f};
      if (GetPartInstanceLocalRotate(partId, extraHeadRotate)) {
        obj->mainPosition.transform.rotate = extraHeadRotate;
      } else {
        obj->mainPosition.transform.rotate =
            source->mainPosition.transform.rotate;
      }

      if (!obj->objectParts_.empty() && !source->objectParts_.empty()) {
        obj->objectParts_[0].transform.scale =
            source->objectParts_[0].transform.scale;
        obj->objectParts_[0].transform.translate = {0.0f, 0.0f, 0.0f};
        obj->objectParts_[0].transform.rotate =
            source->objectParts_[0].transform.rotate;
      }

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

    Vector3 snapRoot = {0.0f, 0.0f, 0.0f};
    Vector3 snapBend = {0.0f, 0.0f, 0.0f};
    Vector3 snapEnd = {0.0f, 0.0f, 0.0f};

    bool hasSnapPositions = GetExtraPartSnapshotPositions(
        snapshotOwnerId, snapRoot, snapBend, snapEnd);

    if (!hasSnapPositions) {
      obj->Update(usingCamera_);
      continue;
    }

    Vector3 extraLocal = {0.0f, 0.0f, 0.0f};
    Vector3 baseLocal = {0.0f, 0.0f, 0.0f};

    bool hasExtraLocal = GetPartInstanceLocalTranslate(partId, extraLocal);
    bool hasBaseLocal = GetFirstPartTypeLocalTranslate(partType, baseLocal);

    Vector3 localDelta = {0.0f, 0.0f, 0.0f};
    if (hasExtraLocal && hasBaseLocal) {
      localDelta = {extraLocal.x - baseLocal.x, extraLocal.y - baseLocal.y,
                    extraLocal.z - baseLocal.z};
    }

    //==============================
    // 追加部位の「長さだけ」を反映する
    // fixed の見た目長さ × (extraCP長 / baseCP長)
    //==============================
    if (!obj->objectParts_.empty() && !source->objectParts_.empty()) {
      int basePartId = -1;
      bool hasBasePartId = GetFirstPartTypePartId(partType, basePartId);

      if (hasBasePartId) {
        int extraOwnerId = GetExtraSnapshotOwnerId(partType, partId, parentId);

        int baseParentId = -1;
        GetPartInstanceParentId(basePartId, baseParentId);

        int baseOwnerId =
            GetExtraSnapshotOwnerId(partType, basePartId, baseParentId);

        float extraSegmentLength =
            GetSnapshotSegmentLength(partType, extraOwnerId);
        float baseSegmentLength =
            GetSnapshotSegmentLength(partType, baseOwnerId);

        if (baseSegmentLength > 0.0001f) {
          float lengthRatio = extraSegmentLength / baseSegmentLength;

          obj->objectParts_[0].transform.scale =
              source->objectParts_[0].transform.scale;
          obj->objectParts_[0].transform.translate =
              source->objectParts_[0].transform.translate;

          obj->objectParts_[0].transform.scale.y *= lengthRatio;
          obj->objectParts_[0].transform.translate.y *= lengthRatio;
        }
      }
    }

    //==============================
    // 一段目（上腕）
    // 基本上腕の正しい位置 + extraとの差分
    //==============================
    if (partType == ModBodyPart::LeftUpperArm ||
        partType == ModBodyPart::RightUpperArm) {
      Object *chestBody = modObjects_[ToIndex(ModBodyPart::ChestBody)];

      if (chestBody != nullptr) {
        const Vector3 &chestPos = chestBody->mainPosition.transform.translate;
        const Vector3 &baseArmLocal = source->mainPosition.transform.translate;

        obj->mainPosition.transform.translate = {
            chestPos.x + baseArmLocal.x + localDelta.x,
            chestPos.y + baseArmLocal.y + localDelta.y,
            chestPos.z + baseArmLocal.z + localDelta.z};
      }
    }

    //==============================
    // 一段目（腿）
    // 基本腿の正しい位置 + extraとの差分
    //==============================
    if (partType == ModBodyPart::LeftThigh ||
        partType == ModBodyPart::RightThigh) {
      Object *chestBody = modObjects_[ToIndex(ModBodyPart::ChestBody)];
      Object *stomachBody = modObjects_[ToIndex(ModBodyPart::StomachBody)];

      if (chestBody != nullptr && stomachBody != nullptr) {
        const Vector3 &chestPos = chestBody->mainPosition.transform.translate;
        const Vector3 &stomachLocal =
            stomachBody->mainPosition.transform.translate;
        const Vector3 &baseLegLocal = source->mainPosition.transform.translate;

        obj->mainPosition.transform.translate = {
            chestPos.x + stomachLocal.x + baseLegLocal.x + localDelta.x,
            chestPos.y + stomachLocal.y + baseLegLocal.y + localDelta.y,
            chestPos.z + stomachLocal.z + baseLegLocal.z + localDelta.z};
      }
    }

    //==============================
    // 二段目（前腕・脛）は親のアニメ後先端から生やす
    //==============================
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
      } else {
        // 親が取れないときだけ snapshot の Bend に逃がす
        obj->mainPosition.transform.translate = snapBend;
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

void TravelScene::ClearExtraVisualParts() {
  for (auto *object : extraObjects_) {
    delete object;
  }

  extraObjects_.clear();
  extraParentIds_.clear();
  extraPartTypes_.clear();
  extraPartIds_.clear();
}