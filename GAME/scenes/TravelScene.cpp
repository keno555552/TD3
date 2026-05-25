#include "TravelScene.h"
#include "GAME/actor/ModCustomizeDataStore.h"
#include "GAME/actor/TravelRunner.h"
#include "Scenes/SceneManager.h"
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

static bool s_hasSeenTravelTutorial = false;

void TravelScene::ResetTutorialFlag() { s_hasSeenTravelTutorial = false; }

TravelScene::TravelScene(kEngine *system) {
  player_ = std::make_unique<TravelRunner>(system);
  player_->SetIsPlayer(true);
  npcManager_ = std::make_unique<TravelNpcManager>(system);
  player_->Initialize(-18.0f);
  Logger::Log("TravelScene ctor");
  system_ = system;
  confettiParticle_ = std::make_unique<ConfettiParticle>(system_);
  confettiParticle_->ClearAll();

  isTutorialMode_ = !s_hasSeenTravelTutorial;

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
  loupeCamera_ = system_->CreateCamera();

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

  // player_->SetLeftLegBend(player_->GetLegRecoverAngle());
  // player_->SetRightLegBend(player_->GetLegRecoverAngle());
  player_->SetLeftLegBend(0.0f);
  player_->SetRightLegBend(0.0f);

  player_->SetLeftLegPrevBend(player_->GetLeftLegBend());
  player_->SetRightLegPrevBend(player_->GetRightLegBend());

  player_->SetLeftLegBendSpeed(0.0f);
  player_->SetRightLegBendSpeed(0.0f);
  player_->SetLeftLegPrevBendSpeed(0.0f);
  player_->SetRightLegPrevBendSpeed(0.0f);

  player_->SetBodyTilt(0.0f);
  player_->SetBodyTiltVelocity(0.0f);

  player_->SetLeftDriveAccum(0.0f);
  player_->SetRightDriveAccum(0.0f);
  player_->SetLeftHoldTime(0.0f);
  player_->SetRightHoldTime(0.0f);
  player_->SetLastKickSide(0);

  player_->SetMoveX(-18.0f);

  player_->SetGaitTiltTarget(0.0f);
  player_->SetLandTimer(999.0f);

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

    // 影が見えるように地面/背景は明るめのグレーにする
    ground->objectParts_[0].materialConfig->textureColor = {0.5f, 0.5f, 0.5f,
                                                            1.0f};

    grounds_.push_back(std::move(ground));
  }

  // スカイドーム
  skydomeModelHandle_ =
      system_->SetModelObj("GAME/resources/skydome/SkyDome.obj");
  skydome_ = std::make_unique<Object>();
  skydome_->IntObject(system_);
  skydome_->CreateModelData(skydomeModelHandle_);
  skydome_->mainPosition.transform = CreateDefaultTransform();
  // 空が大きくなるようにスケール調整
  skydome_->mainPosition.transform.scale = {100.0f, 100.0f, 100.0f};
  for (auto &part : skydome_->objectParts_) {
    part.materialConfig->enableLighting = false;
  }

  // 影 (原点が中心にある球を潰して使用する)
  player_->GetShadowRef() = std::make_unique<Object>();
  player_->GetShadowRef()->IntObject(system_);
  player_->GetShadowRef()->CreateModelData(
      config::default_Sphere_MeshBufferHandle_);
  player_->GetShadowRef()->mainPosition.transform = CreateDefaultTransform();

  player_->GetShadowRef()->mainPosition.transform.translate = {
      0.0f, player_->GetGroundY() + 0.01f, player_->GetMoveX()};
  player_->GetShadowRef()->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  player_->GetShadowRef()->mainPosition.transform.scale = {1.2f, 0.02f, 1.2f};

  player_->GetShadowRef()->objectParts_[0].materialConfig->textureColor = {
      0.0f, 0.0f, 0.0f, 0.9f};
  int shadowTex =
      system_->LoadTexture("GAME/resources/texture/white100x100.png");
  player_->GetShadowRef()->objectParts_[0].materialConfig->textureHandle =
      shadowTex;

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

  // クリア演出用扉と光
  leftDoorModelHandle_ =
      system_->SetModelObj("GAME/resources/model/left_door.obj");
  rightDoorModelHandle_ =
      system_->SetModelObj("GAME/resources/model/right_door.obj");
  float doorZ = goalX_ + 60.0f;
  leftDoor_ = std::make_unique<Object>();
  leftDoor_->IntObject(system_);
  leftDoor_->CreateModelData(leftDoorModelHandle_);
  leftDoor_->mainPosition.transform = CreateDefaultTransform();
  leftDoor_->mainPosition.transform.translate = {-16.0f, 0.0f, doorZ};
  leftDoor_->mainPosition.transform.scale = {4.0f, 4.0f, 4.0f};
  leftDoor_->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  // 紺色
  leftDoor_->objectParts_[0].materialConfig->textureColor = {0.15f, 0.2f, 0.45f,
                                                             1.0f};

  rightDoor_ = std::make_unique<Object>();
  rightDoor_->IntObject(system_);
  rightDoor_->CreateModelData(rightDoorModelHandle_);
  rightDoor_->mainPosition.transform = CreateDefaultTransform();
  rightDoor_->mainPosition.transform.translate = {16.0f, 0.0f, doorZ};
  rightDoor_->mainPosition.transform.scale = {4.0f, 4.0f, 4.0f};
  rightDoor_->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  rightDoor_->objectParts_[0].materialConfig->textureColor = {0.15f, 0.2f,
                                                              0.45f, 1.0f};

  // 建物に見えるように、左右と上に巨大な壁を配置
  leftWall_ = std::make_unique<Object>();
  leftWall_->IntObject(system_);
  leftWall_->CreateModelData(leftDoorModelHandle_);
  leftWall_->mainPosition.transform = CreateDefaultTransform();
  leftWall_->mainPosition.transform.translate = {-56.0f, 0.0f, doorZ};
  leftWall_->mainPosition.transform.scale = {10.0f, 10.0f, 4.0f};
  leftWall_->objectParts_[0].materialConfig->textureColor = {0.15f, 0.2f, 0.45f,
                                                             1.0f};

  rightWall_ = std::make_unique<Object>();
  rightWall_->IntObject(system_);
  rightWall_->CreateModelData(rightDoorModelHandle_);
  rightWall_->mainPosition.transform = CreateDefaultTransform();
  rightWall_->mainPosition.transform.translate = {56.0f, 0.0f, doorZ};
  rightWall_->mainPosition.transform.scale = {10.0f, 10.0f, 4.0f};
  rightWall_->objectParts_[0].materialConfig->textureColor = {0.15f, 0.2f,
                                                              0.45f, 1.0f};

  topWall_ = std::make_unique<Object>();
  topWall_->IntObject(system_);
  topWall_->CreateModelData(leftDoorModelHandle_);
  topWall_->mainPosition.transform = CreateDefaultTransform();
  topWall_->mainPosition.transform.translate = {-16.0f, 24.0f, doorZ};
  topWall_->mainPosition.transform.scale = {8.0f, 10.0f, 4.0f};
  topWall_->objectParts_[0].materialConfig->textureColor = {0.15f, 0.2f, 0.45f,
                                                            1.0f};

  doorLight_ = std::make_unique<Object>();
  doorLight_->IntObject(system_);
  doorLight_->CreateModelData(config::default_Sphere_MeshBufferHandle_);
  doorLight_->mainPosition.transform = CreateDefaultTransform();
  // 光を少し手前に寄せて、ドアの隙間から強烈に漏れ出るようにする
  doorLight_->mainPosition.transform.translate = {0.0f, 10.0f, doorZ + 15.0f};
  doorLight_->mainPosition.transform.scale = {45.0f, 45.0f, 1.0f};
  doorLight_->objectParts_[0].materialConfig->enableLighting = false;
  doorLight_->objectParts_[0].materialConfig->textureColor = {4.0f, 4.0f, 4.0f,
                                                              1.0f}; // 強い白光

  // エラー防止のため無地の白テクスチャをセット
  int doorLightTex =
      system_->LoadTexture("GAME/resources/texture/white100x100.png");
  doorLight_->objectParts_[0].materialConfig->textureHandle = doorLightTex;

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
  spriteA_->mainPosition.transform.translate = {10.0f, 520.0f, 0.0f};
  spriteA_->mainPosition.transform.scale = {0.6f, 0.6f, 1.0f};

  spriteD_ = std::make_unique<SimpleSprite>();
  spriteD_->IntObject(system_);
  spriteD_->CreateDefaultData();
  spriteD_->objectParts_[0].materialConfig->textureHandle = spriteDHandle_;
  spriteD_->mainPosition.transform.translate = {100.0f, 520.0f, 0.0f};
  spriteD_->mainPosition.transform.scale = {0.6f, 0.6f, 1.0f};

  // ルーペ用テクスチャ読み込み
  loupeFrameTexHandle_ =
      system_->LoadTexture("GAME/resources/texture/loupe_frame.png");

  loupeFrameSprite_ = std::make_unique<SimpleSprite>();
  loupeFrameSprite_->IntObject(system_);
  loupeFrameSprite_->CreateDefaultData();
  loupeFrameSprite_->objectParts_[0].materialConfig->textureHandle =
      loupeFrameTexHandle_;

  // 256x256ピクセルぴったりにするためのローカル頂点設定（余白をなくす）
  float lhw = 128.0f;
  float lhh = 128.0f;
  loupeFrameSprite_->objectParts_[0].conerData.coner[0] = {-lhw, -lhh};
  loupeFrameSprite_->objectParts_[0].conerData.coner[1] = {-lhw, lhh};
  loupeFrameSprite_->objectParts_[0].conerData.coner[2] = {lhw, lhh};
  loupeFrameSprite_->objectParts_[0].conerData.coner[3] = {lhw, -lhh};

  // ルーペの背景（Spriteより奥に描画されるため、3Dの球を潰して円盤にする）
  int whiteTex =
      system_->LoadTexture("GAME/resources/texture/white100x100.png");
  loupeBgObj_ = std::make_unique<Object>();
  loupeBgObj_->IntObject(system_);
  loupeBgObj_->CreateModelData(config::default_Sphere_MeshBufferHandle_);
  loupeBgObj_->mainPosition.transform = CreateDefaultTransform();
  loupeBgObj_->objectParts_[0].materialConfig->textureColor = {
      0.0f, 0.0f, 0.0f, 0.65f}; // 半透明の黒
  loupeBgObj_->objectParts_[0].materialConfig->textureHandle =
      whiteTex; // エラー防止のため無地の白テクスチャをセット
  loupeBgObj_->objectParts_[0].materialConfig->enableLighting =
      false; // ライティング無効
  for (int i = 0; i < 5; ++i) {
    std::string path = "GAME/resources/" + std::to_string(i + 1) + "st.png";
    if (i == 1)
      path = "GAME/resources/2nd.png";
    else if (i == 2)
      path = "GAME/resources/3rd.png";
    else if (i == 3)
      path = "GAME/resources/4th.png";
    else if (i == 4)
      path = "GAME/resources/5th.png";

    int texHandle = system_->LoadTexture(path);

    rankSprites_[i] = std::make_unique<SimpleSprite>();
    rankSprites_[i]->IntObject(system_);
    rankSprites_[i]->CreateDefaultData();
    rankSprites_[i]->objectParts_[0].materialConfig->textureHandle = texHandle;

    // 完全に画像の中心を(0,0)のピボットにするため、ローカル頂点(conerData)をずらす
    float hw = 64.0f;                                                 // 128 / 2
    float hh = 64.0f;                                                 // 128 / 2
    rankSprites_[i]->objectParts_[0].conerData.coner[0] = {-hw, -hh}; // 左上
    rankSprites_[i]->objectParts_[0].conerData.coner[1] = {-hw, hh};  // 左下
    rankSprites_[i]->objectParts_[0].conerData.coner[2] = {hw, hh};   // 右下
    rankSprites_[i]->objectParts_[0].conerData.coner[3] = {hw, -hh};  // 右上
  }

  startUITextTimer_ = 4.0f; // 表示時間

  //===============================
  // チュートリアルUI
  //===============================
  whiteTextureHandle_ = system_->LoadTexture(
      "kEngine/EngineAssets/TemplateResource/texture/white5x5.png");
  tutorialBgSprite_ = std::make_unique<SimpleSprite>();
  tutorialBgSprite_->IntObject(system_);
  tutorialBgSprite_->CreateDefaultData();
  tutorialBgSprite_->objectParts_[0].materialConfig->textureHandle =
      whiteTextureHandle_;
  tutorialBgSprite_->mainPosition.transform.translate = {0.0f, 0.0f,
                                                         0.01f}; // Zは手前
  tutorialBgSprite_->mainPosition.transform.scale = {2000.0f, 2000.0f, 1.0f};
  tutorialBgSprite_->objectParts_[0].materialConfig->textureColor = {
      0.0f, 0.0f, 0.0f, 0.7f}; // 半透明の黒

  minimapLineSprite_ = std::make_unique<SimpleSprite>();
  minimapLineSprite_->IntObject(system_);
  minimapLineSprite_->CreateDefaultData();
  minimapLineSprite_->objectParts_[0].materialConfig->textureHandle =
      whiteTextureHandle_;
  minimapLineSprite_->objectParts_[0].materialConfig->textureColor = {
      0.0f, 0.0f, 0.0f, 0.7f}; // 半透明の黒
  minimapLineSprite_->mainPosition.transform.translate = {180.0f, 680.0f, 5.0f};
  minimapLineSprite_->mainPosition.transform.scale = {
      164.0f, 3.0f, 1.0f}; // 820px幅の線(180から1000まで)

  // 画面全体を覆うホワイトフェード用スプライト
  whiteFadeSprite_ = std::make_unique<SimpleSprite>();
  whiteFadeSprite_->IntObject(system_);
  whiteFadeSprite_->CreateDefaultData();
  whiteFadeSprite_->objectParts_[0].materialConfig->textureHandle =
      whiteTextureHandle_;
  whiteFadeSprite_->objectParts_[0].materialConfig->textureColor = {
      4.0f, 4.0f, 4.0f, 0.0f}; // 透明な白
  whiteFadeSprite_->mainPosition.transform.translate = {0.0f, 0.0f,
                                                        -0.1f}; // 一番手前
  whiteFadeSprite_->mainPosition.transform.scale = {
      2000.0f, 2000.0f, 1.0f}; // 画面全体を覆うスケール

  blackOverlaySprite_ = std::make_unique<SimpleSprite>();
  blackOverlaySprite_->IntObject(system_);
  blackOverlaySprite_->CreateDefaultData();
  blackOverlaySprite_->objectParts_[0].materialConfig->textureHandle =
      whiteTextureHandle_;
  blackOverlaySprite_->objectParts_[0].materialConfig->textureColor = {
      0.0f, 0.0f, 0.0f, 0.0f}; // 透明な黒
  blackOverlaySprite_->mainPosition.transform.translate = {
      0.0f, 0.0f, -0.2f}; // whiteFadeSprite_より少し手前
  blackOverlaySprite_->mainPosition.transform.scale = {
      2000.0f, 2000.0f, 1.0f}; // 画面全体を覆うスケール

  heartbeatSoundHandle_ =
      system_->SoundLoadSE("GAME/resources/sounds/心臓の鼓動1.mp3");

  vignetteTextureHandle_ =
      system_->LoadTexture("GAME/resources/texture/vignette.png");
  if (vignetteTextureHandle_ == -1) {
    vignetteTextureHandle_ = whiteTextureHandle_;
  }
  vignetteSprite_ = std::make_unique<SimpleSprite>();
  vignetteSprite_->IntObject(system_);
  vignetteSprite_->CreateDefaultData();
  vignetteSprite_->objectParts_[0].materialConfig->textureHandle =
      vignetteTextureHandle_;
  vignetteSprite_->objectParts_[0].materialConfig->textureColor = {1.0f, 1.0f,
                                                                   1.0f, 0.0f};
  vignetteSprite_->mainPosition.transform.translate = {0.0f, 0.0f, -0.3f};
  vignetteSprite_->mainPosition.transform.scale = {1.0f, 1.0f, 1.0f};

  // 待機プレイヤー表示用のUI画像===============================
  // NPC
  //===============================
  npcManager_->InitializeNpcRunners(customizeData_.get(), player_.get(),
                                    goalX_);

  //===============================
  // パーティクル
  //===============================

  pendingFailureOutcome_ = SceneOutcome::NONE;
  isFailureMenuOpen_ = false;
  failureMenuInputCooldown_ = 0.0f;
  selectedRetryChoiceTravel_ = RetryChoiceTravel::RetryTravel;

  //===============================
  // 初回フレームの座標(0,0,0)バグ対策
  //===============================
  CameraPart();
  for (int i = 0; i < 10; ++i) {
    player_->ApplyVisualState();
    player_->ResolveVisualGroundPenetration();
  }
  player_->ApplyVisualState();
  npcManager_->UpdateNpcRunners(0.0f, goalX_, usingCamera_);
}

TravelScene::~TravelScene() {
  Logger::Log("TravelScene dtor");
  system_->DestroyCamera(camera_);
  system_->DestroyCamera(debugCamera_);
  system_->DestroyCamera(loupeCamera_);

  player_->ClearParticle();

  if (npcManager_) {
    for (auto &npc : npcManager_->npcRunners_) {
      if (npc.runner) {
        npc.runner->ClearParticle();
      }
    }
  }

  system_->RemoveLight(light1_);

  delete light1_;

  if (heartbeatSoundHandle_ != -1) {
    system_->SoundStop(heartbeatSoundHandle_);
  }

  bitmapFont.Cleanup();

  skydome_.reset();
  player_.reset();
  npcManager_.reset();
  grounds_.clear();

  ResourceManager::GetInstance()->CleanupUnusedMaterials();
}

void TravelScene::Update() {
#ifdef _DEBUG
  if (system_->GetTriggerOn(DIK_0)) {
    useDebugCamera_ = !useDebugCamera_;
  }
#endif

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
  // チュートリアル入力待ち
  //===============================
  if (isTutorialMode_) {
    if (system_->GetTriggerOn(DIK_SPACE) || system_->GetMouseTriggerOn(0)) {
      isTutorialMode_ = false;
      s_hasSeenTravelTutorial = true;
    }
    fade_.Update(usingCamera_);
    tutorialBgSprite_->Update(nullptr);
    return;
  }

  //===============================
  // ゲームオーバー演出の更新（メニュー表示中もタイマーを進めるため先に処理）
  //===============================
  if (isGameOverAnimPlaying_) {
    gameOverAnimTimer_ += system_->GetDeltaTime();

    // 演出中は暗転用のアルファ値を更新
    float alpha =
        std::clamp(gameOverAnimTimer_ / gameOverAnimDuration_, 0.0f, 0.7f);
    if (blackOverlaySprite_) {
      blackOverlaySprite_->objectParts_[0].materialConfig->textureColor.w =
          alpha;
    }

    // 演出終了後にメニューを開く
    if (gameOverAnimTimer_ >= gameOverAnimDuration_ && !isFailureMenuOpen_) {
      OpenFailureMenuTravel();
    }
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

#ifdef _DEBUG
  // デバッグ用: Cキーでゴール手前へワープ
  if (system_->GetIsPush(DIK_C)) {
    player_->SetMoveX(goalX_ - 5.0f);
  }
  // デバッグ用: Pキーでピンチ演出の強制ON/OFF
  if (system_->GetTriggerOn(DIK_P)) {
    debugDangerMode_ = !debugDangerMode_;
  }
#endif

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

  if (!isRaceFinished_ && !isGameOverAnimPlaying_) {
    player_->UpdateHoldState(leftNowInput, rightNowInput, deltaTime);
    player_->UpdateLegBendState(leftNowInput, rightNowInput);
    player_->UpdateMovementState(leftNowInput, rightNowInput);

    UpdateRaceRanking();
    UpdateRaceFinishState();
  }

  if (!isGameOverAnimPlaying_) {
    npcManager_->UpdateNpcRunners(deltaTime, goalX_, usingCamera_);
  }

  //==============================
  // ピンチ演出（Danger Mode）の評価と更新
  //==============================
  bool shouldBeDanger = false;
  if (!isPlayerFinished_ && !isGameOverAnimPlaying_ && !isRaceFinished_ &&
      (qualifyCount_ - goalCount_ == 1)) {
    shouldBeDanger = true;
  }
#ifdef _DEBUG
  if (debugDangerMode_) {
    shouldBeDanger = true;
  }
#endif

  if (shouldBeDanger) {
    if (!isDangerMode_) {
      isDangerMode_ = true;
      if (heartbeatSoundHandle_ != -1) {
        system_->SoundPlayBGM(heartbeatSoundHandle_, 1.0f);
      }
    }
  } else {
    if (isDangerMode_) {
      isDangerMode_ = false;
      if (heartbeatSoundHandle_ != -1) {
        system_->SoundStop(heartbeatSoundHandle_);
      }
    }
  }

  if (isDangerMode_) {
    dangerAnimTimer_ += deltaTime;
    // 心音に合わせた明滅 (0.05 〜 0.25)
    // 6.28f (= 2π) にすることで数学的に「ピッタリ1秒に1回」のペースになります
    float pulse = (std::sin(dangerAnimTimer_ * 6.28f) + 1.0f) * 0.5f;
    float alpha = 0.05f + pulse * 0.2f;
    if (vignetteSprite_) {
      vignetteSprite_->objectParts_[0].materialConfig->textureColor.w = alpha;
    }
  } else {
    if (vignetteSprite_) {
      vignetteSprite_->objectParts_[0].materialConfig->textureColor.w = 0.0f;
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

  if (skydome_) {
    skydome_->mainPosition.transform.translate =
        usingCamera_->GetTransform().translate;
    skydome_->Update(usingCamera_);
  }

  // ルーペの更新
  isLoupeActive_ = false;
  if (!useDebugCamera_ && !isClearAnimPlaying_) {
    // プレイヤーのワールド座標（移動方向はZ軸、高さはY軸）
    Vector3 playerWorld = {0.0f,
                           player_->GetMoveY() + player_->GetVisualLiftY(),
                           player_->GetMoveX()};
    Vector3 headWorld = playerWorld;
    headWorld.y += 5.0f; // 少し頭上を基準に
    Vector2 screenPos = usingCamera_->GetObjectScreenPos(headWorld);

    // ルーペが出るタイミングがまだ早い問題の修正（完全に画面外はるか上に消えてからにする）
    if (screenPos.y < -300.0f) {
      isLoupeActive_ = true;
      loupeScreenX_ =
          std::clamp(screenPos.x, 100.0f, 1180.0f); // 画面幅に収める

      // スケール計算（高い時はもっと小さくなるように範囲を拡張）
      float distance = std::abs(screenPos.y + 300.0f);
      float t = std::clamp(distance / 1200.0f, 0.0f, 1.0f);
      loupeScale_ = 1.0f - (t * 0.7f); // 最小で0.3倍まで小さくする

      // ルーペが天井との間に隙間ができる問題の修正
      float radius = 128.0f * 0.6f * loupeScale_;
      loupeScreenY_ = radius + 10.0f; // 上端から10pxのマージン

      // プレイヤーをさらに小さくする（カメラの基準距離を130から180に遠ざける）
      float camDistance = 180.0f / loupeScale_;
      Vector3 camPos = playerWorld;
      camPos.x += camDistance; // サイドから写すためにX軸側にカメラを引く

      // カメラの位置を逆算してずらす (kEngineのデフォルトFOV 0.45f
      // に合わせた正確な係数)
      // これがズレていたため、画面端に行くほどプレイヤーが枠外に飛び出していました。
      float units_per_pixel = camDistance * 0.0006356f;
      float dx_pixels = loupeScreenX_ - 640.0f;
      float dy_pixels = loupeScreenY_ - 360.0f;

      // カメラを移動させて被写体を画面上でずらす
      camPos.z += (-dx_pixels * units_per_pixel);
      camPos.y += (dy_pixels * units_per_pixel);

      // プレイヤーをもう少し上に表示させる（カメラをさらに下に下げる）
      camPos.y -= 4.0f;

      loupeCamera_->SetTranslate(camPos);
      loupeCamera_->SetRotation({0.0f, -1.57f, 0.0f});

      // スカイドームに遮蔽されてプレイヤーが描画されない問題の根本解決
      loupeCamera_->SetNearClip(camDistance * 0.5f);
      loupeCamera_->SetFarClip(camDistance * 2.0f +
                               100.0f); // 背景用円盤が入るようにFarを少し延長

      loupeCamera_->Update();

      // 背景円盤の更新（カメラの正面奥ではなく、ルーペ枠の中心に重なるようにパース補正して配置）
      float bgDepth = camDistance + 50.0f;
      Vector3 bgPos;
      bgPos.x = camPos.x - bgDepth; // プレイヤーよりさらに奥に配置

      // ルーペ枠の中心となる3D空間上のターゲット位置 T
      Vector3 T = playerWorld;
      T.y -= 4.0f; // 上記のカメラYオフセットと同じ調整値

      // ターゲット位置を基準に、深度の比率を掛けて背景の座標を算出（パースペクティブ補正）
      bgPos.y = camPos.y + (T.y - camPos.y) * (bgDepth / camDistance);
      bgPos.z = camPos.z + (T.z - camPos.z) * (bgDepth / camDistance);

      loupeBgObj_->mainPosition.transform.translate = bgPos;

      // 画面上の半径に合わせてワールド座標での半径を逆算
      float bgRadius = radius * bgDepth * 0.0006356f;
      // 円盤にするためX軸スケールを潰す。少し小さめ(0.95倍)にして枠から出ないようにする
      loupeBgObj_->mainPosition.transform.scale = {0.05f, bgRadius * 0.95f,
                                                   bgRadius * 0.95f};
      loupeBgObj_->Update(loupeCamera_);
    }
  }

  player_->UpdateParticle(camera_);
  if (confettiParticle_) {
    confettiParticle_->Update(camera_);
  }

  UpdateSceneTransition();

  if (startUITextTimer_ > 0.0f) {
    startUITextTimer_ -= system_->GetDeltaTime();
    if (startUITextTimer_ < 0.0f) {
      startUITextTimer_ = 0.0f;
    }
  }

  if (rankAnimationTimer_ > 0.0f) {
    rankAnimationTimer_ -= system_->GetDeltaTime();
    if (rankAnimationTimer_ < 0.0f) {
      rankAnimationTimer_ = 0.0f;
    }
  }

  if (isClearAnimPlaying_) {
    clearAnimTimer_ += system_->GetDeltaTime();
  }
}

void TravelScene::Draw() {
  bool currentPause = SceneManager::GetInstance().IsPause();
  if (currentPause && !wasPaused_) {
    if (heartbeatSoundHandle_ != -1) {
      system_->SoundPause(heartbeatSoundHandle_);
    }
  } else if (!currentPause && wasPaused_) {
    if (heartbeatSoundHandle_ != -1) {
      system_->SoundContinue(heartbeatSoundHandle_);
    }
  }
  wasPaused_ = currentPause;

  // ルーペ用のプレイヤーと背景を最優先で描画（Zバッファクリア直後）
  if (isLoupeActive_) {
    system_->SetCamera(loupeCamera_);
    loupeBgObj_->Draw();
    player_->DrawModObjects(loupeCamera_);
    system_->SetCamera(usingCamera_);
  }

  if (showBaseModel_) {
    player_->DrawModObjects(usingCamera_);
  }

  npcManager_->DrawNpcs(goalX_, showNpcModel_, camera_);

  if (skydome_) {
    skydome_->Draw();
  }

  for (auto &ground : grounds_) {
    ground->Draw();
  }

  if (player_->GetShadowRef() != nullptr) {
    player_->GetShadowRef()->Draw();
  }

  if (goalObject_ != nullptr) {
    goalObject_->Draw();
  }

  // 扉の奥の光 -> 扉 の順で描画
  if (doorLight_ != nullptr) {
    doorLight_->Draw();
  }
  if (leftDoor_ != nullptr) {
    leftDoor_->Draw();
  }
  if (rightDoor_ != nullptr) {
    rightDoor_->Draw();
  }
  if (leftWall_ != nullptr) {
    leftWall_->Draw();
  }
  if (rightWall_ != nullptr) {
    rightWall_->Draw();
  }
  if (topWall_ != nullptr) {
    topWall_->Draw();
  }

  // 最後にホワイトフェードを描画
  if (whiteFadeSprite_ != nullptr &&
      whiteFadeSprite_->objectParts_[0].materialConfig->textureColor.w >
          0.001f) {
    whiteFadeSprite_->Draw();
  }
  if (blackOverlaySprite_ != nullptr &&
      blackOverlaySprite_->objectParts_[0].materialConfig->textureColor.w >
          0.001f) {
    blackOverlaySprite_->Draw();
  }
  if (vignetteSprite_ != nullptr &&
      vignetteSprite_->objectParts_[0].materialConfig->textureColor.w >
          0.001f) {
    vignetteSprite_->Draw();
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
  ImGui::Text("MoveX : %.3f", player_->GetMoveX());
  ImGui::Text("MoveY : %.3f", player_->GetMoveY());
  ImGui::Text("VelocityX : %.3f", player_->GetVelocityX());
  ImGui::Text("VelocityY : %.3f", player_->GetVelocityY());

  //==============================
  // 姿勢確認
  //==============================
  float legDiffTilt = (player_->GetLeftLegBend() - player_->GetRightLegBend()) *
                      player_->GetLegDiffTiltPower();

  float postureError =
      std::abs(player_->GetBodyTilt() - player_->GetIdealRunTilt());
  float badPosture =
      std::clamp(postureError / player_->GetPostureTolerance(), 0.0f, 1.0f);
  float forwardRate = 1.0f - badPosture;
  float upwardRate = 0.30f + badPosture * 0.70f;

  ImGui::Separator();
  ImGui::Text("BodyTilt : %.4f", player_->GetBodyTilt());
  ImGui::Text("LegDiffTilt : %.4f", legDiffTilt);

  ImGui::Text("LeftLegBend : %.4f", player_->GetLeftLegBend());
  ImGui::Text("RightLegBend : %.4f", player_->GetRightLegBend());

  ImGui::Text("ForwardRate : %.4f", forwardRate);
  ImGui::Text("UpwardRate : %.4f", upwardRate);

  ImGui::Text("LeftDriveAccum : %.4f", player_->GetLeftDriveAccum());
  ImGui::Text("RightDriveAccum : %.4f", player_->GetRightDriveAccum());

  ImGui::Text("LeftHoldTime : %.4f", player_->GetLeftHoldTime());
  ImGui::Text("RightHoldTime : %.4f", player_->GetRightHoldTime());

  ImGui::Text("BodyTiltVelocity : %.4f", player_->GetBodyTiltVelocity());
  ImGui::Text("LeftLegBendSpeed : %.4f", player_->GetLeftLegBendSpeed());
  ImGui::Text("RightLegBendSpeed : %.4f", player_->GetRightLegBendSpeed());

  ImGui::Checkbox("Force Tilt", player_->GetDebugForceTiltPtr());
  ImGui::SliderFloat("Tilt Value", player_->GetDebugTiltValuePtr(), -0.4f,
                     0.4f);

  ImGui::Checkbox("Use Customize Move", player_->GetUseCustomizeMovePtr());
  ImGui::Text("runPower: %.2f", player_->GetTuningPtr()->runPower);
  ImGui::Text("lift: %.2f", player_->GetTuningPtr()->lift);
  ImGui::Text("maxSpeed: %.2f", player_->GetTuningPtr()->maxSpeed);
  ImGui::Text("stability: %.2f", player_->GetTuningPtr()->stability);
  ImGui::Text("bodyTilt: %.4f", player_->GetBodyTilt());
  ImGui::Text("turnResponse: %.2f", player_->GetTuningPtr()->turnResponse);

  {
    TravelRunner::LowestBodyPart lowestPart =
        TravelRunner::LowestBodyPart::None;
    float lowestBodyLocalY = player_->GetLowestVisualBodyY(&lowestPart);
    float lowestBodyWorldY =
        player_->GetMoveY() + player_->GetVisualLiftY() + lowestBodyLocalY;
    float penetration = player_->GetGroundY() - lowestBodyWorldY;

    ImGui::Text("--- Ground Penetration ---");
    ImGui::Text("Player Y: %.3f", player_->GetMoveY());
    ImGui::Text("Lift Y: %.3f", player_->GetVisualLiftY());
    ImGui::Text("Lowest Part: %s (%.3f)",
                player_->GetLowestBodyPartName(lowestPart), lowestBodyLocalY);
  }

  ImGui::End();
#endif

#ifdef USE_IMGUI
  ImGui::Begin("Time");

  ImGui::End();
#endif

#ifdef USE_IMGUI
  {
    TravelRunner::LowestBodyPart lowestPart =
        TravelRunner::LowestBodyPart::None;
    float lowestBodyLocalY = player_->GetLowestVisualBodyY(&lowestPart);
    float lowestBodyWorldY =
        player_->GetMoveY() + player_->GetVisualLiftY() + lowestBodyLocalY;
    float penetration = player_->GetGroundY() - lowestBodyWorldY;

    ImGui::Begin("LowestBodyCheck");
    ImGui::Text("LowestBodyLocalY : %.3f", lowestBodyLocalY);
    ImGui::Text("LowestBodyWorldY : %.3f", lowestBodyWorldY);
    ImGui::Text("LowestPart       : %s",
                player_->GetLowestBodyPartName(lowestPart));
    ImGui::Text("GroundY          : %.3f", player_->GetGroundY());
    ImGui::Text("Penetration      : %.3f", penetration);
    ImGui::Text("VisualLiftY      : %.3f", player_->GetVisualLiftY());
    ImGui::End();
  }
#endif

#ifdef USE_IMGUI
  ImGui::Begin("NpcDebug");

  ImGui::Checkbox("Show NPC Model", &showNpcModel_);

  // --- プリセット切り替えのデバッグメニュー ---
  ImGui::Separator();
  const char *presetNames[] = {
      "Default",      "HeadBig", "LongLeg",     "BigTorso",   "Gorilla",
      "Slender",      "Chubby",  "Giant",       "Mini",       "LongArm",
      "WideShoulder", "WideHip", "MutantAsura", "OctopusLegs"};
  static int currentPresetIndex = static_cast<int>(NpcPresetType::BigTorso);
  ImGui::Combo("Force NPC Preset", &currentPresetIndex, presetNames,
               IM_ARRAYSIZE(presetNames));
  if (ImGui::Button("Apply Preset to All NPCs")) {
    for (size_t i = 0; i < npcManager_->npcRunners_.size(); ++i) {
      auto &npc = npcManager_->npcRunners_[i];
      auto presetData = ModCustomizeDataStore::CreateNpcPreset(
          static_cast<NpcPresetType>(currentPresetIndex), nullptr);
      npc.customizeData = std::move(presetData);
      npc.runner->SetCustomizeData(npc.customizeData.get());
      npc.runner->LoadCustomizeData();
      npc.runner->BuildFeaturesFromCustomizeData();
      npc.runner->BuildAllVisualParts();
      npc.runner->ApplyCustomizeToMovementParam();
    }
  }
  ImGui::Separator();
  // ------------------------------------------

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

  if (!isGameOverAnimPlaying_ && !isClearAnimPlaying_) {
    //==============================
    // ミニマップ表示
    //==============================
    {
      float startScreenX = 180.0f;
      float endScreenX = 1000.0f;
      float mapScreenY = 680.0f;

      // 背景のバー
      minimapLineSprite_->Draw();

      // 影付きテキスト描画用ヘルパーラムダ
      auto DrawShadowText = [&](const std::string &text, Vector2 pos,
                                float size, Vector4 color) {
        bitmapFont.RenderText(text, {pos.x + 2.0f, pos.y + 2.0f}, size,
                              BitmapFont::Align::Center, 4.0f,
                              {0.0f, 0.0f, 0.0f, 0.8f});
        bitmapFont.RenderText(text, pos, size, BitmapFont::Align::Center, 4.0f,
                              color);
      };

      DrawShadowText("S", {startScreenX - 40.0f, mapScreenY - 8.0f}, 32.0f,
                     {0.0f, 0.8f, 1.0f, 1.0f});
      DrawShadowText("G", {endScreenX + 40.0f, mapScreenY - 8.0f}, 32.0f,
                     {1.0f, 0.1f, 0.1f, 1.0f});

      auto GetMapX = [&](float moveX) {
        float startRaceX = -18.0f; // 実際の開始座標
        float progress = (moveX - startRaceX) / (goalX_ - startRaceX);
        progress = std::clamp(progress, 0.0f, 1.0f);
        return startScreenX + (endScreenX - startScreenX) * progress;
      };

      // Draw NPCs
      for (size_t i = 0; i < npcManager_->npcRunners_.size(); ++i) {
        if (npcManager_->npcRunners_[i].runner) {
          float nx = GetMapX(npcManager_->npcRunners_[i].runner->GetMoveX());
          std::string icon = std::to_string(i + 1);
          DrawShadowText(icon, {nx, mapScreenY - 30.0f}, 24.0f,
                         {1.0f, 0.3f, 0.3f, 1.0f});
        }
      }

      // Draw Player
      float px = GetMapX(player_->GetMoveX());
      Vector4 playerIconColor = {0.0f, 1.0f, 0.0f, 1.0f}; // 基本は緑
      if (isDangerMode_) {
        float pulse = (std::sin(dangerAnimTimer_ * 6.28f) + 1.0f) * 0.5f;
        // 緑から真っ赤へ心音に合わせて脈打つ
        playerIconColor = {pulse, 1.0f - pulse, 0.0f, 1.0f};
      }
      DrawShadowText("YOU", {px, mapScreenY - 45.0f}, 24.0f, playerIconColor);
      DrawShadowText("v", {px, mapScreenY - 20.0f}, 28.0f, playerIconColor);
    }

    //==============================
    // 順位を表示（くるっとアニメーション対応）
    //==============================
    float baseScale = 1.0f; // Scale is 1x as requested
    float currentScale = baseScale;
    float rotZ = 0.0f;

    if (rankAnimationTimer_ > 0.0f) {
      float t = 1.0f - (rankAnimationTimer_ / rankAnimationDuration_);

      if (t >= 0.5f) {
        displayedRank_ = playerRank_;
      }

      // Shrinks to 0 at t=0.5, then expands back to 1
      float scaleFactor = std::abs(std::cos(t * 3.14159f));
      currentScale = baseScale * scaleFactor;

      // Spin around Z axis
      rotZ = t * 3.14159f * 4.0f;
    } else {
      displayedRank_ = playerRank_;
      rotZ = 0.0f;
    }

    int idx = std::clamp(displayedRank_ - 1, 0, 4);

    rankSprites_[idx]->mainPosition.transform.rotate.x = 0.0f;
    rankSprites_[idx]->mainPosition.transform.rotate.z = rotZ;

    // The center of the sprite is placed exactly at {1100, 560}
    rankSprites_[idx]->mainPosition.transform.translate = {1100.0f, 560.0f,
                                                           0.0f};
    rankSprites_[idx]->mainPosition.transform.scale = {currentScale,
                                                       currentScale, 1.0f};
    rankSprites_[idx]->Draw();

    //==============================
    // 残り枠表示
    //==============================
    std::string goalText = "GOAL " + std::to_string(goalCount_) + "/" +
                           std::to_string(qualifyCount_);

    Vector4 goalColor = {1.0f, 1.0f, 1.0f, 1.0f};
    Vector2 goalPos = {1000.0f, 60.0f};

    if (isDangerMode_) {
      float pulse = (std::sin(dangerAnimTimer_ * 6.28f) + 1.0f) * 0.5f;
      // 心音に合わせて赤く脈打つ
      goalColor = {1.0f, 0.2f + 0.8f * (1.0f - pulse),
                   0.2f + 0.8f * (1.0f - pulse), 1.0f};

      // ガタガタ振動させる（ご要望に合わせてかなり弱め）
      float shakeX = ((rand() % 100) / 100.0f - 0.5f) * 2.5f;
      float shakeY = ((rand() % 100) / 100.0f - 0.5f) * 2.5f;
      goalPos.x += shakeX;
      goalPos.y += shakeY;
    }

    bitmapFont.RenderText(goalText, goalPos, 48, BitmapFont::Align::Left, 5.0f,
                          goalColor);

    // if (raceResultState_ == RaceResultState::Clear) {
    //   bitmapFont.RenderText("CLEAR", {900, 300}, 96,
    //   BitmapFont::Align::Left);
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

      bitmapFont.RenderText("せんちゃく3にん！ゴールまでいそげ！", {150, 100},
                            64, BitmapFont::Align::Left, 5.0f,
                            {1.0f, 1.0f, 1.0f, alpha});
    }

    spriteA_->Draw();
    spriteD_->Draw();

    // ルーペの枠の描画（UIの手前に表示）
    if (isLoupeActive_) {
      // 手前の枠（透過済みの画像なので背景スプライトは不要）
      loupeFrameSprite_->mainPosition.transform.translate = {
          loupeScreenX_, loupeScreenY_, 0.0f};
      loupeFrameSprite_->mainPosition.transform.scale = {
          loupeScale_ * 0.6f, loupeScale_ * 0.6f, 1.0f};
      loupeFrameSprite_->Draw();
    }

    //===============================
    // チュートリアル描画
    //===============================
    if (isTutorialMode_) {
      tutorialBgSprite_->Draw();

      bitmapFont.RenderText("AキーとDキーを こうごに おして はしれ！",
                            {640.0f, 320.0f}, 48.0f, BitmapFont::Align::Center,
                            5.0f, {1.0f, 1.0f, 1.0f, 1.0f});

      bitmapFont.RenderText("クリック または [SPACE] キーで スタート",
                            {640.0f, 480.0f}, 32.0f, BitmapFont::Align::Center,
                            5.0f, {1.0f, 1.0f, 0.5f, 1.0f});
    }

  } // end if (!isGameOverAnimPlaying_ && !isClearAnimPlaying_)

  DrawFailureMenuTravel();

  player_->DrawParticle();
  if (confettiParticle_) {
    confettiParticle_->Draw();
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

    //===============================
    // プレイヤー追従
    //===============================
    Vector3 camPos;

    camPos.x = 48.0f;
    camPos.y = 5.0f;

    if (player_->GetMoveX() + 10.0f <= goalX_ - 10.0f) {
      camPos.z = player_->GetMoveX() + 10.0f;
    } else {
      camPos.z = goalX_ - 10.0f;
    }

    if (isGameOverAnimPlaying_) {
      float progress =
          std::clamp(gameOverAnimTimer_ / gameOverAnimDuration_, 0.0f, 1.0f);
      // イージング (OutCubic)
      float ease = 1.0f - std::pow(1.0f - progress, 3.0f);

      // ターゲット（NPC）の横にカメラを寄せる（キャラクターの大きさに応じて距離と高さを変える）
      // 身長10.0fを基準に、カメラの引き具合(X)と高さ(Y)をスケールさせる
      float sizeScale = std::clamp(failureNpcHeight_ / 10.0f, 0.5f, 5.0f);
      Vector3 endCamPos = {failureNpcPos_.x + 42.0f * sizeScale,
                           failureNpcPos_.y + 3.0f * sizeScale,
                           failureNpcPos_.z};

      camPos.x = cameraStartPos_.x + (endCamPos.x - cameraStartPos_.x) * ease;
      camPos.y = cameraStartPos_.y + (endCamPos.y - cameraStartPos_.y) * ease;
      camPos.z = cameraStartPos_.z + (endCamPos.z - cameraStartPos_.z) * ease;

      camera_->SetRotation({0.0f, -1.57f, 0.0f});
    } else if (isClearAnimPlaying_) {
      // 演出進行度 (0.0 ～ 1.0)
      float progress =
          std::clamp(clearAnimTimer_ / clearAnimDuration_, 0.0f, 1.0f);

      // イージング（滑らかな動き）
      float easeProgress = progress * progress * (3.0f - 2.0f * progress);

      // ディズニーのアトラクション風：
      // 1. 扉をずっと「注視」しながら、扉の手前（プレイヤーの前方）へ回り込む
      // 2. 停止して扉が重々しく開くのを待つ
      // 3. 扉の奥深く（光のトンネルの中）までゆっくり進んでいく
      float phase1End = 0.30f;
      float phase2End = 0.65f;

      float doorZ =
          goalX_ + 60.0f; // カメラとドアを今度こそ離す（前回45からさらに奥へ）

      float startCamX = 48.0f;
      float startCamZ = goalX_ - 10.0f;
      float startCamY = 5.0f;

      float waitCamX = 0.0f;
      float waitCamZ = goalX_ + 3.0f; // プレイヤーの少し前
      float waitCamY = 5.0f;

      float endCamX = 0.0f;
      float endCamZ = doorZ + 60.0f; // 扉の奥深くへ
      float endCamY = 5.0f;

      if (progress < phase1End) {
        // フェーズ1: 扉を注視しながら前に回り込む
        float p = progress / phase1End;
        float ease = p * p * (3.0f - 2.0f * p);
        camPos.x = startCamX + (waitCamX - startCamX) * ease;
        camPos.y = startCamY + (waitCamY - startCamY) * ease;
        camPos.z = startCamZ + (waitCamZ - startCamZ) * ease;

        // 元の横向き(-1.57f)から、扉を注視する角度へ滑らかに回転させる
        float targetRotY = std::atan2(-camPos.x, doorZ - camPos.z);
        float rotY = -1.57f + (targetRotY - (-1.57f)) * ease;
        camera_->SetRotation({-0.08f, rotY, 0.0f});

        if (leftDoor_ && rightDoor_) {
          leftDoor_->mainPosition.transform.rotate.y = 0.0f;
          rightDoor_->mainPosition.transform.rotate.y = 0.0f;
        }
      } else if (progress < phase2End) {
        // フェーズ2: 扉が開きながら、カメラもじわじわとゆっくり前進する
        float p = (progress - phase1End) / (phase2End - phase1End);
        float ease = p * p * (3.0f - 2.0f * p);
        float creepDist = 15.0f; // ドアが開ききるまでに進む距離

        camPos.x = waitCamX;
        camPos.y = waitCamY;
        camPos.z = waitCamZ + creepDist * ease;
        camera_->SetRotation({-0.08f, 0.0f, 0.0f}); // ほんの少し上を向く

        if (leftDoor_ && rightDoor_) {
          // 奥に向かって（+Z方向へ）開くように符号を修正
          leftDoor_->mainPosition.transform.rotate.y = -1.57f * ease;
          rightDoor_->mainPosition.transform.rotate.y = 1.57f * ease;
        }
      } else {
        // フェーズ3: 扉が開ききり、奥の光へ向かって進む
        float p = (progress - phase2End) / (1.0f - phase2End);
        float ease = p * p * (3.0f - 2.0f * p);
        float creepDist = 15.0f;
        float startZ = waitCamZ + creepDist;

        camPos.x = waitCamX + (endCamX - waitCamX) * ease;
        camPos.y = waitCamY + (endCamY - waitCamY) * ease;
        camPos.z = startZ + (endCamZ - startZ) * ease;
        camera_->SetRotation({-0.08f, 0.0f, 0.0f});

        if (leftDoor_ && rightDoor_) {
          leftDoor_->mainPosition.transform.rotate.y = -1.57f;
          rightDoor_->mainPosition.transform.rotate.y = 1.57f;
        }
      }

      // ホワイトフェードの更新（フェーズ2中盤以降で徐々に白く）
      if (whiteFadeSprite_) {
        // progress: 0.0 ~ 1.0
        // 扉が開き始める(phase1End)から完了(1.0)に向けてフェード
        if (progress >= phase1End) {
          float fadeP = (progress - phase1End) / (1.0f - phase1End);
          // 演出を強めるため、二乗で加速的に白くする
          whiteFadeSprite_->objectParts_[0].materialConfig->textureColor.w =
              fadeP * fadeP * 1.5f;
        } else {
          whiteFadeSprite_->objectParts_[0].materialConfig->textureColor.w =
              0.0f;
        }
      }

    } else if (!isGameOverAnimPlaying_) {
      // 向き（横から見る）
      camera_->SetRotation({0.0f, -1.57f, 0.0f});
    }

    camera_->SetTranslate(camPos);
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

void TravelScene::UpdateSceneTransition() {

  //================================
  // レース結果による遷移
  // Clear    -> 次シーン
  // GameOver -> リトライ
  //================================
  // クリアで次シーンへ
  if (isRaceFinished_ && !fade_.IsBusy() && !isStartTransition_) {
    if (raceResultState_ == RaceResultState::Clear) {

      // アニメーションが十分進んでから（あるいは終わってから）フェードアウトする
      if (isClearAnimPlaying_ &&
          clearAnimTimer_ >= clearAnimDuration_ * 0.85f) {
        // ==== ここで暫定順位を計算し、上位2名のNPC情報を保存する ====
        std::vector<int> npcIndices;
        for (size_t i = 0; i < npcManager_->npcRunners_.size(); ++i) {
          if (npcManager_->npcRunners_[i].runner) {
            npcIndices.push_back(static_cast<int>(i));
          }
        }

        // X座標（進んだ距離）で降順ソート
        std::sort(npcIndices.begin(), npcIndices.end(), [&](int a, int b) {
          return npcManager_->npcRunners_[a].runner->GetMoveX() >
                 npcManager_->npcRunners_[b].runner->GetMoveX();
        });

        ModCustomizeDataStore::ClearSharedNpcCustomizeData();
        for (int i = 0; i < 2 && i < static_cast<int>(npcIndices.size()); ++i) {
          int idx = npcIndices[i];
          if (npcManager_->npcRunners_[idx].customizeData != nullptr) {
            ModCustomizeDataStore::SetSharedNpcCustomizeData(
                i, *npcManager_->npcRunners_[idx].customizeData);
          }
        }
        // ==============================================================

        // プレイヤーの到着順位をコンテストシーンへ引き渡す
        ModCustomizeDataStore::SetTravelFinishRank(playerFinishRank_);

        fade_.StartFadeOut();
        isStartTransition_ = true;
        nextOutcome_ = SceneOutcome::NEXT;
      }
    }
  }

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
    entry.progress = player_->GetMoveX();
    entries.push_back(entry);
  }

  // NPC
  for (size_t i = 0; i < npcManager_->npcRunners_.size(); ++i) {
    RaceEntry entry;
    entry.isPlayer = false;
    entry.npcIndex = static_cast<int>(i);
    entry.progress = npcManager_->npcRunners_[i].runner
                         ? npcManager_->npcRunners_[i].runner->GetMoveX()
                         : 0.0f;
    entries.push_back(entry);
  }

  std::sort(entries.begin(), entries.end(),
            [](const RaceEntry &a, const RaceEntry &b) {
              return a.progress > b.progress;
            });

  int oldPlayerRank = playerRank_;

  playerRank_ = 1;

  for (size_t i = 0; i < entries.size(); ++i) {
    if (entries[i].isPlayer) {
      playerRank_ = static_cast<int>(i) + 1;
      isPlayerQualified_ = (playerRank_ <= qualifyCount_);
      break;
    }
  }

  // Trigger flip animation if rank changed
  // (Skip the very first moment to avoid spawn sorting glitches)
  if (oldPlayerRank != playerRank_ && startUITextTimer_ < 3.8f &&
      !isRaceFinished_) {
    rankAnimationTimer_ = rankAnimationDuration_;
  } else if (rankAnimationTimer_ <= 0.0f) {
    displayedRank_ = playerRank_;
  }

  goalCount_ = 0;

  if (player_->GetMoveX() >= goalX_) {
    goalCount_++;
  }

  for (const auto &npc : npcManager_->npcRunners_) {
    if (npc.runner && npc.runner->GetMoveX() >= goalX_) {
      goalCount_++;
    }
  }
}

void TravelScene::UpdateRaceFinishState() {
  if (isRaceFinished_) {
    return;
  }

#ifdef _DEBUG
  // デバッグ用: Gキーで即座にゲームオーバー
  if (system_->GetTriggerOn(DIK_G) && !isGameOverAnimPlaying_ &&
      !isClearAnimPlaying_) {
    raceResultState_ = RaceResultState::GameOver;
    isRaceFinished_ = true;
    isGameOverAnimPlaying_ = true;
    gameOverAnimTimer_ = 0.0f;

    // 対象のNPCの位置を適当に一番進んでいるNPCにする。いなければプレイヤーにする。
    float maxX = -9999.0f;
    bool foundNpc = false;
    for (auto &npc : npcManager_->npcRunners_) {
      if (npc.runner && npc.started) {
        if (npc.runner->GetMoveX() > maxX) {
          maxX = npc.runner->GetMoveX();
          failureNpcPos_ = {npc.runner->GetLaneX(),
                            npc.runner->GetMoveY() +
                                npc.runner->GetVisualLiftY(),
                            npc.runner->GetMoveX()};
          failureNpcHeight_ = npc.runner->GetCharacterHeight();
          foundNpc = true;
        }
      }
    }

    if (!foundNpc) {
      failureNpcPos_ = {player_->GetLaneX(),
                        player_->GetMoveY() + player_->GetVisualLiftY(),
                        player_->GetMoveX()};
      failureNpcHeight_ = player_->GetCharacterHeight();
    }
    cameraStartPos_ = usingCamera_->GetTransform().translate;
    return;
  }
#endif

  //==============================
  // プレイヤーのゴール判定
  //==============================
  if (!isPlayerFinished_ && player_->GetMoveX() >= goalX_) {
    isPlayerFinished_ = true;
    finishCount_++;
    playerFinishRank_ = finishCount_;

    if (confettiParticle_) {
      confettiParticle_->Spawn(
          {player_->GetLaneX(), 10.0f, player_->GetMoveX()}, 0.15f);
    }

    if (playerFinishRank_ <= qualifyCount_) {
      raceResultState_ = RaceResultState::Clear;
      isRaceFinished_ = true;
      if (!isClearAnimPlaying_ && !isStartTransition_) {
        isClearAnimPlaying_ = true;
        clearAnimTimer_ = 0.0f;
      }
    } else {
      raceResultState_ = RaceResultState::GameOver;
      isRaceFinished_ = true;
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

      if (confettiParticle_ && npc.runner) {
        confettiParticle_->Spawn(
            {npc.runner->GetLaneX(), 10.0f, npc.runner->GetMoveX()}, 0.15f);
      }

      // プレイヤー到着前に枠が埋まったらゲームオーバー演出開始
      if (!isPlayerFinished_ && finishCount_ >= qualifyCount_ &&
          !isGameOverAnimPlaying_) {
        raceResultState_ = RaceResultState::GameOver;
        isRaceFinished_ = true;
        isGameOverAnimPlaying_ = true;
        gameOverAnimTimer_ = 0.0f;

        // 対象のNPCの位置、高さ、現在のカメラ位置を保存
        failureNpcPos_ = {npc.runner->GetLaneX(),
                          npc.runner->GetMoveY() + npc.runner->GetVisualLiftY(),
                          npc.runner->GetMoveX()};
        failureNpcHeight_ = npc.runner->GetCharacterHeight();
        cameraStartPos_ = usingCamera_->GetTransform().translate;
      }
    }
  }
}

void TravelScene::OpenFailureMenuTravel() {
  isFailureMenuOpen_ = true;
  selectedRetryChoiceTravel_ = RetryChoiceTravel::RetryTravel;
  failureMenuInputCooldown_ = 0.15f;
  failureMenuAnimTimer_ = 0.0f;
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
  failureMenuAnimTimer_ += dt;

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

  float alpha = std::clamp(failureMenuAnimTimer_ / 0.5f, 0.0f, 1.0f);

  // イージング（ポップイン）
  float easeOutBack = 1.0f + 0.3f * std::pow(1.0f - alpha, 3.0f);
  float yOffset = (1.0f - alpha) * 30.0f;

  const Vector4 normalColor = {1.0f, 1.0f, 1.0f, alpha};
  const Vector4 selectedColor = {1.0f, 1.0f, 0.2f, alpha};

  bitmapFont.RenderText("GAME OVER", {640.0f, 180.0f + yOffset},
                        80.0f * easeOutBack, BitmapFont::Align::Center, 5.0f,
                        {1.0f, 0.2f, 0.2f, alpha});

  bitmapFont.RenderText(
      "おだいせんたくにもどる", {640.0f, 300.0f + yOffset},
      selectedRetryChoiceTravel_ == RetryChoiceTravel::BackToPrompt ? 44.0f
                                                                    : 36.0f,
      BitmapFont::Align::Center, 5.0f,
      selectedRetryChoiceTravel_ == RetryChoiceTravel::BackToPrompt
          ? selectedColor
          : normalColor);

  bitmapFont.RenderText(
      "かいぞうにもどる", {640.0f, 380.0f + yOffset},
      selectedRetryChoiceTravel_ == RetryChoiceTravel::RetryMod ? 44.0f : 36.0f,
      BitmapFont::Align::Center, 5.0f,
      selectedRetryChoiceTravel_ == RetryChoiceTravel::RetryMod ? selectedColor
                                                                : normalColor);

  bitmapFont.RenderText(
      "いどうにもどる", {640.0f, 460.0f + yOffset},
      selectedRetryChoiceTravel_ == RetryChoiceTravel::RetryTravel ? 44.0f
                                                                   : 36.0f,
      BitmapFont::Align::Center, 5.0f,
      selectedRetryChoiceTravel_ == RetryChoiceTravel::RetryTravel
          ? selectedColor
          : normalColor);
}
