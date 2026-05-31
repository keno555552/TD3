#include "ContestScene.h"
#include "GAME/contest/parts/JudgingPart.h"
#include "GAME/contest/parts/ResultPart.h"
#include "GAME/contest/parts/ShowOffPart.h"
#include "GAME/contest/parts/TrophyPart.h"
#include "GAME/contest/parts/RankingPart.h"
#include "GAME/contest/parts/AdvicePart.h"
#include "GAME/contest/parts/SuspensePart.h"
#include "GAME/actor/ModCustomizeDataStore.h"
#include <algorithm>

#include <cmath>

ContestScene::ContestScene(kEngine* system) {
	system_ = system;

	// ライト
	light1_ = new Light;
	light1_->direction = { -0.5f, -1.0f, -0.3f };
	light1_->color = { 1.0f, 1.0f, 1.0f };
	light1_->intensity = 0.2f;
	system_->AddLight(light1_);
	
	judgesSpotLight_ = new Light;
	judgesSpotLight_->lightingType = LightingType::SpotLight;
	judgesSpotLight_->position = { 0.0f,2.0f,-1.65f };
	judgesSpotLight_->direction = { 0.0f, -1.0f, 0.0f };
	judgesSpotLight_->color = { 1.0f, 1.0f, 1.0f };
	judgesSpotLight_->angle = 0.455f;
	judgesSpotLight_->range = 100.0f;
	judgesSpotLight_->intensity = 2.0f;
	judgesSpotLight_->extra0 = 1;
	system_->AddLight(judgesSpotLight_);

	spotlight_ = new Light;
	spotlight_->lightingType = LightingType::SpotLight;
	spotlight_->position = { 0.0f,2.0f,0.15f };
	spotlight_->direction = { 0.0f, -1.0f, 0.0f };
	spotlight_->color = { 1.0f, 1.0f, 1.0f };
	spotlight_->angle = 0.455f;
	spotlight_->range = 100.0f;
	spotlight_->intensity = 5.0f;
	spotlight_->extra0 = 1;
	system_->AddLight(spotlight_);

	// カメラ
	debugCamera_ = system_->CreateDebugCamera();
	camera_ = system_->CreateCamera();
	camera_->SetTranslate({ 0.0f, 3.5f, -13.0f });
	camera_->SetRotation({ 0.15f, 0.0f, 0.0f });
	usingCamera_ = camera_;
	system_->SetCamera(usingCamera_);

	// 最初のパートを生成
	phase_ = ContestPhase::ShowOff;
	currentPart_ = CreatePart(phase_);

	// カメラ初期値を設定
	if (currentPart_) {
		cameraTarget_ = currentPart_->GetCameraTransform();
		cameraCurrent_ = cameraTarget_;
	}

	fade_.Initialize(system_);
	fade_.StartFadeIn();

	// 観客SEループ再生開始（Judging遷移時に停止）
	audienceSoundHandle_ = system_->SoundLoadSE("GAME/resources/sounds/Audience.mp3");
	if (audienceSoundHandle_ != -1) {
		system_->SoundPlayBGM(audienceSoundHandle_, 0.6f);
	}

	float objectScale = 0.2f;
	float PI = 3.14159265f;

	//==================
	// モデル
	//==================
	// ステージオブジェクト

	// 天球
	contestVenueModelHandle_ =
		system_->SetModelObj("GAME/resources/ContestStageObject/contestVenue.obj");
	SetupSceneObject(contestVenue_, contestVenueModelHandle_, { 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f }, 10.0f);

	// ステージ
	stageModelHandle_ =
		system_->SetModelObj("GAME/resources/ContestStageObject/stage.obj");
	SetupSceneObject(stage_, stageModelHandle_, { 0.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f }, objectScale);

	// バックスクリーン
	backScreenModelHandle_ =
		system_->SetModelObj("GAME/resources/ContestStageObject/backScreen.obj");
	SetupSceneObject(backScreen_, backScreenModelHandle_, { 0.0f, 0.2f, 0.8f },
		{ 0.0f, PI, 0.0f }, objectScale);

	// 右サイドスクリーン
	rightSideScreenModelHandle_ = system_->SetModelObj(
		"GAME/resources/ContestStageObject/RightSideScreen.obj");
	rightSideScreens_.resize(2);
	SetupSceneObject(rightSideScreens_[0], rightSideScreenModelHandle_,
		{ 1.4f, 0.2f, 0.3f }, { 0.0f, PI, 0.0f }, objectScale); // 右1
	SetupSceneObject(rightSideScreens_[1], rightSideScreenModelHandle_,
		{ 1.6f, 0.2f, -0.5f }, { 0.0f, PI, 0.0f }, objectScale); // 右2

	// 左サイドスクリーン
	leftSideScreenModelHandle_ = system_->SetModelObj(
		"GAME/resources/ContestStageObject/LeftSideScreen.obj");
	leftSideScreens_.resize(2);
	SetupSceneObject(leftSideScreens_[0], leftSideScreenModelHandle_,
		{ -1.4f, 0.2f, 0.3f }, { 0.0f, PI, 0.0f }, objectScale); // 左1
	SetupSceneObject(leftSideScreens_[1], leftSideScreenModelHandle_,
		{ -1.6f, 0.2f, -0.5f }, { 0.0f, PI, 0.0f }, objectScale); // 左2

	// 床
	floorModelHandle_ =
		system_->SetModelObj("GAME/resources/ContestStageObject/floor.obj");
	SetupSceneObject(floor_, floorModelHandle_, { 0.0f, 0.0f, -5.0f },
		{ 0.0f, PI, 0.0f }, objectScale);

	// 審査員
	judgesModelHandle_ =
		system_->SetModelObj("GAME/resources/judges/models/judges.obj");
	judges_.resize(3);
	SetupSceneObject(judges_[0], judgesModelHandle_, { -0.25f, 0.16f, -2.0f },
		{ 0.0f, 0.0f, 0.0f }, { 0.03f });
	SetupSceneObject(judges_[1], judgesModelHandle_, { 0.0f, 0.16f, -2.0f },
		{ 0.0f, 0.0f, 0.0f }, { 0.03f });
	SetupSceneObject(judges_[2], judgesModelHandle_, { 0.25f, 0.16f, -2.0f },
		{ 0.0f, 0.0f, 0.0f }, { 0.03f });
	// 審査員用ステージ
	judgesStageModelHandle_ =
		system_->SetModelObj("GAME/resources/ContestStageObject/judgesStage.obj");
	SetupSceneObject(judgesStage_, judgesStageModelHandle_, { 0.0f, 0.0f, -2.0f },
		{ 0.0f, 0.0f, 0.0f }, objectScale);

	// 審査員用机
	judgesDeskModelHandle_ =
		system_->SetModelObj("GAME/resources/ContestStageObject/judgesDesk.obj");
	SetupSceneObject(judgesDesk_, judgesDeskModelHandle_, { 0.0f, 0.2f, -1.7f },
		{ 0.0f, PI, 0.0f }, objectScale);

	// 審査員用椅子
	judgesChairModelHandle_ =
		system_->SetModelObj("GAME/resources/ContestStageObject/judgesChair.obj");
	judgesChairs_.resize(3);
	SetupSceneObject(judgesChairs_[0], judgesChairModelHandle_,
		{ -0.25f, 0.195f, -2.0f }, { 0.0f, PI, 0.0f },
		objectScale); // 審査員席1
	SetupSceneObject(judgesChairs_[1], judgesChairModelHandle_,
		{ 0.0f, 0.195f, -2.0f }, { 0.0f, PI, 0.0f },
		objectScale); // 審査員席2
	SetupSceneObject(judgesChairs_[2], judgesChairModelHandle_,
		{ 0.25f, 0.195f, -2.0f }, { 0.0f, PI, 0.0f },
		objectScale); // 審査員席3

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
	JudgeCommentManager::LoadCommentTable("GAME/resources/judges/comments/",
		judgeCommentTable_);

	if (isScoreCalculated_) {
		judgeCommentResults_ =
			JudgeCommentManager::GenerateComments(judgeCommentTable_, scoreResult_);
	}

	// 二つ名テーブル読み込み
	NicknameManager::LoadNicknameTable("GAME/resources/nicknames/nicknames.json",
		nicknameTable_);

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
		"GAME/resources/audience/audience_comments.json", audienceCommentData_);

	if (isScoreCalculated_) {
		audienceResult_ =
			AudienceManager::GenerateComments(audienceCommentData_, *playerData);
	}

	// フォント初期化
	bitmapFont_.Initialize(system_);

	customizedBodyActor_.Initialize(system_);

	//プレイヤーのモデルを再構築
	if (playerData != nullptr) {
		customizedBodyActor_.SetActorScale({ 0.03f, 0.03f, 0.03f });
				customizedBodyActor_.SetActorRotate({ 0.0f, PI, 0.0f });
		customizedBodyActor_.SetActorTranslate({ 0.0f, 0.0f, -0.2f });
    customizedBodyActor_.SetActorRotate({0.0f, PI, 0.0f});

		// ステージ床の高さに合わせる
		customizedBodyActor_.SetGroundY(0.18f);

		// 足首ではなく足裏っぽく少し上げ下げしたいときの微調整
		customizedBodyActor_.SetGroundOffsetY(0.02f);

		customizedBodyActor_.SetAutoGroundEnabled(true);
		customizedBodyActor_.BuildFromCustomizeData(*playerData);
	}

	// NPCのデータ取得と構築
	for (int i = 0; i < 2; ++i) {
		const ModBodyCustomizeData* npcData = ModCustomizeDataStore::GetSharedNpcCustomizeData(i);
		if (npcData != nullptr) {
			npcBodyActors_[i].Initialize(system_);
			npcBodyActors_[i].SetActorScale({ 0.03f, 0.03f, 0.03f });
			npcBodyActors_[i].SetActorRotate({ 0.0f, PI, 0.0f });

			// 配置を左右に散らす
			float npcX = (i == 0) ? -1.0f : 1.0f; // プレイヤーは 0.0f とする
			npcBodyActors_[i].SetActorTranslate({ npcX, 0.0f, -0.2f });
			npcBodyActors_[i].SetGroundY(0.18f);
			npcBodyActors_[i].SetGroundOffsetY(0.02f);
			npcBodyActors_[i].SetAutoGroundEnabled(true);
			npcBodyActors_[i].BuildFromCustomizeData(*npcData);

			// NPC用スポットライト
			npcSpotlights_[i] = new Light;
			npcSpotlights_[i]->lightingType = LightingType::SpotLight;
			npcSpotlights_[i]->position = { npcX, 2.0f, 0.15f };
			npcSpotlights_[i]->direction = { 0.0f, -1.0f, 0.0f };
			npcSpotlights_[i]->color = { 1.0f, 1.0f, 1.0f };
			npcSpotlights_[i]->angle = 0.455f;
			npcSpotlights_[i]->range = 100.0f;
			npcSpotlights_[i]->intensity = 5.0f;
			npcSpotlights_[i]->extra0 = 1;
			system_->AddLight(npcSpotlights_[i]);

			// スコア計算
			if (theme != nullptr) {
				std::vector<JudgeData> judgeList;
				if (judges != nullptr) {
					judgeList = *judges;
				}
				npcScoreResults_[i] = ScoreCalculator::Calculate(*theme, *npcData, judgeList);
				npcScoreCalculated_[i] = true;
			}
		}
	}

	// Ranking data build
	if (isScoreCalculated_) {
		ContestRankEntry pEntry;
		pEntry.name = "あなた";
		pEntry.totalStars = scoreResult_.totalStars;
		pEntry.overallRank = scoreResult_.overallRank;
		pEntry.finalScore = scoreResult_.finalScore;
		pEntry.rank = 0;
		contestRanking_.push_back(pEntry);
	}

	for (int i = 0; i < 2; ++i) {
		if (npcScoreCalculated_[i]) {
			ContestRankEntry nEntry;
			nEntry.name = "ライバル " + std::to_string(i + 1);
			nEntry.totalStars = npcScoreResults_[i].totalStars;
			nEntry.overallRank = npcScoreResults_[i].overallRank;
			nEntry.finalScore = npcScoreResults_[i].finalScore;
			nEntry.rank = 0;
			contestRanking_.push_back(nEntry);
		}
	}

	// スコア順にソートしてRank決定
	std::sort(contestRanking_.begin(), contestRanking_.end(), [](const ContestRankEntry& a, const ContestRankEntry& b) {
		if (a.totalStars != b.totalStars) {
			return a.totalStars > b.totalStars;
		}
		return a.finalScore > b.finalScore;
		});

	// 同じスコアなら同じ順位（dense ranking）
	if (!contestRanking_.empty()) {
		contestRanking_[0].rank = 1;
		for (size_t i = 1; i < contestRanking_.size(); ++i) {
			bool sameScore = (contestRanking_[i].totalStars == contestRanking_[i - 1].totalStars)
				&& (std::abs(contestRanking_[i].finalScore - contestRanking_[i - 1].finalScore) < 0.01f);
			if (sameScore) {
				// 前と同じスコアなら同じ順位
				contestRanking_[i].rank = contestRanking_[i - 1].rank;
			} else {
				// 異なるスコアなら +1（denseランキング：スキップなし）
				contestRanking_[i].rank = contestRanking_[i - 1].rank + 1;
			}
		}
	}


	// 最初のパートを生成
	phase_ = ContestPhase::ShowOff;
	currentPart_ = CreatePart(phase_);
}

ContestScene::~ContestScene() {
	if (audienceSoundHandle_ != -1) {
		system_->SoundStop(audienceSoundHandle_);
	}
	currentPart_.reset();
	bitmapFont_.Cleanup();
	system_->DestroyCamera(camera_);
	system_->DestroyCamera(debugCamera_);
	system_->RemoveLight(spotlight_);
	system_->RemoveLight(judgesSpotLight_);
	for (int i = 0; i < 2; ++i) {
		if (npcSpotlights_[i]) {
			system_->RemoveLight(npcSpotlights_[i]);
		}
	}
	system_->RemoveLight(light1_);

	ResourceManager::GetInstance()->CleanupUnusedMaterials();

	delete light1_;
	delete judgesSpotLight_;
	delete spotlight_;
	for (int i = 0; i < 2; ++i) {
		delete npcSpotlights_[i];
		npcSpotlights_[i] = nullptr;
	}
	delete userDataManager_;
	userDataManager_ = nullptr;
}

void ContestScene::Update() {

	CameraPart();

	// フェード中は操作を受け付けない
	if (!fade_.IsBusy() && currentPart_) {
		// カメラ補間中はパートの更新をスキップ（入力による遷移を無効にする）
		if (!isCameraLerping_) {
			currentPart_->Update();
		}

		if (currentPart_->IsFinished()) {
			if (phase_ == ContestPhase::Trophy) {
				HandleTrophyChoice();
			} else {
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

	bitmapFont_.BeginFrame();

#ifdef USE_IMGUI

	// ステージオブジェクト配置調整
	ImGui::Begin("Stage Objects");

	if (stage_.object) {
		if (ImGui::TreeNode("judgesSpotLight")) {
			ImGui::DragFloat3("Pos##judgesSpotLight",
				&judgesSpotLight_->position.x,
				0.1f);
			ImGui::DragFloat3("Rot##judgesSpotLight",
				&judgesSpotLight_->direction.x, 0.01f);
			ImGui::DragFloat3("Scale##judgesSpotLight",
				&judgesSpotLight_->color.x, 0.01f);
			ImGui::DragFloat("intensity##judgesSpotLight",
				&judgesSpotLight_->intensity, 0.01f);
			ImGui::DragFloat("angle##judgesSpotLight",
				&judgesSpotLight_->angle, 0.01f);
			ImGui::DragFloat("range##judgesSpotLight",
				&judgesSpotLight_->range, 0.01f);
			ImGui::TreePop();
		}
	}

	if (stage_.object) {
		if (ImGui::TreeNode("spotLight")) {
			ImGui::DragFloat3("Pos##spotLight",
				&spotlight_->position.x,
				0.1f);
			ImGui::DragFloat3("Rot##spotLight",
				&spotlight_->direction.x, 0.01f);
			ImGui::DragFloat3("Scale##spotLight",
				&spotlight_->color.x, 0.01f);
			ImGui::DragFloat("intensity##spotLight",
				&spotlight_->intensity, 0.01f);
			ImGui::DragFloat("angle##spotLight",
				&spotlight_->angle, 0.01f);
			ImGui::DragFloat("range##spotLight",
				&spotlight_->range, 0.01f);
			ImGui::TreePop();
		}
	}

	for (int i = 0; i < 2; ++i) {
		if (!npcSpotlights_[i]) continue;
		char label[32];
		snprintf(label, sizeof(label), "npcSpotLight %d", i);
		if (ImGui::TreeNode(label)) {
			ImGui::DragFloat3("Pos", &npcSpotlights_[i]->position.x, 0.1f);
			ImGui::DragFloat3("Dir", &npcSpotlights_[i]->direction.x, 0.01f);
			ImGui::DragFloat3("Color", &npcSpotlights_[i]->color.x, 0.01f);
			ImGui::DragFloat("intensity", &npcSpotlights_[i]->intensity, 0.01f);
			ImGui::DragFloat("angle", &npcSpotlights_[i]->angle, 0.01f);
			ImGui::DragFloat("range", &npcSpotlights_[i]->range, 0.01f);
			ImGui::TreePop();
		}
	}

	if (stage_.object) {
		if (ImGui::TreeNode("Stage")) {
			ImGui::DragFloat3("Pos##stage",
				&stage_.object->mainPosition.transform.translate.x,
				0.1f);
			ImGui::DragFloat3("Rot##stage",
				&stage_.object->mainPosition.transform.rotate.x, 0.01f);
			ImGui::DragFloat3("Scale##stage",
				&stage_.object->mainPosition.transform.scale.x, 0.01f);
			ImGui::TreePop();
		}
	}

	if (backScreen_.object) {
		if (ImGui::TreeNode("BackScreen")) {
			ImGui::DragFloat3("Pos##back",
				&backScreen_.object->mainPosition.transform.translate.x,
				0.1f);
			ImGui::DragFloat3("Rot##back",
				&backScreen_.object->mainPosition.transform.rotate.x,
				0.01f);
			ImGui::DragFloat3("Scale##back",
				&backScreen_.object->mainPosition.transform.scale.x,
				0.01f);
			ImGui::TreePop();
		}
	}

	for (int i = 0; i < (int)rightSideScreens_.size(); ++i) {
		char label[32];
		snprintf(label, sizeof(label), "RightSideScreen %d", i);
		if (ImGui::TreeNode(label)) {
			ImGui::DragFloat3(
				"Pos",
				&rightSideScreens_[i].object->mainPosition.transform.translate.x,
				0.1f);
			ImGui::DragFloat3(
				"Rot", &rightSideScreens_[i].object->mainPosition.transform.rotate.x,
				0.01f);
			ImGui::TreePop();
		}
	}

	for (int i = 0; i < (int)leftSideScreens_.size(); ++i) {
		char label[32];
		snprintf(label, sizeof(label), "LefttSideScreen %d", i);
		if (ImGui::TreeNode(label)) {
			ImGui::DragFloat3(
				"Pos",
				&leftSideScreens_[i].object->mainPosition.transform.translate.x,
				0.1f);
			ImGui::DragFloat3(
				"Rot", &leftSideScreens_[i].object->mainPosition.transform.rotate.x,
				0.01f);
			ImGui::TreePop();
		}
	}

	if (floor_.object) {
		if (ImGui::TreeNode("Floor")) {
			ImGui::DragFloat3("Pos##floor",
				&floor_.object->mainPosition.transform.translate.x,
				0.1f);
			ImGui::DragFloat3("Rot##floor",
				&floor_.object->mainPosition.transform.rotate.x, 0.01f);
			ImGui::DragFloat3("Scale##floor",
				&floor_.object->mainPosition.transform.scale.x, 0.01f);
			ImGui::TreePop();
		}
	}

	if (judgesStage_.object) {
		if (ImGui::TreeNode("JudgesStage")) {
			ImGui::DragFloat3(
				"Pos##judgesStage",
				&judgesStage_.object->mainPosition.transform.translate.x, 0.1f);
			ImGui::DragFloat3("Rot##judgesStage",
				&judgesStage_.object->mainPosition.transform.rotate.x,
				0.01f);
			ImGui::DragFloat3("Scale##judgesStage",
				&judgesStage_.object->mainPosition.transform.scale.x,
				0.01f);
			ImGui::TreePop();
		}
	}

	if (judgesDesk_.object) {
		if (ImGui::TreeNode("JudgesDesk")) {
			ImGui::DragFloat3("Pos##judgesDesk",
				&judgesDesk_.object->mainPosition.transform.translate.x,
				0.1f);
			ImGui::DragFloat3("Rot##judgesDesk",
				&judgesDesk_.object->mainPosition.transform.rotate.x,
				0.01f);
			ImGui::DragFloat3("Scale##judgesDesk",
				&judgesDesk_.object->mainPosition.transform.scale.x,
				0.01f);
			ImGui::TreePop();
		}
	}

	for (int i = 0; i < (int)judges_.size(); ++i) {
		char label[32];
		snprintf(label, sizeof(label), "Judge %d", i);
		if (ImGui::TreeNode(label)) {
			ImGui::DragFloat3(
				"Pos", &judges_[i].object->mainPosition.transform.translate.x, 0.1f);
			ImGui::DragFloat3(
				"Rot", &judges_[i].object->mainPosition.transform.rotate.x, 0.01f);
			ImGui::DragFloat3(
				"Scale", &judges_[i].object->mainPosition.transform.scale.x, 0.01f);
			ImGui::TreePop();
		}
	}

	for (int i = 0; i < (int)judgesChairs_.size(); ++i) {
		char label[32];
		snprintf(label, sizeof(label), "JudgesChair %d", i);
		if (ImGui::TreeNode(label)) {
			ImGui::DragFloat3(
				"Pos", &judgesChairs_[i].object->mainPosition.transform.translate.x,
				0.1f);
			ImGui::DragFloat3(
				"Rot", &judgesChairs_[i].object->mainPosition.transform.rotate.x,
				0.01f);
			ImGui::DragFloat3(
				"Scale", &judgesChairs_[i].object->mainPosition.transform.scale.x,
				0.01f);
			ImGui::TreePop();
		}
	}

	for (int i = 0; i < (int)audienceChairsMid_.size(); ++i) {
		char label[32];
		snprintf(label, sizeof(label), "AudienceChairMid %d", i);
		if (ImGui::TreeNode(label)) {
			ImGui::DragFloat3(
				"Pos",
				&audienceChairsMid_[i].object->mainPosition.transform.translate.x,
				0.1f);
			ImGui::DragFloat3(
				"Rot", &audienceChairsMid_[i].object->mainPosition.transform.rotate.x,
				0.01f);
			ImGui::DragFloat3(
				"Scale",
				&audienceChairsMid_[i].object->mainPosition.transform.scale.x, 0.01f);
			ImGui::TreePop();
		}
	}

	for (int i = 0; i < (int)audienceChairsRightSide_.size(); ++i) {
		char label[32];
		snprintf(label, sizeof(label), "AudienceChairRightSide %d", i);
		if (ImGui::TreeNode(label)) {
			ImGui::DragFloat3("Pos",
				&audienceChairsRightSide_[i]
				.object->mainPosition.transform.translate.x,
				0.1f);
			ImGui::DragFloat3(
				"Rot",
				&audienceChairsRightSide_[i].object->mainPosition.transform.rotate.x,
				0.01f);
			ImGui::DragFloat3(
				"Scale",
				&audienceChairsRightSide_[i].object->mainPosition.transform.scale.x,
				0.01f);
			ImGui::TreePop();
		}
	}

	for (int i = 0; i < (int)audienceChairsLeftSide_.size(); ++i) {
		char label[32];
		snprintf(label, sizeof(label), "AudienceChairLeftSide %d", i);
		if (ImGui::TreeNode(label)) {
			ImGui::DragFloat3("Pos",
				&audienceChairsLeftSide_[i]
				.object->mainPosition.transform.translate.x,
				0.1f);
			ImGui::DragFloat3(
				"Rot",
				&audienceChairsLeftSide_[i].object->mainPosition.transform.rotate.x,
				0.01f);
			ImGui::DragFloat3(
				"Scale",
				&audienceChairsLeftSide_[i].object->mainPosition.transform.scale.x,
				0.01f);
			ImGui::TreePop();
		}
	}

	ImGui::End();
#endif

#ifdef USE_IMGUI
	ImGui::Begin("Camera Control");
	ImGui::Checkbox("Use Debug Camera", &useDebugCamera_);
	if (usingCamera_) {
		ImGui::Text("Pos: %.1f, %.1f, %.1f",
			usingCamera_->GetTransform().translate.x,
			usingCamera_->GetTransform().translate.y,
			usingCamera_->GetTransform().translate.z);
		ImGui::Text("Rot: %.2f, %.2f, %.2f", usingCamera_->GetTransform().rotate.x,
			usingCamera_->GetTransform().rotate.y,
			usingCamera_->GetTransform().rotate.z);
	}
	ImGui::End();
#endif

#ifdef USE_IMGUI
	// 現在シーン・フェーズ表示
	ImGui::Begin("Scene");
	ImGui::Text("ContestScene");

	const char* phaseNames[] = { "ShowOff", "Judging", "Result", "Suspense", "Ranking", "Advice", "Trophy" };
	ImGui::Text("Phase: %s", phaseNames[static_cast<int>(phase_)]);
	ImGui::End();
#endif

	if (contestVenue_.object) {
		contestVenue_.object->Update(usingCamera_);
		contestVenue_.object->Draw();
	}

	if (stage_.object) {
		stage_.object->Update(usingCamera_);
		stage_.object->Draw();
	}

	if (backScreen_.object) {
		backScreen_.object->Update(usingCamera_);
		backScreen_.object->Draw();
	}

	for (auto& rightScreen : rightSideScreens_) {
		if (rightScreen.object) {
			rightScreen.object->Update(usingCamera_);
			rightScreen.object->Draw();
		}
	}

	for (auto& leftScreen : leftSideScreens_) {
		if (leftScreen.object) {
			leftScreen.object->Update(usingCamera_);
			leftScreen.object->Draw();
		}
	}

	if (floor_.object) {
		floor_.object->Update(usingCamera_);
		floor_.object->Draw();
	}

	for (auto& judge : judges_) {
		if (judge.object) {
			judge.object->Update(usingCamera_);
			judge.object->Draw();
		}
	}

	if (judgesStage_.object) {
		judgesStage_.object->Update(usingCamera_);
		judgesStage_.object->Draw();
	}

	if (judgesDesk_.object) {
		judgesDesk_.object->Update(usingCamera_);
		judgesDesk_.object->Draw();
	}

	for (auto& chair : judgesChairs_) {
		if (chair.object) {
			chair.object->Update(usingCamera_);
			chair.object->Draw();
		}
	}

	customizedBodyActor_.UpdateAndDraw(usingCamera_);

	for (int i = 0; i < 2; ++i) {
		if (npcScoreCalculated_[i]) {
			npcBodyActors_[i].UpdateAndDraw(usingCamera_);
		}
	}

	// 現在のパートの描画
	if (currentPart_) {
		currentPart_->Draw();
	}

	// フェード描画
	fade_.Draw();
}

void ContestScene::SetupSceneObject(SceneObject& obj, int modelHandle,
	const Vector3& pos, const Vector3& rot,
	float scale) {
	obj.object = std::make_unique<Object>();
	obj.object->IntObject(system_);
	obj.object->CreateModelData(modelHandle);
	obj.object->mainPosition.transform = CreateDefaultTransform();
	obj.object->mainPosition.transform.translate = pos;
	obj.object->mainPosition.transform.rotate = rot;
	obj.object->mainPosition.transform.scale = { scale, scale, scale };
	obj.position = pos;
	obj.rotation = rot;
	obj.scale = { scale, scale, scale };
}

void ContestScene::AdvancePhase() {
	switch (phase_) {
	case ContestPhase::ShowOff:
		phase_ = ContestPhase::Judging;
		// 審査中は歓声を少し小さくする
		if (audienceSoundHandle_ != -1) {
			system_->SoundSetVolume(audienceSoundHandle_, 0.2f);
		}
		break;
	case ContestPhase::Judging:
		phase_ = ContestPhase::Result;
		// リザルト画面で歓声を少し戻す
		if (audienceSoundHandle_ != -1) {
			system_->SoundSetVolume(audienceSoundHandle_, 0.4f);
		}
		break;
	case ContestPhase::Result:
		phase_ = ContestPhase::Suspense;
		// サスペンス演出（ドラムロール等）が始まるため歓声をストップ
		if (!audienceStopped_ && audienceSoundHandle_ != -1) {
			system_->SoundStop(audienceSoundHandle_);
			audienceStopped_ = true;
		}
		break;
	case ContestPhase::Suspense:
		phase_ = ContestPhase::Ranking;
		// ランキング発表以降は再び歓声を鳴らす
		if (audienceStopped_ && audienceSoundHandle_ != -1) {
			system_->SoundPlayBGM(audienceSoundHandle_, 0.6f);
			audienceStopped_ = false;
		}
		break;
	case ContestPhase::Ranking:
		phase_ = ContestPhase::Advice;
		break;
	case ContestPhase::Advice:
		phase_ = ContestPhase::Trophy;
		break;
	case ContestPhase::Trophy:
		// Trophyは HandleTrophyChoice で処理するのでここには来ない
		break;
	}

	currentPart_ = CreatePart(phase_);
}

std::unique_ptr<IContestPart> ContestScene::CreatePart(ContestPhase phase) {
	Logger::Log("[ContestScene] font=%p size=%d", &bitmapFont_, bitmapFont_.GetGlyphMapSize());
	switch (phase) {
	case ContestPhase::ShowOff:
		return std::make_unique<ShowOffPart>(system_, &bitmapFont_,
			audienceResult_);

	case ContestPhase::Judging:
		return std::make_unique<JudgingPart>(system_, &bitmapFont_, scoreResult_,
			judgeCommentResults_);

	case ContestPhase::Result:
		return std::make_unique<ResultPart>(system_, &bitmapFont_, scoreResult_,
			earnedNickname_);

	case ContestPhase::Suspense: {
		// 3つのスポットライト：プレイヤー、NPC0、NPC1
		Light* lights[3] = { spotlight_, npcSpotlights_[0], npcSpotlights_[1] };
		// 対応する人物のワールド位置（プレイヤー x=0, NPC0 x=-1.0, NPC1 x=+1.0）
		Vector3 targets[3] = {
			{ 0.0f, 0.15f, -0.2f },
			{ -1.0f, 0.15f, -0.2f },
			{ 1.0f, 0.15f, -0.2f },
		};
		// rank==1 の全エントリをインデックス化（同率1位対応）
		std::vector<int> winners;
		if (!contestRanking_.empty()) {
			int topRank = contestRanking_[0].rank;
			for (const auto& e : contestRanking_) {
				if (e.rank != topRank) break;
				int idx = -1;
				if (e.name == "あなた" || e.name == "YOU") idx = 0;
				else if (e.name == "ライバル 1") idx = 1;
				else if (e.name == "ライバル 2") idx = 2;
				if (idx >= 0) winners.push_back(idx);
			}
		}
		if (winners.empty()) winners.push_back(0);
		
		ModCustomizedBodyActor* actors[3] = { &customizedBodyActor_, &npcBodyActors_[0], &npcBodyActors_[1] };
		return std::make_unique<SuspensePart>(system_, &bitmapFont_, lights, targets, winners, actors, light1_, usingCamera_);
	}

	case ContestPhase::Ranking:
		return std::make_unique<RankingPart>(system_, &bitmapFont_, contestRanking_, customizedBodyActor_.GetHeadWorldPosition());

	case ContestPhase::Advice:
		return std::make_unique<AdvicePart>(system_, &bitmapFont_, customizedBodyActor_.GetHeadWorldPosition());

	case ContestPhase::Trophy:
		return std::make_unique<TrophyPart>(system_, &bitmapFont_, customizedBodyActor_.GetHeadWorldPosition());

	default:
		return nullptr;
	}
}

void ContestScene::HandleTrophyChoice() {
	// TrophyPartにダウンキャストして選択結果を取得
	auto* trophyPart = dynamic_cast<TrophyPart*>(currentPart_.get());
	if (!trophyPart)
		return;

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

		if (currentPart_) {
			PartCameraTransform target = currentPart_->GetCameraTransform();

			// ターゲットが変わったら補間開始
			if (target.position.x != cameraTarget_.position.x ||
				target.position.y != cameraTarget_.position.y ||
				target.position.z != cameraTarget_.position.z ||
				target.rotation.x != cameraTarget_.rotation.x ||
				target.rotation.y != cameraTarget_.rotation.y ||
				target.rotation.z != cameraTarget_.rotation.z) {

				cameraTarget_ = target;
				
				if (currentPart_->UseCustomCameraControl()) {
					cameraCurrent_ = target;
					isCameraLerping_ = false;
				} else {
					cameraLerpTimer_ = 0.0f;
					isCameraLerping_ = true;
				}
			}

			if (isCameraLerping_) {
				cameraLerpTimer_ += 1.0f / 60.0f;
				float t = cameraLerpTimer_ / cameraLerpDuration_;
				if (t >= 1.0f) {
					t = 1.0f;
					isCameraLerping_ = false;
				}

				// 線形補間
				cameraCurrent_.position.x =
					cameraCurrent_.position.x +
					(cameraTarget_.position.x - cameraCurrent_.position.x) * t;
				cameraCurrent_.position.y =
					cameraCurrent_.position.y +
					(cameraTarget_.position.y - cameraCurrent_.position.y) * t;
				cameraCurrent_.position.z =
					cameraCurrent_.position.z +
					(cameraTarget_.position.z - cameraCurrent_.position.z) * t;
				cameraCurrent_.rotation.x =
					cameraCurrent_.rotation.x +
					(cameraTarget_.rotation.x - cameraCurrent_.rotation.x) * t;
				cameraCurrent_.rotation.y =
					cameraCurrent_.rotation.y +
					(cameraTarget_.rotation.y - cameraCurrent_.rotation.y) * t;
				cameraCurrent_.rotation.z =
					cameraCurrent_.rotation.z +
					(cameraTarget_.rotation.z - cameraCurrent_.rotation.z) * t;
			}

			camera_->SetTranslate(cameraCurrent_.position);
			camera_->SetRotation(cameraCurrent_.rotation);

			usingCamera_ = camera_;
		}
	}

	system_->SetCamera(usingCamera_);
}