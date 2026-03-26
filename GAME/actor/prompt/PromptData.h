#pragma once
#include <string>
#include "GAME/theme/ThemeData.h"
#include "GAME/judges/JudgeData.h"

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

  static void SetJudges(const std::vector<JudgeData>& judges);
  static const std::vector<JudgeData>* GetJudges();
  static void ClearJudges();

private:
  static std::string selectedPrompt_;
  static std::string selectedPromptTexturePath_;

  static ThemeData selectedTheme_;
  static bool hasThemeData_;

  static std::vector<JudgeData> selectedJudges_;
  static bool hasJudgeData_;
};