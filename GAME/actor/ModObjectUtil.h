#pragma once
#include "Object/Object.h"

namespace ModObjectUtil {

inline Matrix4x4 ComputeMainPositionWorldMatrix(const Object *target) {
  if (target == nullptr) {
    return Identity();
  }

  Matrix4x4 objectMainMatrix =
      MakeAffineMatrix(target->mainPosition.transform.scale,
                       target->mainPosition.transform.rotate,
                       target->mainPosition.transform.translate);

  Matrix4x4 parentMatrix = Identity();

  const ObjectPart *parent = target->mainPosition.parentPart;
  while (parent != nullptr) {
    Matrix4x4 local =
        MakeAffineMatrix(parent->transform.scale, parent->transform.rotate,
                         parent->transform.translate);
    parentMatrix = local * parentMatrix;
    parent = parent->parentPart;
  }

  return objectMainMatrix * parentMatrix;
}

inline Vector3 TransformPoint(const Matrix4x4 &m, const Vector3 &p) {
  Vector3 result{};
  result.x = p.x * m.m[0][0] + p.y * m.m[1][0] + p.z * m.m[2][0] + m.m[3][0];
  result.y = p.x * m.m[0][1] + p.y * m.m[1][1] + p.z * m.m[2][1] + m.m[3][1];
  result.z = p.x * m.m[0][2] + p.y * m.m[1][2] + p.z * m.m[2][2] + m.m[3][2];
  return result;
}

inline Vector3 ComputeMainPositionWorldTranslate(const Object *target) {
  const Matrix4x4 world = ComputeMainPositionWorldMatrix(target);
  return {world.m[3][0], world.m[3][1], world.m[3][2]};
}

inline Vector3 ComputeObjectRootWorldTranslate(const Object *object) {
  return ComputeMainPositionWorldTranslate(object);
}

inline Vector3 TransformLocalPointToWorld(const Object *target,
                                          const Vector3 &localPoint) {
  const Matrix4x4 world = ComputeMainPositionWorldMatrix(target);
  return TransformPoint(world, localPoint);
}

inline Vector3 TransformWorldPointToLocal(const Object *target,
                                          const Vector3 &worldPoint) {
  if (target == nullptr) {
    return worldPoint;
  }

  Matrix4x4 world = ComputeMainPositionWorldMatrix(target);
  Matrix4x4 invWorld = world.Inverse();
  return TransformPoint(invWorld, worldPoint);
}

} // namespace ModObjectUtil