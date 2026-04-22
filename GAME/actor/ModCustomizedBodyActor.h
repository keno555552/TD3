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

  void BuildFromCustomizeData(const ModBodyCustomizeData &data);
  void UpdateAndDraw(Camera *camera);

  void SetActorTranslate(const Vector3 &translate) {
    actorTransform_.translate = translate;
  }

  void SetActorRotate(const Vector3 &rotate) {
    actorTransform_.rotate = rotate;
  }

  void SetActorScale(const Vector3 &scale) { actorTransform_.scale = scale; }

  const Transform &GetActorTransform() const { return actorTransform_; }

  void SetGroundY(float y) { groundY_ = y; }
  void SetGroundOffsetY(float y) { groundOffsetY_ = y; }

  void SetAutoGroundEnabled(bool enabled) { autoGroundEnabled_ = enabled; }

  void SnapToGround();

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

  Vector3 FindBodyPointLocal(int ownerPartId, ModControlPointRole role,
                             const Vector3 &fallback) const;

  float FindBodyPointRadius(int ownerPartId, ModControlPointRole role,
                            float fallback) const;

  float GetDefaultAttachRadius(ModBodyPart childPart) const;
  float GetCurrentAttachRadius(const PartNode &childNode) const;

  float GetDefaultAttachLength(ModBodyPart childPart) const;
  float GetCurrentAttachLength(const PartNode &childNode) const;

  Vector3 ComputeAttachPushOffset(const PartNode &parentNode,
                                  const PartNode &childNode) const;

  void ApplyModBodies();

  bool IsRootPartNode(const PartNode &node) const;
  bool TryGetFootEndWorldPosition(int partId, Vector3 &outWorld) const;
  bool TryGetFootEndContactWorldY(int partId, float &outContactY) const;
  float ComputeLowestFootWorldY() const;
  void ApplyGroundingToRootParts();

  Vector3 ResolveChildSelfAttachOffset(const PartNode &childNode) const;

private:
  kEngine *system_ = nullptr;

  ModAssemblyGraph assembly_;
  std::vector<int> orderedPartIds_;

  std::unordered_map<int, int> modModelHandles_;
  std::unordered_map<int, std::unique_ptr<Object>> modObjects_;
  std::unordered_map<int, ModBody> modBodies_;

  std::unique_ptr<ModBodyCustomizeData> customizeData_ = nullptr;
  std::vector<ModControlPoint> torsoSharedPoints_;

  Transform actorTransform_ = CreateDefaultTransform();

  bool autoGroundEnabled_ = false;
  float groundY_ = 0.0f;
  float groundOffsetY_ = 0.0f;
};