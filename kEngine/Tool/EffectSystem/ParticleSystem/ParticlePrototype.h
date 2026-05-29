#pragma once
#include <string>
#include <vector>
#include "Vector3.h"
#include "Vector4.h"
#include "Transform.h"
#include "Data/Render/CPUData/ObjectData.h"
#include "Data/Render/CPUData/SpriteData.h"
    
enum class ParticleRenderType
{
	Object,
	Sprite
};


struct ParticlePrototype
{
    // --- 基本生命サイクル ---
    float lifetime = 1.0f;           // 粒子の最大寿命（秒）
    float lifetimeRandomness = 0.0f; // 壽命ランダム比率（0~1）

    // --- 初始狀態 ---
	Vector3 startPosition = { 0, 0, 0 };
	Vector3 startPositionRandom = { 0, 0, 0 }; // 初始位置ランダム比率

    Vector3 startVelocity = { 0, 0, 0 };
    Vector3 startVelocityRandom = { 0, 0, 0 }; // 初速ランダム比率
    Vector3 startSpeedRandom = { 0.0f, 0.0f, 0.0f }; // 初速ランダム比率（全体速度のランダム率）

    Vector3 startScale = { 1, 1, 1 };
    Vector3 endScale = { 1, 1, 1 };
    Vector3 startScaleRandom = { 0.0f, 0.0f, 0.0f }; // 大きさの変化値（+-のランダム率）
    Vector3 endScaleRandom = { 0.0f, 0.0f, 0.0f }; // 大きさの変化値（+-のランダム率）
	bool isConstantScale = true; // 変化しないかどうか

    Vector3 startRotation = { 0, 0, 0 };
    Vector3 endRotation = { 0, 0, 0 };
    Vector3 startRotationRandom = { 0.0f, 0.0f, 0.0f }; // 回転の変化値（+-のランダム率）
    Vector3 endRotationRandom = { 0.0f, 0.0f, 0.0f };
    bool isConstantRotation = true; // 変化しないかどうか

    Vector4 startColor = { 1, 1, 1, 1 };
    Vector4 endColor = { 1, 1, 1, 0 };
    Vector4 startColorRandom = { 0.0f, 0.0f, 0.0f, 0.0f }; // 色の変化値（+-のランダム率）
    Vector4 endColorRandom = { 0.0f, 0.0f, 0.0f, 0.0f };
	bool isConstantColor = false; // 変化しないかどうか

    // --- 行為変数 ---
    Vector3 gravity = { 0, 0, 0 };
    float damping = 0.0f; // 速度減衰（0 = 無減衰, 1 = 即停止）
	float dampingRandom = 0.0f; // 速度減衰のランダム率（+-のランダム率）

    // --- 発射設定 ---
    float emitRate = 0.0f; // 秒こと発射回数（0 = 手動）
	float emitIntervalRandom = 0.0f; // 発射間隔のランダム率（+-のランダム率）

    int burstCount = 1;    // 一回発射で生成する粒子数
	float emitNumRandom = 0.0f; // 発射間隔のランダム率（+-のランダム率）

    // --- 外観設定（未來可擴展） ---
	ParticleRenderType renderType = ParticleRenderType::Object;     // 描画タイプ
    std::vector<ObjectData> objectList;                             // 表示用物体(object)
    std::vector<SimpleSpriteData> spriteList;                       // 表示用スプライト(sprite)

    // --- マテリアルのロケーション（以降のJSON用） ---
    std::string modelPath;
    std::string texturePath;

};

/// ParticlePrototypeのオーバーライド用構造体
struct ParticlePrototypeOverride
{
    // --- 基本生命サイクル ---
    bool hasLifetime = false;
    float lifetime = 1.0f;

    bool hasLifetimeRandomness = false;
    float lifetimeRandomness = 0.0f;

    // --- 初始位置 ---
    bool hasStartPosition = false;
    Vector3 startPosition = { 0,0,0 };

    bool hasStartPositionRandom = false;
    Vector3 startPositionRandom = { 0,0,0 };

    // --- 初速 ---
    bool hasStartVelocity = false;
    Vector3 startVelocity = { 0,0,0 };

    bool hasStartVelocityRandom = false;
    Vector3 startVelocityRandom = { 0,0,0 };

    bool hasStartSpeedRandom = false;
    Vector3 startSpeedRandom = { 0,0,0 };

    // --- 大小 ---
    bool hasStartScale = false;
    Vector3 startScale = { 1,1,1 };

    bool hasEndScale = false;
    Vector3 endScale = { 1,1,1 };

    bool hasStartScaleRandom = false;
    Vector3 startScaleRandom = { 0,0,0 };

    bool hasEndScaleRandom = false;
    Vector3 endScaleRandom = { 0,0,0 };

    bool hasIsConstantScale = false;
    bool isConstantScale = true;

    // --- 旋轉 ---
    bool hasStartRotation = false;
    Vector3 startRotation = { 0,0,0 };

    bool hasEndRotation = false;
    Vector3 endRotation = { 0,0,0 };

    bool hasStartRotationRandom = false;
    Vector3 startRotationRandom = { 0,0,0 };

    bool hasEndRotationRandom = false;
    Vector3 endRotationRandom = { 0,0,0 };

    bool hasIsConstantRotation = false;
    bool isConstantRotation = true;

    // --- 顏色 ---
    bool hasStartColor = false;
    Vector4 startColor = { 1,1,1,1 };

    bool hasEndColor = false;
    Vector4 endColor = { 1,1,1,0 };

    bool hasStartColorRandom = false;
    Vector4 startColorRandom = { 0,0,0,0 };

    bool hasEndColorRandom = false;
    Vector4 endColorRandom = { 0,0,0,0 };

    bool hasIsConstantColor = false;
    bool isConstantColor = false;

    // --- 行為 ---
    bool hasGravity = false;
    Vector3 gravity = { 0,0,0 };

    bool hasDamping = false;
    float damping = 0.0f;

    bool hasDampingRandom = false;
    float dampingRandom = 0.0f;

    // --- 發射設定 ---
    bool hasEmitRate = false;
    float emitRate = 0.0f;

    bool hasEmitIntervalRandom = false;
    float emitIntervalRandom = 0.0f;

    bool hasBurstCount = false;
    int burstCount = 1;

    bool hasEmitNumRandom = false;
    float emitNumRandom = 0.0f;

    // --- 外觀 ---
    bool hasRenderType = false;
    ParticleRenderType renderType = ParticleRenderType::Object;

    bool hasObjectList = false;
    std::vector<ObjectData> objectList;

    bool hasSpriteList = false;
    std::vector<SimpleSpriteData> spriteList;

    // --- 路徑 ---
    bool hasModelPath = false;
    std::string modelPath;

    bool hasTexturePath = false;
    std::string texturePath;
};

/// ParticlePrototypeにParticlePrototypeOverrideの内容をコピーする関数
void CopyPPbyActiveData(ParticlePrototype& dest, const ParticlePrototypeOverride& src);