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
	camera_->SetTranslate({ 0.0f, 3.5f, -13.0f });
	camera_->SetRotation({ 0.15f, 0.0f, 0.0f }); 
	usingCamera_ = camera_;
	system_->SetCamera(usingCamera_);

	fade_.Initialize(system_);
	fade_.StartFadeIn();

	float objectScale = 0.2f;
	float PI = 3.14159265f;

	//==================
	// モデル
	//==================
	// ステージオブジェクト

    // ステージ
	stageModelHandle_ = system_->SetModelObj("GAME/resources/ContestStageObject/stage.obj");
	SetupSceneObject(stage_, stageModelHandle_,
		{ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, objectScale);

	// バックスクリーン
	backScreenModelHandle_ = system_->SetModelObj("GAME/resources/ContestStageObject/backScreen.obj");
	SetupSceneObject(backScreen_, backScreenModelHandle_,
		{ 0.0f, 0.2f, 0.8f }, { 0.0f, PI, 0.0f }, objectScale);

	// 右サイドスクリーン
	rightSideScreenModelHandle_ = system_->SetModelObj("GAME/resources/ContestStageObject/RightSideScreen.obj");
	rightSideScreens_.resize(2);
	SetupSceneObject(rightSideScreens_[0], rightSideScreenModelHandle_,
		{ 1.4f, 0.2f, 0.3f }, { 0.0f, PI, 0.0f }, objectScale);   // 右1
	SetupSceneObject(rightSideScreens_[1], rightSideScreenModelHandle_,
		{ 1.6f, 0.2f, -0.5f }, { 0.0f, PI, 0.0f }, objectScale);   // 右2

	// 左サイドスクリーン
	leftSideScreenModelHandle_ = system_->SetModelObj("GAME/resources/ContestStageObject/LeftSideScreen.obj");
	leftSideScreens_.resize(2);
	SetupSceneObject(leftSideScreens_[0], leftSideScreenModelHandle_,
		{ -1.4f, 0.2f, 0.3f }, { 0.0f, PI, 0.0f }, objectScale);   // 左1
	SetupSceneObject(leftSideScreens_[1], leftSideScreenModelHandle_,
		{ -1.6f, 0.2f, -0.5f }, { 0.0f, PI, 0.0f }, objectScale);   // 左2

	// 床
	floorModelHandle_ = system_->SetModelObj("GAME/resources/ContestStageObject/floor.obj");
	SetupSceneObject(floor_, floorModelHandle_,
		{ 0.0f, 0.0f, -5.0f }, { 0.0f, PI, 0.0f }, objectScale);

	// 審査員用ステージ
	judgesStageModelHandle_ = system_->SetModelObj("GAME/resources/ContestStageObject/judgesStage.obj");
	SetupSceneObject(judgesStage_, judgesStageModelHandle_,
		{ 0.0f, 0.0f, -2.0f }, { 0.0f, 0.0f, 0.0f }, objectScale);

	// 審査員用机
	judgesDeskModelHandle_ = system_->SetModelObj("GAME/resources/ContestStageObject/judgesDesk.obj");
	SetupSceneObject(judgesDesk_, judgesDeskModelHandle_,
		{ 0.0f, 0.2f, -1.7f }, { 0.0f, PI, 0.0f }, objectScale);

	// 審査員用椅子
	judgesChairModelHandle_ = system_->SetModelObj("GAME/resources/ContestStageObject/judgesChair.obj");
	judgesChairs_.resize(3);
	SetupSceneObject(judgesChairs_[0], judgesChairModelHandle_,
		{ -0.25f, 0.195f, -2.0f }, { 0.0f, PI, 0.0f }, objectScale);   // 審査員席1
	SetupSceneObject(judgesChairs_[1], judgesChairModelHandle_,
		{ 0.0f, 0.195f, -2.0f }, { 0.0f, PI, 0.0f }, objectScale);   // 審査員席2
	SetupSceneObject(judgesChairs_[2], judgesChairModelHandle_,
		{ 0.25f, 0.195f, -2.0f }, { 0.0f, PI, 0.0f }, objectScale);   // 審査員席3

	// 観客用椅子（中央）
	audienceChairsMidModelHandle_ = system_->SetModelObj("GAME/resources/ContestStageObject/audienceChairMid.obj");
	audienceChairsMid_.resize(9);
	for (int i = 0; i < 9; ++i) {
		float zPos = -3.0f - (i * 0.4f);
		SetupSceneObject(audienceChairsMid_[i], audienceChairsMidModelHandle_,
			{ 0.0f, 0.0f, zPos }, { 0.0f, PI, 0.0f }, objectScale);
	}

	// 観客用椅子（右側）
	audienceChairsSideModelHandle_ = system_->SetModelObj("GAME/resources/ContestStageObject/audienceChairSide.obj");
	audienceChairsRightSide_.resize(9);
	audienceChairsLeftSide_.resize(9);
	for (int i = 0; i < 9; ++i) {
		float zPos = -2.65f - (i * 0.4f);
		SetupSceneObject(audienceChairsRightSide_[i], audienceChairsSideModelHandle_,
			{ 2.0f, 0.0f, zPos }, { 0.0f, PI-0.35f, 0.0f }, objectScale);   // 右側
		SetupSceneObject(audienceChairsLeftSide_[i], audienceChairsSideModelHandle_,
			{ -2.0f, 0.0f, zPos }, { 0.0f, PI+0.35f, 0.0f }, objectScale);   // 左側
	}

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
	// ステージオブジェクト配置調整
	ImGui::Begin("Stage Objects");

	if (stage_.object) {
		if (ImGui::TreeNode("Stage")) {
			ImGui::DragFloat3("Pos##stage",
				&stage_.object->mainPosition.transform.translate.x, 0.1f);
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
				&backScreen_.object->mainPosition.transform.translate.x, 0.1f);
			ImGui::DragFloat3("Rot##back",
				&backScreen_.object->mainPosition.transform.rotate.x, 0.01f);
			ImGui::DragFloat3("Scale##back",
				&backScreen_.object->mainPosition.transform.scale.x, 0.01f);
			ImGui::TreePop();
		}
	}

	for (int i = 0; i < (int)rightSideScreens_.size(); ++i) {
		char label[32];
		snprintf(label, sizeof(label), "RightSideScreen %d", i);
		if (ImGui::TreeNode(label)) {
			ImGui::DragFloat3("Pos", &rightSideScreens_[i].object->mainPosition.transform.translate.x, 0.1f);
			ImGui::DragFloat3("Rot", &rightSideScreens_[i].object->mainPosition.transform.rotate.x, 0.01f);
			ImGui::TreePop();
		}
	}

	for (int i = 0; i < (int)leftSideScreens_.size(); ++i) {
		char label[32];
		snprintf(label, sizeof(label), "LefttSideScreen %d", i);
		if (ImGui::TreeNode(label)) {
			ImGui::DragFloat3("Pos", &leftSideScreens_[i].object->mainPosition.transform.translate.x, 0.1f);
			ImGui::DragFloat3("Rot", &leftSideScreens_[i].object->mainPosition.transform.rotate.x, 0.01f);
			ImGui::TreePop();
		}
	}

	if (floor_.object) {
		if (ImGui::TreeNode("Floor")) {
			ImGui::DragFloat3("Pos##floor",
				&floor_.object->mainPosition.transform.translate.x, 0.1f);
			ImGui::DragFloat3("Rot##floor",
				&floor_.object->mainPosition.transform.rotate.x, 0.01f);
			ImGui::DragFloat3("Scale##floor",
				&floor_.object->mainPosition.transform.scale.x, 0.01f);
			ImGui::TreePop();
		}
	}

	if(judgesStage_.object) {
		if (ImGui::TreeNode("JudgesStage")) {
			ImGui::DragFloat3("Pos##judgesStage",
				&judgesStage_.object->mainPosition.transform.translate.x, 0.1f);
			ImGui::DragFloat3("Rot##judgesStage",
				&judgesStage_.object->mainPosition.transform.rotate.x, 0.01f);
			ImGui::DragFloat3("Scale##judgesStage",
				&judgesStage_.object->mainPosition.transform.scale.x, 0.01f);
			ImGui::TreePop();
		}
	}

	if(judgesDesk_.object) {
		if (ImGui::TreeNode("JudgesDesk")) {
			ImGui::DragFloat3("Pos##judgesDesk",
				&judgesDesk_.object->mainPosition.transform.translate.x, 0.1f);
			ImGui::DragFloat3("Rot##judgesDesk",
				&judgesDesk_.object->mainPosition.transform.rotate.x, 0.01f);
			ImGui::DragFloat3("Scale##judgesDesk",
				&judgesDesk_.object->mainPosition.transform.scale.x, 0.01f);
			ImGui::TreePop();
		}
	}

	for (int i = 0; i < (int)judgesChairs_.size(); ++i) {
		char label[32];
		snprintf(label, sizeof(label), "JudgesChair %d", i);
		if (ImGui::TreeNode(label)) {
			ImGui::DragFloat3("Pos", &judgesChairs_[i].object->mainPosition.transform.translate.x, 0.1f);
			ImGui::DragFloat3("Rot", &judgesChairs_[i].object->mainPosition.transform.rotate.x, 0.01f);
			ImGui::DragFloat3("Scale", &judgesChairs_[i].object->mainPosition.transform.scale.x, 0.01f);
			ImGui::TreePop();
		}
	}

	for(int i = 0; i < (int)audienceChairsMid_.size(); ++i) {
		char label[32];
		snprintf(label, sizeof(label), "AudienceChairMid %d", i);
		if (ImGui::TreeNode(label)) {
			ImGui::DragFloat3("Pos", &audienceChairsMid_[i].object->mainPosition.transform.translate.x, 0.1f);
			ImGui::DragFloat3("Rot", &audienceChairsMid_[i].object->mainPosition.transform.rotate.x, 0.01f);
			ImGui::DragFloat3("Scale", &audienceChairsMid_[i].object->mainPosition.transform.scale.x, 0.01f);
			ImGui::TreePop();
		}
	}

	for(int i = 0; i < (int)audienceChairsRightSide_.size(); ++i) {
		char label[32];
		snprintf(label, sizeof(label), "AudienceChairRightSide %d", i);
		if (ImGui::TreeNode(label)) {
			ImGui::DragFloat3("Pos", &audienceChairsRightSide_[i].object->mainPosition.transform.translate.x, 0.1f);
			ImGui::DragFloat3("Rot", &audienceChairsRightSide_[i].object->mainPosition.transform.rotate.x, 0.01f);
			ImGui::DragFloat3("Scale", &audienceChairsRightSide_[i].object->mainPosition.transform.scale.x, 0.01f);
			ImGui::TreePop();
		}
	}

	for(int i = 0; i < (int)audienceChairsLeftSide_.size(); ++i) {
		char label[32];
		snprintf(label, sizeof(label), "AudienceChairLeftSide %d", i);
		if (ImGui::TreeNode(label)) {
			ImGui::DragFloat3("Pos", &audienceChairsLeftSide_[i].object->mainPosition.transform.translate.x, 0.1f);
			ImGui::DragFloat3("Rot", &audienceChairsLeftSide_[i].object->mainPosition.transform.rotate.x, 0.01f);
			ImGui::DragFloat3("Scale", &audienceChairsLeftSide_[i].object->mainPosition.transform.scale.x, 0.01f);
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
		ImGui::Text("Rot: %.2f, %.2f, %.2f",
			usingCamera_->GetTransform().rotate.x,
			usingCamera_->GetTransform().rotate.y,
			usingCamera_->GetTransform().rotate.z);
	}
	ImGui::End();
#endif

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

	if(floor_.object) {
		floor_.object->Update(usingCamera_);
		floor_.object->Draw();
	}

	if(judgesStage_.object) {
		judgesStage_.object->Update(usingCamera_);
		judgesStage_.object->Draw();
	}

	if(judgesDesk_.object) {
		judgesDesk_.object->Update(usingCamera_);
		judgesDesk_.object->Draw();
	}

	for (auto& chair : judgesChairs_) {
		if (chair.object) {
			chair.object->Update(usingCamera_);
			chair.object->Draw();
		}
	}

	for (auto& chair : audienceChairsMid_) {
		if (chair.object) {
			chair.object->Update(usingCamera_);
			chair.object->Draw();
		}
	}

	for (auto& chair : audienceChairsRightSide_) {
		if (chair.object) {
			chair.object->Update(usingCamera_);
			chair.object->Draw();
		}
	}

	for (auto& chair : audienceChairsLeftSide_) {
		if (chair.object) {
			chair.object->Update(usingCamera_);
			chair.object->Draw();
		}
	}

	// 現在のパートの描画
	if (currentPart_) {
		currentPart_->Draw();
	}

	// フェード描画
	fade_.Draw();
}

void ContestScene::SetupSceneObject(SceneObject& obj, int modelHandle, const Vector3& pos, const Vector3& rot, float scale)
{
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

		// パートからカメラ設定を取得して反映
		if (currentPart_) {
			auto camTf = currentPart_->GetCameraTransform();
			camera_->SetTranslate(camTf.position);
			camera_->SetRotation(camTf.rotation);
		}

		usingCamera_ = camera_;
	}

	system_->SetCamera(usingCamera_);
}