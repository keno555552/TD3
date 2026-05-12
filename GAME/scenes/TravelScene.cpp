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

  // 改造用の各部位オブジェクトをセットアップ

  customizeData_ = ModBody::CopySharedCustomizeData();
  if (customizeData_ == nullptr) {
    customizeData_ = ModBody::CreateDefaultCustomizeData();
  }

  player_->SetCustomizeData(customizeData_.get());
  player_->LoadCustomizeData();

  player_->BuildFeaturesFromCustomizeData();
  player_->BuildAllVisualParts();

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
    obj->CreateModelData(system_->SetModelObj("GAME/resources/modBody/head/head.obj"));
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



  for (auto &npc : npcManager_->npcRunners_) {
    npcManager_->ClearNpcCustomizedVisual(npc);
  }

  player_->ClearParticle();


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
  if (showBaseModel_) {
    player_->DrawModObjects(usingCamera_);
  }

  npcManager_->DrawNpcs(goalX_, showNpcModel_);

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

  ImGui::Checkbox("Show Base Model", &showBaseModel_);
  ImGui::Checkbox("Show Extra Model", &showExtraModel_);
  ImGui::Separator();

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




