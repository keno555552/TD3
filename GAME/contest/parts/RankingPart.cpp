#include "RankingPart.h"
#include "kEngine.h"
#include "GAME/font/BitmapFont.h"
#include <algorithm>

RankingPart::RankingPart(kEngine* system, BitmapFont* font,
    const std::vector<ContestRankEntry>& entries)
    : IContestPart(system, font), entries_(entries) {

    cameraTransform_ = { { 0.0f, 1.2f, -3.0f }, { 0.12f, 0.0f, 0.0f } };

    nextButton_ = std::make_unique<DetailButton>(system);
    nextButton_->SetButton({ 640.0f, 650.0f }, 400.0f, 80.0f);
}

void RankingPart::Update() {
    if (isFinished_) return;

    nextButton_->Update();

    if (system_->GetTriggerOn(DIK_SPACE) || nextButton_->GetIsRelease()) {
        isFinished_ = true;
    }
}

void RankingPart::Draw() {
    font_->RenderText("Final Ranking", { 640.0f, 100.0f }, 64.0f, BitmapFont::Align::Center, 5, { 1.0f, 1.0f, 0.0f, 1.0f });

    float startY = 250.0f;
    float stepY = 100.0f;

    for (size_t i = 0; i < entries_.size(); ++i) {
        const auto& entry = entries_[i];
        
        Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
        if (entry.name == "あなた" || entry.name == "YOU") { // プレイヤー強調用
            color = { 0.2f, 1.0f, 0.2f, 1.0f };
        }

        std::string rankStr = std::to_string(entry.rank) + "位: ";
        std::string text = rankStr + entry.name + " / ランク " + entry.overallRank + " / ★ " + std::to_string(entry.totalStars);

        font_->RenderText(text, { 640.0f, startY + i * stepY }, 48.0f, BitmapFont::Align::Center, 5, color);
    }

    nextButton_->Render();
    font_->RenderText("To Next", { 640.0f, 620.0f }, 48.0f, BitmapFont::Align::Center, 5, { 1.0f, 1.0f, 0.0f, 1.0f });

#ifdef USE_IMGUI
    ImGui::Begin("Contest - Ranking");
    ImGui::Text("[Final Ranking]");
    for (const auto& e : entries_) {
        ImGui::Text("%d Rank: %s (Score: %.1f, Stars: %d)", e.rank, e.name.c_str(), e.finalScore, e.totalStars);
    }
    ImGui::End();
#endif
}

bool RankingPart::IsFinished() const {
    return isFinished_;
}

PartCameraTransform RankingPart::GetCameraTransform() const {
    return cameraTransform_;
}
