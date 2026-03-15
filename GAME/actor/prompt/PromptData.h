#pragma once
#include <string>

class PromptData {
public:
  static void SetSelectedPrompt(const std::string &prompt);
  static void SetSelectedPrompt(const std::string &prompt,
                                const std::string &texturePath);

  static const std::string &GetSelectedPrompt();
  static const std::string &GetSelectedPromptTexturePath();

  static void Clear();

private:
  static std::string selectedPrompt_;
  static std::string selectedPromptTexturePath_;
};