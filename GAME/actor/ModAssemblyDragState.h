#pragma once
#include "ModAssemblyTypes.h"
#include "Transform.h"
#include "Vector3.h"

struct ModAssemblyDragState {
  bool isDragging = false;

  int pickedPartId = -1;
  int assemblyRootPartId = -1;

  ModAssemblyType assemblyType = ModAssemblyType::None;
  PartSide side = PartSide::Center;

  int beforeParentId = -1;
  int beforeParentConnectorId = -1;
  int beforeSelfConnectorId = -1;

  Vector3 beforeLocalTranslate{0.0f, 0.0f, 0.0f};
  Vector3 beforeLocalRotate{0.0f, 0.0f, 0.0f};

  Vector3 previewLocalTranslate{0.0f, 0.0f, 0.0f};
  Vector3 previewLocalRotate{0.0f, 0.0f, 0.0f};

  int hoveredParentPartId = -1;
  int hoveredParentConnectorId = -1;
  ModAttachFace hoveredFace = ModAttachFace::PosY;

  bool isPlacementValid = false;

  Vector3 dragPlanePoint{0.0f, 0.0f, 0.0f};
  Vector3 dragPlaneNormal{0.0f, 0.0f, 1.0f};

  Vector3 dragRootOffset{0.0f, 0.0f, 0.0f};

  void Clear() {
    isDragging = false;
    pickedPartId = -1;
    assemblyRootPartId = -1;
    assemblyType = ModAssemblyType::None;
    side = PartSide::Center;
    beforeParentId = -1;
    beforeParentConnectorId = -1;
    beforeSelfConnectorId = -1;
    beforeLocalTranslate = {0.0f, 0.0f, 0.0f};
    beforeLocalRotate = {0.0f, 0.0f, 0.0f};
    previewLocalTranslate = {0.0f, 0.0f, 0.0f};
    previewLocalRotate = {0.0f, 0.0f, 0.0f};
    hoveredParentPartId = -1;
    hoveredParentConnectorId = -1;
    hoveredFace = ModAttachFace::PosY;
    isPlacementValid = false;
    dragPlanePoint = {0.0f, 0.0f, 0.0f};
    dragPlaneNormal = {0.0f, 0.0f, 1.0f};
    dragRootOffset = {0.0f, 0.0f, 0.0f};
  }
};