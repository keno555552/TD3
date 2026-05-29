#include "ParticlePrototype.h"

void CopyPPbyActiveData(ParticlePrototype& dest, const ParticlePrototypeOverride& src) {
    // --- 基本生命サイクル ---
    if (src.hasLifetime)
        dest.lifetime = src.lifetime;

    if (src.hasLifetimeRandomness)
        dest.lifetimeRandomness = src.lifetimeRandomness;

    // --- 初始位置 ---
    if (src.hasStartPosition)
        dest.startPosition = src.startPosition;

    if (src.hasStartPositionRandom)
        dest.startPositionRandom = src.startPositionRandom;

    // --- 初速 ---
    if (src.hasStartVelocity)
        dest.startVelocity = src.startVelocity;

    if (src.hasStartVelocityRandom)
        dest.startVelocityRandom = src.startVelocityRandom;

    if (src.hasStartSpeedRandom)
        dest.startSpeedRandom = src.startSpeedRandom;

    // --- 大小 ---
    if (src.hasStartScale)
        dest.startScale = src.startScale;

    if (src.hasEndScale)
        dest.endScale = src.endScale;

    if (src.hasStartScaleRandom)
        dest.startScaleRandom = src.startScaleRandom;

    if (src.hasEndScaleRandom)
        dest.endScaleRandom = src.endScaleRandom;

    if (src.hasIsConstantScale)
        dest.isConstantScale = src.isConstantScale;

    // --- 旋轉 ---
    if (src.hasStartRotation)
        dest.startRotation = src.startRotation;

    if (src.hasEndRotation)
        dest.endRotation = src.endRotation;

    if (src.hasStartRotationRandom)
        dest.startRotationRandom = src.startRotationRandom;

    if (src.hasEndRotationRandom)
        dest.endRotationRandom = src.endRotationRandom;

    if (src.hasIsConstantRotation)
        dest.isConstantRotation = src.isConstantRotation;

    // --- 顏色 ---
    if (src.hasStartColor)
        dest.startColor = src.startColor;

    if (src.hasEndColor)
        dest.endColor = src.endColor;

    if (src.hasStartColorRandom)
        dest.startColorRandom = src.startColorRandom;

    if (src.hasEndColorRandom)
        dest.endColorRandom = src.endColorRandom;

    if (src.hasIsConstantColor)
        dest.isConstantColor = src.isConstantColor;

    // --- 行為 ---
    if (src.hasGravity)
        dest.gravity = src.gravity;

    if (src.hasDamping)
        dest.damping = src.damping;

    if (src.hasDampingRandom)
        dest.dampingRandom = src.dampingRandom;

    // --- 發射設定 ---
    if (src.hasEmitRate)
        dest.emitRate = src.emitRate;

    if (src.hasEmitIntervalRandom)
        dest.emitIntervalRandom = src.emitIntervalRandom;

    if (src.hasBurstCount)
        dest.burstCount = src.burstCount;

    if (src.hasEmitNumRandom)
        dest.emitNumRandom = src.emitNumRandom;

    // --- 外觀 ---
    if (src.hasRenderType)
        dest.renderType = src.renderType;

    if (src.hasObjectList)
        dest.objectList = src.objectList;

    if (src.hasSpriteList)
        dest.spriteList = src.spriteList;

    // --- 路徑 ---
    if (src.hasModelPath)
        dest.modelPath = src.modelPath;

    if (src.hasTexturePath)
        dest.texturePath = src.texturePath;
}
