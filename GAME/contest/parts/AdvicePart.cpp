#include "AdvicePart.h"
#include "kEngine.h"
#include "GAME/font/BitmapFont.h"
#include "GAME/actor/prompt/PromptData.h"
#include "GAME/score/ScoreCalculator.h"
#include <algorithm>
#include <cmath>
#include <map>

AdvicePart::AdvicePart(kEngine* system, BitmapFont* font, Vector3 playerTarget)
    : IContestPart(system, font) {
    cameraTransform_ = { { playerTarget.x, playerTarget.y, playerTarget.z - 1.5f }, { 0.0f, 0.0f, 0.0f } };

    nextButton_ = std::make_unique<DetailButton>(system);
    nextButton_->SetButton({ 640.0f, 650.0f }, 400.0f, 80.0f);

    decideSoundHandle_ = system_->SoundLoadSE("GAME/resources/sounds/decide.mp3");

    GenerateAdvice();
}

AdvicePart::~AdvicePart() {
}

void AdvicePart::Update() {
    if (isFinished_) return;

    nextButton_->Update();

    if (system_->GetTriggerOn(DIK_SPACE) || nextButton_->GetIsRelease()) {
        if (system_->GetTriggerOn(DIK_SPACE) && decideSoundHandle_ != -1) {
            system_->SoundPlaySE(decideSoundHandle_, 0.5f);
        }
        isFinished_ = true;
    }
}

void AdvicePart::Draw() {
    font_->RenderText("アドバイス", { 640.0f, 100.0f }, 64.0f, BitmapFont::Align::Center, 5, { 1.0f, 0.6f, 0.2f, 1.0f });

    float startY = 250.0f;
    float stepY = 80.0f;

    if (adviceTexts_.empty()) {
        font_->RenderText("完璧なカスタマイズだ！", { 640.0f, startY }, 48.0f, BitmapFont::Align::Center, 5, { 1.0f, 1.0f, 1.0f, 1.0f });
    } else {
        for (size_t i = 0; i < adviceTexts_.size(); ++i) {
            font_->RenderText(adviceTexts_[i], { 640.0f, startY + i * stepY }, 40.0f, BitmapFont::Align::Center, 5, { 1.0f, 1.0f, 1.0f, 1.0f });
        }
    }

    nextButton_->Render();
    font_->RenderText("次へ", { 640.0f, 620.0f }, 48.0f, BitmapFont::Align::Center, 5, { 1.0f, 1.0f, 0.0f, 1.0f });
}

bool AdvicePart::IsFinished() const {
    return isFinished_;
}

PartCameraTransform AdvicePart::GetCameraTransform() const {
    return cameraTransform_;
}

std::string AdvicePart::GetPartNameJapanese(ModBodyPart part) const {
    switch (part) {
        case ModBodyPart::ChestBody: return "胸部";
        case ModBodyPart::StomachBody: return "腹部";
        case ModBodyPart::Neck: return "首";
        case ModBodyPart::Head: return "頭";
        case ModBodyPart::LeftUpperArm: return "左二の腕";
        case ModBodyPart::LeftForeArm: return "左前腕";
        case ModBodyPart::RightUpperArm: return "右二の腕";
        case ModBodyPart::RightForeArm: return "右前腕";
        case ModBodyPart::LeftThigh: return "左太もも";
        case ModBodyPart::LeftShin: return "左すね";
        case ModBodyPart::RightThigh: return "右太もも";
        case ModBodyPart::RightShin: return "右すね";
        default: return "未知の部位";
    }
}

void AdvicePart::GenerateAdvice() {
    const ThemeData* theme = PromptData::GetThemeData();
    const ModBodyCustomizeData* playerData = ModBody::GetSharedCustomizeData();

    if (!theme || !playerData) {
        return;
    }

    if (theme->scoreSets.empty()) {
        return;
    }

    const ScoreSet& scoreSet = theme->scoreSets[0];

    struct PartWeightInfo {
        ModBodyPart part;
        float totalWeight;
        float maxX;
        float maxY;
        float maxZ;
        float maxWeightVal; // 最大の影響を持つ軸の値（正負）
        std::string dominantAxis;
    };

    std::vector<PartWeightInfo> weightInfos;

    for (int i = 0; i < 12; ++i) {
        ModBodyPart part = static_cast<ModBodyPart>(i);
        const PartWeight& pw = scoreSet.partWeights[i];
        
        float ax = std::abs(pw.scaleX);
        float ay = std::abs(pw.scaleY);
        float az = std::abs(pw.scaleZ);
        float total = ax + ay + az;

        if (total > 0.0f) {
            PartWeightInfo info;
            info.part = part;
            info.totalWeight = total;
            info.maxX = pw.scaleX;
            info.maxY = pw.scaleY;
            info.maxZ = pw.scaleZ;
            
            if (ax >= ay && ax >= az) {
                info.dominantAxis = "幅";
                info.maxWeightVal = pw.scaleX;
            } else if (ay >= ax && ay >= az) {
                info.dominantAxis = "長さ";
                info.maxWeightVal = pw.scaleY;
            } else {
                info.dominantAxis = "厚み";
                info.maxWeightVal = pw.scaleZ;
            }

            weightInfos.push_back(info);
        }
    }

    // 重みが大きい順にソート
    std::sort(weightInfos.begin(), weightInfos.end(), [](const PartWeightInfo& a, const PartWeightInfo& b) {
        return a.totalWeight > b.totalWeight;
    });

    int generatedCount = 0;
    const int MAX_ADVICE = 2; // 表示する最大アドバイス数

    for (const auto& info : weightInfos) {
        if (generatedCount >= MAX_ADVICE) break;

        // プレイヤーが部位を改造しているかチェック
        // CPからの変更量を取得 (0に近いと変更なし)
        float changeAmount = ScoreCalculator::GetPartChangeAmountFromCP(info.part, *playerData);
        
        // 変更量が少ない（閾値未満）ならアドバイス対象
        if (changeAmount < 0.2f) {
            std::string partName = GetPartNameJapanese(info.part);
            std::string adviceStr = partName + "の" + info.dominantAxis + "を";
            
            if (info.maxWeightVal > 0) {
                adviceStr += "もっと大きくすると高得点が狙えるぞ！";
            } else {
                adviceStr += "もっと小さくすると高得点が狙えるぞ！";
            }
            
            adviceTexts_.push_back(adviceStr);
            generatedCount++;
        }
    }
}
