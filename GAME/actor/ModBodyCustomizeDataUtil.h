#pragma once
#include "GAME/actor/ModBody.h"
#include <vector>

namespace ModBodyCustomizeDataUtil {

inline const ModPartInstanceData *FindPartById(const ModBodyCustomizeData &data,
                                               int partId) {
  for (size_t i = 0; i < data.partInstances.size(); ++i) {
    if (data.partInstances[i].partId == partId) {
      return &data.partInstances[i];
    }
  }
  return nullptr;
}

inline std::vector<const ModPartInstanceData *>
FindPartsByType(const ModBodyCustomizeData &data, ModBodyPart partType) {
  std::vector<const ModPartInstanceData *> result;

  for (size_t i = 0; i < data.partInstances.size(); ++i) {
    if (data.partInstances[i].partType == partType) {
      result.push_back(&data.partInstances[i]);
    }
  }

  return result;
}

inline std::vector<const ModPartInstanceData *>
FindChildren(const ModBodyCustomizeData &data, int parentId) {
  std::vector<const ModPartInstanceData *> result;

  for (size_t i = 0; i < data.partInstances.size(); ++i) {
    if (data.partInstances[i].parentId == parentId) {
      result.push_back(&data.partInstances[i]);
    }
  }

  return result;
}

inline std::vector<const ModControlPointSnapshot *>
FindControlPointsByOwnerPartId(const ModBodyCustomizeData &data,
                               int ownerPartId) {
  std::vector<const ModControlPointSnapshot *> result;

  for (size_t i = 0; i < data.controlPointSnapshots.size(); ++i) {
    if (data.controlPointSnapshots[i].ownerPartId == ownerPartId) {
      result.push_back(&data.controlPointSnapshots[i]);
    }
  }

  return result;
}

inline const ModControlPointSnapshot *
FindControlPointByRole(const ModBodyCustomizeData &data, int ownerPartId,
                       ModControlPointRole role) {
  for (size_t i = 0; i < data.controlPointSnapshots.size(); ++i) {
    const ModControlPointSnapshot &snapshot = data.controlPointSnapshots[i];
    if (snapshot.ownerPartId == ownerPartId && snapshot.role == role) {
      return &snapshot;
    }
  }
  return nullptr;
}

inline bool HasNewFormatData(const ModBodyCustomizeData &data) {
  return !data.partInstances.empty() || !data.controlPointSnapshots.empty();
}

} // namespace ModBodyCustomizeDataUtil