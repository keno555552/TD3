#include "ContestScene.h"

namespace {

    /// ★を文字列に変換（例：3 → "★★★☆☆"）
    std::string StarsToString(int stars) {
        std::string result;
        for (int i = 0; i < 5; ++i) {
            result += (i < stars) ? "*" : "-";
        }
        return result;
    }

} // namespace

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
}

ContestScene::~ContestScene() {
    system_->DestroyCamera(camera_);
    system_->DestroyCamera(debugCamera_);
    system_->RemoveLight(light1_);

    delete light1_;
}

void ContestScene::Update() {
    CameraPart();

    // スペースキーでお題発表シーンへ
    if (!fade_.IsBusy() && system_->GetTriggerOn(DIK_SPACE)) {
        fade_.StartFadeOut();
        isStartTransition_ = true;
        nextOutcome_ = SceneOutcome::NEXT;
    }

    // タイトルへ戻る
    if (!fade_.IsBusy() && system_->GetTriggerOn(DIK_1)) {
        fade_.StartFadeOut();
        isStartTransition_ = true;
        nextOutcome_ = SceneOutcome::RETURN;
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
    // 現在シーン表示
    ImGui::Begin("Scene");
    ImGui::Text("ContestScene");
    ImGui::End();

    // スコア結果ウィンドウ
    ImGui::Begin("Contest Result");

    const ThemeData* theme = PromptData::GetThemeData();
    if (theme != nullptr) {
        ImGui::Text("Theme: %s", theme->themeId.c_str());
    }

    ImGui::Separator();

    if (!isScoreCalculated_) {
        ImGui::Text("Score not calculated");
    } else {
        // 総合ランク
        ImGui::Text("Overall Rank: %s", scoreResult_.overallRank.c_str());
        ImGui::Text("Total Stars: %d / 25", scoreResult_.totalStars);
        ImGui::Separator();
        ImGui::Spacing();

        // ★評価 4 項目
        if (ImGui::TreeNodeEx("Star Ratings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("  Theme Match : %s (%d)",
                StarsToString(scoreResult_.starThemeMatch).c_str(),
                scoreResult_.starThemeMatch);
            ImGui::Text("  Impact      : %s (%d)",
                StarsToString(scoreResult_.starImpact).c_str(),
                scoreResult_.starImpact);
            ImGui::Text("  Efficiency  : %s (%d)",
                StarsToString(scoreResult_.starEfficiency).c_str(),
                scoreResult_.starEfficiency);
            ImGui::Text("  Commitment  : %s (%d)",
                StarsToString(scoreResult_.starCommitment).c_str(),
                scoreResult_.starCommitment);
            ImGui::TreePop();
        }

        // 審査員評価
        if (ImGui::TreeNodeEx("Judge Evaluations",
            ImGuiTreeNodeFlags_DefaultOpen)) {
            for (size_t i = 0; i < 3; ++i) {
                const auto& je = scoreResult_.judgeEvaluations[i];
                if (!je.judgeId.empty()) {
                    ImGui::Text("  %s (%s)", je.judgeId.c_str(),
                        je.judgeTitle.c_str());
                    ImGui::Text("    %s (%d)",
                        StarsToString(je.star).c_str(), je.star);
                    ImGui::Spacing();
                }
            }
            ImGui::Separator();
            ImGui::Text("  Judge Bonus : %s (%d)",
                StarsToString(scoreResult_.starJudgeBonus).c_str(),
                scoreResult_.starJudgeBonus);
            ImGui::TreePop();
        }

        // デバッグ用：内部スコア詳細
        if (ImGui::TreeNode("Debug: Internal Scores")) {
            const char* partNames[] = {
                "Body",        "Neck",           "Head",
                "LeftUpperArm", "LeftForeArm",   "RightUpperArm",
                "RightForeArm", "LeftThigh",     "LeftShin",
                "RightThigh",   "RightShin",
            };
            const char* countNames[] = {
                "LeftArm", "RightArm", "LeftLeg", "RightLeg",
            };
            const char* bonusNames[] = {
                "Symmetry", "CostEfficiency", "Minimalist",
                "Wildcard", "Balance",
            };

            ImGui::Text("Part Scores (total: %.1f)", scoreResult_.totalPartScore);
            for (int i = 0; i < 11; ++i) {
                ImGui::Text("  %s = %.1f", partNames[i], scoreResult_.partScores[i]);
            }

            ImGui::Text("Count Scores (total: %.1f)", scoreResult_.totalCountScore);
            for (int i = 0; i < 4; ++i) {
                ImGui::Text("  %s = %.1f", countNames[i], scoreResult_.countScores[i]);
            }

            ImGui::Text("Bonus Scores (total: %.1f)", scoreResult_.totalBonusScore);
            for (int i = 0; i < 5; ++i) {
                ImGui::Text("  %s = %.1f", bonusNames[i], scoreResult_.bonusScores[i]);
            }

            ImGui::Text("Final Score: %.1f", scoreResult_.finalScore);
            ImGui::TreePop();
        }
    }

    ImGui::End();
#endif

    // フェード描画
    fade_.Draw();
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