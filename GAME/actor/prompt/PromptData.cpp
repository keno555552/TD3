#include "PromptData.h"

std::string PromptData::selectedPrompt_ = "";
std::string PromptData::selectedPromptTexturePath_ = "";

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
}