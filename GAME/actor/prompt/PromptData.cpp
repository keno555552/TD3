#include "PromptData.h"

std::string PromptData::selectedPrompt_ = "";
std::string PromptData::selectedPromptTexturePath_ = "";

ThemeData PromptData::selectedTheme_{};
bool PromptData::hasThemeData_ = false;

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