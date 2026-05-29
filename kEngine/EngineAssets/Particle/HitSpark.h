#pragma once
#include "Tool/EffectSystem/ParticleSystem/ParticlePrototype.h"
#include "Tool/EffectSystem/ParticleSystem/EmitterLink.h"

struct HitSpark : ParticlePrototype
{
	HitSpark() {
		/// 生存時間
		lifetime = 1.0f;

		/// 移動しないから設定要らない

		/// スケール設定、その後変わらない
		startScale = { 0.05f, 1.0f, 1.0f };
		endScale = { 0.05f, 1.0f, 1.0f };
		isConstantScale = true;

		/// 回転はランダム
		startRotationRandom = { 0.0f, 0.0f, 3.2f };
		isConstantRotation = true;

		/// 色は元の色から透明へ
		startColor = { 1.0f, 0.3f, 0.3f, 1.0f };
		endColor = { 1.0f, 0.3f, 0.3f, 0.0f };

		/// 行為設定
		burstCount = 8;
		emitNumRandom = 0.5f;

		/// 見た目設定
		ObjectData obj;
		obj.isBillboard_ = true;
		obj.modelHandle_ = config::config::default_Plane_MeshBufferHandle_;

		ObjectPart newObjectPart;
		newObjectPart.name = "HitSpark";
		newObjectPart.materialConfig = std::make_shared<MaterialConfig>();
		newObjectPart.materialConfig->MakePSOParticle();
		newObjectPart.materialConfig->enableLighting = false;
		newObjectPart.materialConfig->textureColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		InitMaterialConfig(newObjectPart.materialConfig.get());
		obj.objectParts_.push_back(newObjectPart);

		objectList.push_back(obj);
	}
};

struct HitImpact : ParticlePrototype
{
	HitImpact() {
		/// 生存時間
		lifetime = 0.5f;
		/// 移動しないから設定要らない
		/// スケール設定、その後変わらない
		startScale = { 0.2f, 0.2f, 1.0f };
		endScale = { 0.3f, 0.3f, 1.0f };
		isConstantScale = false;
		/// 回転はランダム
		startRotationRandom = { 0.0f, 0.0f, 3.2f };
		isConstantRotation = true;
		/// 色は元の色から透明へ
		startColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		endColor = { 1.0f, 1.0f, 1.0f, 0.0f };
		/// 行為設定
		burstCount = 1;
		/// 見た目設定
		ObjectData obj;
		obj.isBillboard_ = true;
		obj.modelHandle_ = config::config::default_Ring_MeshBufferHandle_;
		ObjectPart newObjectPart;
		newObjectPart.name = "HitImpact";
		newObjectPart.materialConfig = std::make_shared<MaterialConfig>();
		newObjectPart.materialConfig->MakePSOParticle();
		newObjectPart.materialConfig->enableLighting = false;
		newObjectPart.materialConfig->textureColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		InitMaterialConfig(newObjectPart.materialConfig.get());
		obj.objectParts_.push_back(newObjectPart);
		objectList.push_back(obj);
	}
};

struct HitSpackImpactLink : EmitterLink
{
	HitSpackImpactLink() {
		emitCount = 1;
		delayTime = 0.0f;
		linkMode = LinkMode::PerBurst;
		linkFollow = LinkFollow::Emitter;
		emitterTiming = EmitterTiming::SourceEmit;
	}
};
