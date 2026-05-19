#include "ShowOffPart.h"
#include "kEngine.h"

ShowOffPart::ShowOffPart(kEngine* system, BitmapFont* font,
	const AudienceResult& audienceResult)
	: IContestPart(system, font), audienceResult_(audienceResult) {

	// 初期カメラ位置（後でImGuiで調整）
	cameraTransform_.position = { 0.0f, 3.5f, -13.0f };
	cameraTransform_.rotation = { 0.15f, 0.0f, 0.0f };

	// 乱数生成器の初期化
	std::random_device rd;
	rng_.seed(rd());

	GenerateScrollingComments();

	nextButton_ = std::make_unique<DetailButton>(system);
	nextButton_->SetButton({ 640.0f, 650.0f }, 400.0f, 80.0f);
}

void ShowOffPart::Update() {
	if (isFinished_) return;

	nextButton_->Update();

	// === スクロール更新 === //
	float dt = system_->GetDeltaTime();

	// ざわのスクロール更新
	for (auto& sc : zawaScrolls_) {
		sc.position.x -= sc.speed * dt;

		// 画面左端を超えたら右端からリスタート（ループ）
		if (sc.position.x < -kSpawnMargin) {
			ResetScrollPosition(sc, rng_);
		}
	}

	// コメントのスクロール更新
	for (auto& sc : commentScrolls_) {
		sc.position.x -= sc.speed * dt;

		// 画面左端を超えたら右端からリスタート（ループ）
		if (sc.position.x < -kSpawnMargin) {
			ResetScrollPosition(sc, rng_);
		}
	}

	switch (step_) {
	case ShowOffStep::StageView:
		// SPACEで審査員側へカメラ移動
		if (system_->GetTriggerOn(DIK_SPACE)) {
			step_ = ShowOffStep::TurnToJudges;
		}

		if(nextButton_->GetIsRelease()){
			step_ = ShowOffStep::TurnToJudges;
		}
		break;

	case ShowOffStep::TurnToJudges:
		// TODO: カメラ移動完了判定（今は即完了）
		isFinished_ = true;
		break;
	}
}

void ShowOffPart::Draw() {

	// ざわをスクロール表示（奥側：layer=6）
	for (const auto& sc : zawaScrolls_) {
		font_->RenderText("ざわ・・・",
			{ sc.position.x, sc.yPos }, sc.size,
			BitmapFont::Align::Center, 6.0f);
	}

	// 観客コメントをスクロール表示（手前側：layer=5）
	for (int i = 0; i < 3; ++i) {
		if (i < (int)commentScrolls_.size() &&
			!audienceResult_.comments[i].empty()) {
			font_->RenderText(
				audienceResult_.comments[i],
				{ commentScrolls_[i].position.x, commentScrolls_[i].yPos },
				commentScrolls_[i].size,
				BitmapFont::Align::Center,
				5.0f, { 1.0f,0.0f,0.0f,1.0f });
		}
	}

	nextButton_->Render();

	font_->RenderText(
		"To Judge",
		{ 640.0f, 620.0f }, 48.0f,
		BitmapFont::Align::Center, 5, { 1.0f,1.0f,0.0f,1.0f });

#ifdef USE_IMGUI
	ImGui::Begin("Contest - ShowOff");

	ImGui::Text("[ShowOff] %s",
		step_ == ShowOffStep::StageView ? "Stage View" : "Turn To Judges");

	ImGui::Separator();
	for (int i = 0; i < 3; ++i) {
		ImGui::Text("  Audience %d: %s", i + 1,
			audienceResult_.comments[i].c_str());
	}

	ImGui::Separator();
	ImGui::Text("Camera:");
	ImGui::DragFloat3("Pos##showoff", &cameraTransform_.position.x, 0.1f);
	ImGui::DragFloat3("Rot##showoff", &cameraTransform_.rotation.x, 0.01f);

	ImGui::Spacing();
	ImGui::Text("Press SPACE to continue");

	ImGui::End();
#endif
}

bool ShowOffPart::IsFinished() const {
	return isFinished_;
}

PartCameraTransform ShowOffPart::GetCameraTransform() const {
	return cameraTransform_;
}

void ShowOffPart::GenerateScrollingComments() {
	std::uniform_real_distribution<float> distY(50.0f, 550.0f);
	std::uniform_real_distribution<float> distSpeed(100.0f, 300.0f);
	std::uniform_real_distribution<float> distStartX(kScreenWidth, kScreenWidth + 600.0f);
	std::uniform_int_distribution<int> distSize(0, 2);

	float sizes[] = { 32.0f, 48.0f, 64.0f };

	// ざわ8個（初期X位置をばらけさせて一斉に出ないようにする）
	zawaScrolls_.resize(8);
	for (auto& sc : zawaScrolls_) {
		sc.position.x = distStartX(rng_);
		sc.yPos = distY(rng_);
		sc.size = sizes[distSize(rng_)];
		sc.speed = distSpeed(rng_);
	}

	// コメント3個
	commentScrolls_.resize(3);
	for (auto& sc : commentScrolls_) {
		sc.position.x = distStartX(rng_);
		sc.yPos = distY(rng_);
		sc.size = sizes[distSize(rng_)];
		sc.speed = distSpeed(rng_);
	}
}

void ShowOffPart::ResetScrollPosition(ScrollingComment& sc, std::mt19937& gen) {
	std::uniform_real_distribution<float> distY(50.0f, 550.0f);
	std::uniform_real_distribution<float> distSpeed(100.0f, 300.0f);
	std::uniform_real_distribution<float> distStartX(kScreenWidth, kScreenWidth + 400.0f);

	// 右端外からリスタート、Y座標と速度も再ランダム
	sc.position.x = distStartX(gen);
	sc.yPos = distY(gen);
	sc.speed = distSpeed(gen);
}