#pragma once
#include "GAME/actor/ModAssemblyGraph.h"
#include "GAME/actor/ModAssemblyUtil.h"

namespace ModAssemblyResolver {

inline int ResolveAssemblyRootPartId(const ModAssemblyGraph &graph,
                                     int partId) {
  const PartNode *start = graph.FindNode(partId);
  if (start == nullptr) {
    return -1;
  }

  const ModBodyPart rootPartType =
      ModAssemblyUtil::GetAssemblyRootPartType(start->part);

  if (start->part == rootPartType) {
    return start->id;
  }

  int currentId = start->parentId;
  while (currentId >= 0) {
    const PartNode *node = graph.FindNode(currentId);
    if (node == nullptr) {
      break;
    }

    if (node->part == rootPartType) {
      return node->id;
    }

    currentId = node->parentId;
  }

  return start->id;
}

inline bool BelongsToAssemblyRoot(const ModAssemblyGraph &graph, int rootPartId,
                                  int candidatePartId) {
  const PartNode *rootNode = graph.FindNode(rootPartId);
  const PartNode *candidateNode = graph.FindNode(candidatePartId);
  if (rootNode == nullptr || candidateNode == nullptr) {
    return false;
  }

  const ModAssemblyKey rootKey =
      ModAssemblyUtil::GetAssemblyKey(rootNode->part);
  const ModAssemblyKey candidateKey =
      ModAssemblyUtil::GetAssemblyKey(candidateNode->part);

  if (rootKey != candidateKey) {
    return false;
  }

  const int resolvedRoot = ResolveAssemblyRootPartId(graph, candidatePartId);
  return resolvedRoot == rootPartId;
}

inline std::vector<int> CollectAssemblyPartIds(const ModAssemblyGraph &graph,
                                               int rootPartId) {
  std::vector<int> result;
  const std::vector<int> ids = graph.GetNodeIdsSorted();

  for (size_t i = 0; i < ids.size(); ++i) {
    if (BelongsToAssemblyRoot(graph, rootPartId, ids[i])) {
      result.push_back(ids[i]);
    }
  }

  return result;
}

} // namespace ModAssemblyResolver