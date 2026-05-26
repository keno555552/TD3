#pragma once
#include "IContestPart.h"
#include "GAME/Object/DetailButton/DetailButton.h"
#include "GAME/actor/ModBody.h"
#include "GAME/theme/ThemeData.h"
#include <vector>
#include <string>

class AdvicePart : public IContestPart {
public:
    AdvicePart(kEngine* system, BitmapFont* font);
    ~AdvicePart() override = default;

    void Update() override;
    void Draw() override;
    bool IsFinished() const override;
    PartCameraTransform GetCameraTransform() const override;

private:
    std::unique_ptr<DetailButton> nextButton_;
    bool isFinished_ = false;
    PartCameraTransform cameraTransform_;

    std::vector<std::string> adviceTexts_;

    void GenerateAdvice();
    std::string GetPartNameJapanese(ModBodyPart part) const;
};
