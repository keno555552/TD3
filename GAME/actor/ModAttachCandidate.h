#pragma once
#include "GAME/actor/ModAssemblyTypes.h"
#include "Vector3.h"

struct ModAttachFaceCandidate {
  int parentPartId = -1;
  int parentConnectorId = -1;
  ModAttachFace face = ModAttachFace::PosY;

  Vector3 worldPosition{0.0f, 0.0f, 0.0f};
  Vector3 worldNormal{0.0f, 1.0f, 0.0f};

  float distanceSq = 0.0f;
  bool isValid = false;
};

struct ModAttachSearchResult {
  ModAttachFaceCandidate bestCandidate{};
  bool found = false;
};