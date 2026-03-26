#include "PromptData.h"

std::string PromptData::selectedPrompt_ = "";
std::string PromptData::selectedPromptTexturePath_ = "";

ThemeData PromptData::selectedTheme_{};
bool PromptData::hasThemeData_ = false;

std::vector<JudgeData> PromptData::selectedJudges_{};
bool PromptData::hasJudgeData_ = false;

void PromptData::SetSelectedPrompt(const std::string &prompt) {
  selectedPrompt_ = prompt;
  selectedPromptTexturePath_.clear();
}

void PromptData::SetSelectedPrompt(const std::string &prompt,
                                   const std::string &texturePath) {
  selectedPrompt_ = prompt;
  selectedPromptTexturePath_ = texturePath;
}

const std::string &PromptData::GetSelectedPrompt() { return selectedPrompt_; }

const std::string &PromptData::GetSelectedPromptTexturePath() {
  return selectedPromptTexturePath_;
}

void PromptData::Clear() {
  selectedPrompt_.clear();
  selectedPromptTexturePath_.clear();
  ClearThemeData();
  ClearJudges();
}

void PromptData::SetThemeData(const ThemeData& theme) {
    selectedTheme_ = theme;
    hasThemeData_ = true;
}

const ThemeData* PromptData::GetThemeData() {
    if (!hasThemeData_) {
        return nullptr;
    }
    return &selectedTheme_;
}

void PromptData::ClearThemeData() {
    selectedTheme_ = ThemeData{};
    hasThemeData_ = false;
}

void PromptData::SetJudges(const std::vector<JudgeData>& judges) {
    selectedJudges_ = judges;
    hasJudgeData_ = true;
}

const std::vector<JudgeData>* PromptData::GetJudges() {
    if (!hasJudgeData_) {
        return nullptr;
    }
    return &selectedJudges_;
}

void PromptData::ClearJudges() {
    selectedJudges_.clear();
    hasJudgeData_ = false;
}