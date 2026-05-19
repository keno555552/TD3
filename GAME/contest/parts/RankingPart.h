#pragma once
#include "IContestPart.h"
#include "GAME/Object/DetailButton/DetailButton.h"
#include <vector>
#include <string>

struct ContestRankEntry {
    std::string name;       // 表示名
    int totalStars;         // ★合計
    std::string overallRank; // 総合ランク
    float finalScore;       // 最終スコア
    int rank;               // 最終順位 (1〜3)
};

class RankingPart : public IContestPart {
public:
    RankingPart(kEngine* system, BitmapFont* font,
        const std::vector<ContestRankEntry>& entries);
    ~RankingPart() override = default;

    void Update() override;
    void Draw() override;
    bool IsFinished() const override;
    PartCameraTransform GetCameraTransform() const override;

private:
    std::unique_ptr<DetailButton> nextButton_;
    std::vector<ContestRankEntry> entries_; // ソート済み
    bool isFinished_ = false;
    PartCameraTransform cameraTransform_;
};
