#pragma once
#include "ModBody.h"
#include <memory>

class ModCustomizeDataStore {
public:
  static std::unique_ptr<ModBodyCustomizeData> CreateDefaultCustomizeData();
  static std::unique_ptr<ModBodyCustomizeData> CopySharedCustomizeData();
  static void SetSharedCustomizeData(const ModBodyCustomizeData &data);
  static const ModBodyCustomizeData *GetSharedCustomizeData();

  static void NormalizeCustomizeData(ModBodyCustomizeData &data);

private:
  ModCustomizeDataStore() = delete;
};