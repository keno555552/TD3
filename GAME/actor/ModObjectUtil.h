#pragma once
#include "Object/Object.h"

namespace ModObjectUtil {

inline Vector3 ComputeMainPositionWorldTranslate(const Object *target) {
  if (target == nullptr) {
    return {0.0f, 0.0f, 0.0f};
  }

  Vector3 world = target->mainPosition.transform.translate;

  const ObjectPart *parent = target->mainPosition.parentPart;
  while (parent != nullptr) {
    world = Add(world, parent->transform.translate);
    parent = parent->parentPart;
  }

  return world;
}

inline Vector3 ComputeObjectRootWorldTranslate(const Object *object) {
  // 現状コードは mainPosition ベースと同一挙動なので alias として維持する
  return ComputeMainPositionWorldTranslate(object);
}

} // namespace ModObjectUtil