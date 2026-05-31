#include "RankingPart.h"
#include "kEngine.h"
#include "GAME/font/BitmapFont.h"
#include <algorithm>

namespace {
// 黒アウトライン付きテキスト描画。
// 同じサイズの黒を周囲8方向に少しずつズラして先に描き、その上に本来の色を重ねる。
// 各文字の送り幅は同一なので文字ごとにきれいに揃った縁取りになる。
// （深度関数がLESS_EQUALなので、後から同レイヤーで描く本体が黒の上に乗る）
void RenderTextOutlined(BitmapFont* font, const std::string& text,
    Vector2 position, float heightPx, BitmapFont::Align align,
    float layer, Vector4 color) {
    const float d = heightPx * 0.05f; // 縁の太さ（オフセット量）
    const Vector4 black = { 0.0f, 0.0f, 0.0f, 1.0f };
    const Vector2 offsets[8] = {
        { -d, -d }, { 0.0f, -d }, { d, -d },
        { -d, 0.0f },             { d, 0.0f },
        { -d,  d }, { 0.0f,  d }, { d,  d },
    };
    for (const auto& o : offsets) {
        font->RenderText(text, { position.x + o.x, position.y + o.y },
            heightPx, align, layer, black);
    }
    font->RenderText(text, position, heightPx, align, layer, color);
}
} // namespace

RankingPart::RankingPart(kEngine* system, BitmapFont* font,
    const std::vector<ContestRankEntry>& entries,
    Vector3 playerTarget)
    : IContestPart(system, font), entries_(entries) {

    cameraTransform_ = { { playerTarget.x, playerTarget.y, playerTarget.z - 1.5f }, { 0.0f, 0.0f, 0.0f } };

    nextButton_ = std::make_unique<DetailButton>(system);
    nextButton_->SetButton({ 640.0f, 650.0f }, 400.0f, 80.0f);

    decideSoundHandle_ = system_->SoundLoadSE("GAME/resources/sounds/decide.mp3");
}

RankingPart::~RankingPart() {
}

void RankingPart::Update() {
    if (isFinished_) return;

    nextButton_->Update();

    if (system_->GetTriggerOn(DIK_SPACE) || nextButton_->GetIsRelease()) {
        if (system_->GetTriggerOn(DIK_SPACE) && decideSoundHandle_ != -1) {
            system_->SoundPlaySE(decideSoundHandle_, 0.5f);
        }
        isFinished_ = true;
    }
}

void RankingPart::Draw() {
    RenderTextOutlined(font_, "Final Ranking", { 640.0f, 100.0f }, 64.0f, BitmapFont::Align::Center, 5, { 1.0f, 1.0f, 0.0f, 1.0f });

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

        RenderTextOutlined(font_, text, { 640.0f, startY + i * stepY }, 48.0f, BitmapFont::Align::Center, 5, color);
    }

    nextButton_->Render();
    RenderTextOutlined(font_, "To Next", { 640.0f, 620.0f }, 48.0f, BitmapFont::Align::Center, 5, { 1.0f, 1.0f, 0.0f, 1.0f });

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
