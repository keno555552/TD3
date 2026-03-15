#pragma once
#include <string>
#include "GAME/theme/ThemeData.h"

class PromptData {
public:
  static void SetSelectedPrompt(const std::string &prompt);
  static void SetSelectedPrompt(const std::string &prompt,
                                const std::string &texturePath);

  static const std::string &GetSelectedPrompt();
  static const std::string &GetSelectedPromptTexturePath();

  static void Clear();

  static void SetThemeData(const ThemeData& theme);
  static const ThemeData* GetThemeData();
  static void ClearThemeData();

private:
  static std::string selectedPrompt_;
  static std::string selectedPromptTexturePath_;

  static ThemeData selectedTheme_;
  static bool hasThemeData_;
};