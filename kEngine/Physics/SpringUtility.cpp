#include "SpringUtility.h"

void springUpdate(Spring* spring, float deltaTime) {
    Vector3 diff = spring->end.position - spring->anchor;
    float length = Length(diff);
    if (length != 0.0f) {
        Vector3 direction = Normalize(diff);
        Vector3 restPosition = spring->anchor + direction * spring->naturalLength;
        Vector3 displacement = length * (spring->end.position - restPosition);
        Vector3 restoringForce = -spring->stiffness * displacement;
        Vector3 dampingForce = -spring->dampingCoefficient * spring->end.velocity;
        Vector3 force = restoringForce + dampingForce;
        spring->end.acceleration = force / spring->end.mass;
    }
    // 加速度や速度のどちらも秒を基準として計算する
    // それが、1/60秒間(deltaTime)適用されたと考える
    spring->end.velocity = spring->end.velocity + spring->end.acceleration * deltaTime;
    spring->end.position = spring->end.position + spring->end.velocity * deltaTime;

}
