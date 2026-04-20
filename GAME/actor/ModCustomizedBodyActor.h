#pragma once
#include "GAME/actor/ModAssemblyGraph.h"
#include "GAME/actor/ModBody.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class ModCustomizedBodyActor {
public:
  void Initialize(kEngine *system);

  // 改造データから再構築
  void BuildFromCustomizeData(const ModBodyCustomizeData &data);

  // 毎フレーム更新＋描画
  void UpdateAndDraw(Camera *camera);

private:
  static std::string ModelPath(ModBodyPart part);
  static int ResolveControlOwnerPartId(const ModAssemblyGraph &assembly,
                                       int partId);

  void ClearRuntimeObjects();
  void SyncObjectsWithAssembly();
  void CreateObjectForNode(int partId, const PartNode &node);

  void RestorePartParamsFromCustomizeData();
  void RestoreControlPointsFromCustomizeData();

  void ApplyAssemblyToSceneHierarchy();
  Vector3 ResolveDynamicAttachBase(const PartNode &parentNode,
                                   const PartNode &childNode,
                                   const Vector3 &defaultAttach) const;
  int FindTorsoOwnerId() const;
  Vector3 FindSnapshotLocal(int ownerPartId, ModControlPointRole role,
                            const Vector3 &fallback) const;

  void ApplyModBodies();

private:
  kEngine *system_ = nullptr;

  ModAssemblyGraph assembly_;
  std::vector<int> orderedPartIds_;

  std::unordered_map<int, int> modModelHandles_;
  std::unordered_map<int, std::unique_ptr<Object>> modObjects_;
  std::unordered_map<int, ModBody> modBodies_;

  std::unique_ptr<ModBodyCustomizeData> customizeData_ = nullptr;
  std::vector<ModControlPoint> torsoSharedPoints_;
};