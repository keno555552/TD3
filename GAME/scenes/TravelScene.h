#pragma once
#include "../effect/Fade.h"
#include "BaseScene.h"
#include "GAME/actor/ModBody.h"
#include "GAME/effect/Perfect_Particle.h"
#include "GAME/font/BitmapFont.h"
#include "Object/Object.h"
#include "GAME/actor/TravelRunner.h"
#include "GAME/manager/TravelNpcManager.h"
#include <array>
#include <memory>
#include <string>

class TravelScene : public BaseScene {
public:
  TravelScene(kEngine *system);
  ~TravelScene();

  static void ResetTutorialFlag();

  void Update() override;

  void Draw() override;

private:
  /// <summary>
  /// 更新に必要な部位がそろっているかチェック
  /// </summary>
  /// <returns></returns>
  bool HasRequiredParts() const;

  /// <summary>
  /// 前フレーム保存
  /// </summary>


  /// <summary>
  /// シーン遷移
  /// </summary>
  void UpdateSceneTransition();

private:
  /*ライト
  ------------------------------*/
  Light *light1_ = nullptr;

  /* カメラ
  ------------------------------*/
  Camera *camera_ = nullptr;
  DebugCamera *debugCamera_ = nullptr;
  Camera *usingCamera_ = nullptr;

  bool useDebugCamera_ = false;

  /* ビットマップフォント
  ------------------------------*/
  BitmapFont bitmapFont;

  /* フェード
  ------------------------------*/
  Fade fade_;
  bool isStartTransition_ = false;

  // フェードアウト完了後の遷移先
  SceneOutcome nextOutcome_ = SceneOutcome::NONE;

  std::unique_ptr<ModBodyCustomizeData> customizeData_ = nullptr;







  // ゴール判定
  float goalX_ = 80.0f;
  bool isGoalReached_ = false;

  // 地面
  std::vector<std::unique_ptr<Object>> grounds_;
  uint32_t groundModelHandle_ = 0;

  // 制限時間
  float timeLimit_ = 30.0f;
  float travelTimeLimit_ = 1.0f;
  bool isTimeUp_ = false;




  std::unique_ptr<TravelRunner> player_;

private:
  /// <summary>
  /// 使用するカメラを設定・更新する
  /// </summary>
  void CameraPart();

  void UpdateTimeLimit(float deltaTime);

private:


  

  // NPC
  std::unique_ptr<TravelNpcManager> npcManager_;

  struct RaceEntry {
    bool isPlayer = false;
    int npcIndex = -1;
    float progress = 0.0f;
  };

  int playerRank_ = 1;
  int displayedRank_ = 1;
  float rankAnimationTimer_ = 0.0f;
  const float rankAnimationDuration_ = 0.6f;
  void UpdateRaceRanking();

  // レース通過人数
  int qualifyCount_ = 3;
  bool isPlayerQualified_ = false;
  int goalCount_ = 0;

  // リザルト
  enum class RaceResultState { None, Clear, GameOver };

  bool isPlayerFinished_ = false;
  bool isRaceFinished_ = false;
  int playerFinishRank_ = -1;

  int finishCount_ = 0;

  RaceResultState raceResultState_ = RaceResultState::None;

  void UpdateRaceFinishState();

  bool showBaseModel_ = true;
  bool showExtraModel_ = true;
  bool showNpcModel_ = true;
  int shadowModelHandle_ = 0;

  // ゴールオブジェクト
  std::unique_ptr<Object> goalObject_;
  int goalModelHandle_ = 0;

  // UI
  std::unique_ptr<SimpleSprite> spriteA_;
  std::unique_ptr<SimpleSprite> spriteD_;

  int spriteAHandle_ = 0;
  int spriteDHandle_ = 0;

  // 順位表示用
  std::unique_ptr<SimpleSprite> rankSprites_[5];

  float aKeyFlashTimer_ = 0.0f;
  float dKeyFlashTimer_ = 0.0f;

  float startUITextTimer_ = 0.0f;

  // チュートリアル用
  bool isTutorialMode_;  // チュートリアル用
  std::unique_ptr<SimpleSprite> tutorialBgSprite_;
  int whiteTextureHandle_ = 0;

  // ミニマップ用
  std::unique_ptr<SimpleSprite> minimapLineSprite_;

  /* 失敗時のリトライ選択
  ---------------------------*/
  enum class RetryChoiceTravel {
    BackToPrompt = 0,
    RetryMod,
    RetryTravel,
    Count
  };

  bool isFailureMenuOpen_ = false;
  RetryChoiceTravel selectedRetryChoiceTravel_ = RetryChoiceTravel::RetryTravel;
  float failureMenuInputCooldown_ = 0.0f;

  SceneOutcome pendingFailureOutcome_ = SceneOutcome::NONE;

  void OpenFailureMenuTravel();
  void UpdateFailureMenuInputTravel();
  void DrawFailureMenuTravel();
  void DecideFailureMenuTravel();
};