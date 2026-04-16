#include "ContestScene.h"
#include "GAME/contest/parts/ShowOffPart.h"
#include "GAME/contest/parts/JudgingPart.h"
#include "GAME/contest/parts/ResultPart.h"
#include "GAME/contest/parts/TrophyPart.h"

ContestScene::ContestScene(kEngine* system) {
	system_ = system;

	// ライト
	light1_ = new Light;
	light1_->direction = { -0.5f, -1.0f, -0.3f };
	light1_->color = { 1.0f, 1.0f, 1.0f };
	light1_->intensity = 1.0f;
	system_->AddLight(light1_);

	// カメラ
	debugCamera_ = system_->CreateDebugCamera();
	camera_ = system_->CreateCamera();
	usingCamera_ = camera_;
	system_->SetCamera(usingCamera_);

	fade_.Initialize(system_);
	fade_.StartFadeIn();

	// PromptData からお題と審査員を取得してスコア計算
	const ThemeData* theme = PromptData::GetThemeData();
	const ModBodyCustomizeData* playerData = ModBody::GetSharedCustomizeData();
	const std::vector<JudgeData>* judges = PromptData::GetJudges();

	if (theme != nullptr && playerData != nullptr) {
		std::vector<JudgeData> judgeList;
		if (judges != nullptr) {
			judgeList = *judges;
		}
		scoreResult_ = ScoreCalculator::Calculate(*theme, *playerData, judgeList);
		isScoreCalculated_ = true;
	}

	// ユーザーデータ読み込み
	userDataManager_ = new UserDataManager();

	// 審査員コメント読み込み＆生成
	JudgeCommentManager::LoadCommentTable(
		"GAME/resources/judges/comments/", judgeCommentTable_);

	if (isScoreCalculated_) {
		judgeCommentResults_ = JudgeCommentManager::GenerateComments(
			judgeCommentTable_, scoreResult_);
	}

	// 二つ名テーブル読み込み
	NicknameManager::LoadNicknameTable(
		"GAME/resources/nicknames/nicknames.json", nicknameTable_);

	// 二つ名生成
	if (isScoreCalculated_) {
		earnedNickname_ = NicknameManager::GenerateNickname(
			nicknameTable_, scoreResult_, userDataManager_->GetUserData(),
			*playerData);
		earnedNickname_.earnedTheme = theme->themeId;

		// ユーザーデータ更新＆保存
		userDataManager_->IncrementPlayCount();
		userDataManager_->UpdateBestRank(scoreResult_.overallRank);
		userDataManager_->AddNickname(earnedNickname_);
		userDataManager_->Save();
	}

	// 観客コメント読み込み＆生成
	AudienceManager::LoadCommentData(
		"GAME/resources/audience/audience_comments.json",
		audienceCommentData_);

	if (isScoreCalculated_) {
		audienceResult_ = AudienceManager::GenerateComments(
			audienceCommentData_, *playerData);
	}

	// フォント初期化
	bitmapFont_.Initialize(system_);

	// 最初のパートを生成
	phase_ = ContestPhase::ShowOff;
	currentPart_ = CreatePart(phase_);
}

ContestScene::~ContestScene() {
	currentPart_.reset();
	bitmapFont_.Cleanup();

	system_->DestroyCamera(camera_);
	system_->DestroyCamera(debugCamera_);
	system_->RemoveLight(light1_);

	delete light1_;

	delete userDataManager_;
	userDataManager_ = nullptr;
}

void ContestScene::Update() {
	CameraPart();

	// フェード中は操作を受け付けない
	if (!fade_.IsBusy() && currentPart_) {
		currentPart_->Update();

		if (currentPart_->IsFinished()) {
			if (phase_ == ContestPhase::Trophy) {
				// トロフィーパートの選択結果を処理
				HandleTrophyChoice();
			} else {
				// 次のパートへ遷移
				AdvancePhase();
			}
		}
	}

	// フェード更新
	fade_.Update(usingCamera_);

	// フェード終了後にシーン移行
	if (isStartTransition_ && fade_.IsFinished()) {
		outcome_ = nextOutcome_;
	}
}

void ContestScene::Draw() {
#ifdef USE_IMGUI
	// 現在シーン・フェーズ表示
	ImGui::Begin("Scene");
	ImGui::Text("ContestScene");

	const char* phaseNames[] = {
		"ShowOff", "Judging", "Result", "Trophy"
	};
	ImGui::Text("Phase: %s", phaseNames[static_cast<int>(phase_)]);
	ImGui::End();
#endif

	// 現在のパートの描画
	if (currentPart_) {
		currentPart_->Draw();
	}

	// フェード描画
	fade_.Draw();
}

void ContestScene::AdvancePhase() {
	switch (phase_) {
	case ContestPhase::ShowOff:
		phase_ = ContestPhase::Judging;
		break;
	case ContestPhase::Judging:
		phase_ = ContestPhase::Result;
		break;
	case ContestPhase::Result:
		phase_ = ContestPhase::Trophy;
		break;
	case ContestPhase::Trophy:
		// Trophyは HandleTrophyChoice で処理するのでここには来ない
		break;
	}

	currentPart_ = CreatePart(phase_);
}

std::unique_ptr<IContestPart> ContestScene::CreatePart(ContestPhase phase) {
	switch (phase) {
	case ContestPhase::ShowOff:
		return std::make_unique<ShowOffPart>(
			system_, &bitmapFont_, audienceResult_);

	case ContestPhase::Judging:
		return std::make_unique<JudgingPart>(
			system_, &bitmapFont_, scoreResult_, judgeCommentResults_);

	case ContestPhase::Result:
		return std::make_unique<ResultPart>(
			system_, &bitmapFont_, scoreResult_, earnedNickname_);

	case ContestPhase::Trophy:
		return std::make_unique<TrophyPart>(system_, &bitmapFont_);

	default:
		return nullptr;
	}
}

void ContestScene::HandleTrophyChoice() {
	// TrophyPartにダウンキャストして選択結果を取得
	auto* trophyPart = dynamic_cast<TrophyPart*>(currentPart_.get());
	if (!trophyPart) return;

	TrophyChoice choice = trophyPart->GetChoice();

	switch (choice) {
	case TrophyChoice::NextTheme:
		fade_.StartFadeOut();
		isStartTransition_ = true;
		nextOutcome_ = SceneOutcome::NEXT;
		break;

	case TrophyChoice::Retry:
		fade_.StartFadeOut();
		isStartTransition_ = true;
		nextOutcome_ = SceneOutcome::RETRY_MOD;
		break;

	case TrophyChoice::Title:
		fade_.StartFadeOut();
		isStartTransition_ = true;
		nextOutcome_ = SceneOutcome::RETURN;
		break;

	default:
		break;
	}
}

void ContestScene::CameraPart() {
	if (useDebugCamera_) {
		usingCamera_ = debugCamera_;
		debugCamera_->MouseControlUpdate();
	} else {
		usingCamera_ = camera_;
	}

	system_->SetCamera(usingCamera_);
}
