#pragma once
#include <vector>
#include <map>
#include "config.h"
#include "DirectXCore.h"
#include "ParticleEmitter.h"
#include "EmitterLink.h"

struct delayData {
	bool hasEmitted{ false };
	bool activeNow{ false };
	bool followParticle{ false };
	int emittyCount{ 1 };
	int emitterLinkIndex{ -1 };
	int sourceEmitterId{ -1 };
	int targetEmitterId{ -1 };
	int particleId{ -1 };
	Vector3 positionOffset{};
	Vector3 rotationOffset{};
	Vector3 scaleOffset{};
	Timer timer{};
};

class kEngine;
class ParticleManager
{
public:
	ParticleManager(kEngine* system);
	~ParticleManager();

	/// --- 基本関数 --- ///

	void Update();
	void Draw();

	/// --- エミッタ管理 --- ///
	int CreateEmitter(const ParticlePrototype& proto, int maxParticles);
	void ClearEmitter(int emitterId);

	/// 手動発射
	void ShootEmitter(int emitterId, int count);
	void SwitchEmitter(int emitterId, bool isEmitting);

	/// リングパーティクル関連
	//void LinkParticleToObject(int emitterId, Object* object); //まだ準備できてない
	void LinkEmitterToEmitter(EmitterLink& linkData);

private:

	/// 借りのDevice
	kEngine* system_ = nullptr;

	/// 粒子生成器
	std::map<int, std::unique_ptr<ParticleEmitter>> emitterList_;
	int nextEmitterId_ = 0;

	/// Emitter同士のリンク管理
	std::vector<EmitterLink> emitterLinks_;
	std::vector<delayData> delayDataList_;

private:
	/// --- 内部関数 --- ///
	void UpdateEmitterLinks();


};