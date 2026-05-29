#include "ParticleEmitter.h"
#include "kEngine.h"

ParticleEmitter::ParticleEmitter(kEngine* system,
	const ParticlePrototype particlePrototype,
	int maxParticles) :
	system_(system),
	prototype_(particlePrototype),
	maxParticles_(maxParticles),
	emitterPosition(particlePrototype.startPosition) {
	isObjectType_ = (particlePrototype.renderType == ParticleRenderType::Object) ?
		ParticleRenderType::Object :
		ParticleRenderType::Sprite;

	emitTimer_.Init0(particlePrototype.emitRate, system_->GetTimeManager());
}

ParticleEmitter::~ParticleEmitter() {}

void ParticleEmitter::Update() {

	/// 発射停止中は更新しない
	if (isPaused_)return;

	/// デルタタイムの取得
	float deltaTime = system_->GetDeltaTime() * timeScale_;

	/// 自動發射
	if (prototype_.emitRate > 0.0f) {
		emitTimer_.parameter_ += deltaTime;
		float interval = 1.0f / prototype_.emitRate;

		while (emitTimer_.parameter_ >= interval) {
			Emit(prototype_.burstCount);
			emitTimer_.parameter_ -= interval;
		}
	}

	/// 更新所有粒子
	for (auto& p : particles_) {

		p.life.parameter_ += deltaTime;
		if (p.life.parameter_ >= p.life.maxTime_) {
			p.isAlive = false;
			continue;
		}

		/// 重力
		p.velocity += prototype_.gravity * deltaTime;

		/// 位置更新
		p.nowTranslate += p.velocity * deltaTime;

		/// 縮放插值
		p.nowScale = {
			Lerp(p.startScale.x, p.endScale.x, p.life.parameter_, p.life.maxTime_),
			Lerp(p.startScale.y, p.endScale.y, p.life.parameter_, p.life.maxTime_),
			Lerp(p.startScale.z, p.endScale.z, p.life.parameter_, p.life.maxTime_)
		};

		/// 旋轉插值
		p.nowRotate = {
			Lerp(p.startRotate.x, p.endRotate.x, p.life.parameter_, p.life.maxTime_),
			Lerp(p.startRotate.y, p.endRotate.y, p.life.parameter_, p.life.maxTime_),
			Lerp(p.startRotate.z, p.endRotate.z, p.life.parameter_, p.life.maxTime_)
		};

		/// 顏色插值
		p.nowColor = {
			//Lerp(p.startColor.x, p.endColor.x, p.life.parameter_, p.life.maxTime_),
			//Lerp(p.startColor.y, p.endColor.y, p.life.parameter_, p.life.maxTime_),
			//Lerp(p.startColor.z, p.endColor.z, p.life.parameter_, p.life.maxTime_),
			//Lerp(p.startColor.w, p.endColor.w, p.life.parameter_, p.life.maxTime_)
			std::clamp(Lerp(p.startColor.x, p.endColor.x, p.life.parameter_, p.life.maxTime_),0.0f,1.0f),
			std::clamp(Lerp(p.startColor.y, p.endColor.y, p.life.parameter_, p.life.maxTime_),0.0f,1.0f),
			std::clamp(Lerp(p.startColor.z, p.endColor.z, p.life.parameter_, p.life.maxTime_),0.0f,1.0f),
			std::clamp(Lerp(p.startColor.w, p.endColor.w, p.life.parameter_, p.life.maxTime_),0.0f,1.0f)
		};

		/// 阻尼
		p.velocity *= std::exp(-prototype_.damping * deltaTime);
	}

	/// 自動發射およびタイマー更新
	if (isEmitting_) {
		/// TimerとParameterも同じく0なら
		/// Timerが無効とし
		/// 発射しないし、更新もしない
		bool isParameter0 = (emitTimer_.parameter_ == 0.0f);
		bool isMaxTime0 = (emitTimer_.maxTime_ == 0.0f);
		if (!(isParameter0 || isMaxTime0)) {
			emitTimer_.ToMix();
			if (emitTimer_.GetIsMax()) {
				Emit(prototype_.burstCount);
				emitTimer_.Reset0();
			}
		}
	}

	/// 無序刪除
	for (int i = (int)particles_.size() - 1; i >= 0; --i) {
		if (!particles_[i].isAlive) {
			dexpiredData_.push_back(particles_.back());
			particles_[i] = std::move(particles_.back());
			particles_.pop_back();
		}
	}
}

void ParticleEmitter::Emit(int count, ParticlePrototypeOverride* prototype) {

	ParticlePrototype usedPrototype{prototype_};
	if (prototype) {
		CopyPPbyActiveData(usedPrototype, *prototype);
	}

	/// 発射停止中は発射しない
	if (!isEmitting_) return;


	for (int i = 0; i < count; i++) {

		float emitRate = randomMaker_->randomFloat(1.0f - usedPrototype.emitNumRandom, 1.0f + usedPrototype.emitNumRandom);
		int shootTime = (int)(usedPrototype.burstCount * emitRate);

		for (int s = 0; s < shootTime; s++) {
			ParticleInstance p;

			p.particleId = counter_++;
			p.isAlive = true;
			p.life.parameter_ = 0.0f;

			// 壽命
			float randLife = usedPrototype.lifetimeRandomness * usedPrototype.lifetime;
			p.life.maxTime_ = usedPrototype.lifetime + randomMaker_->randomFloat(-randLife, randLife);

			// 外觀
			int listSize = (usedPrototype.renderType == ParticleRenderType::Object) ?
				(int)usedPrototype.objectList.size() :
				(int)usedPrototype.spriteList.size();

			if (listSize > 0) {
				p.objectIndex = randomMaker_->randomInt(0, listSize - 1);
			} else {
				p.objectIndex = 0;
			}

			/// ============================== 初期状態設定 ============================== ///

			// 初速
			Vector3 rv = {
				randomMaker_->randomFloat(-usedPrototype.startVelocityRandom.x, usedPrototype.startVelocityRandom.x),
				randomMaker_->randomFloat(-usedPrototype.startVelocityRandom.y, usedPrototype.startVelocityRandom.y),
				randomMaker_->randomFloat(-usedPrototype.startVelocityRandom.z, usedPrototype.startVelocityRandom.z)
			};

			p.velocity = usedPrototype.startVelocity + rv;

			// 初期状態設定
			p.startTranslate = emitterPosition;

			Vector3 ranScale = {
				randomMaker_->randomFloat(1.0f - usedPrototype.startScaleRandom.x, 1.0f + usedPrototype.startScaleRandom.x),
				randomMaker_->randomFloat(1.0f - usedPrototype.startScaleRandom.y, 1.0f + usedPrototype.startScaleRandom.y),
				randomMaker_->randomFloat(1.0f - usedPrototype.startScaleRandom.z, 1.0f + usedPrototype.startScaleRandom.z)
			};
			p.startScale = emitterScale * (usedPrototype.startScale * ranScale);


			Vector3 ranRotate = {
				randomMaker_->randomFloat(-usedPrototype.startRotationRandom.x, usedPrototype.startRotationRandom.x),
				randomMaker_->randomFloat(-usedPrototype.startRotationRandom.y, usedPrototype.startRotationRandom.y),
				randomMaker_->randomFloat(-usedPrototype.startRotationRandom.z, usedPrototype.startRotationRandom.z)
			};
			p.startRotate = emitterRotation + usedPrototype.startRotation + ranRotate;

			Vector4 ranColor = {
				randomMaker_->randomFloat(1.0f - usedPrototype.startColorRandom.x, 1.0f + usedPrototype.startColorRandom.x),
				randomMaker_->randomFloat(1.0f - usedPrototype.startColorRandom.y, 1.0f + usedPrototype.startColorRandom.y),
				randomMaker_->randomFloat(1.0f - usedPrototype.startColorRandom.z, 1.0f + usedPrototype.startColorRandom.z),
				randomMaker_->randomFloat(1.0f - usedPrototype.startColorRandom.w, 1.0f + usedPrototype.startColorRandom.w)
			};
			p.startColor.x = usedPrototype.startColor.x * ranColor.x;
			p.startColor.y = usedPrototype.startColor.y * ranColor.y;
			p.startColor.z = usedPrototype.startColor.z * ranColor.z;
			p.startColor.w = usedPrototype.startColor.w * ranColor.w;

			/// ============================== 最終状態設定 ============================== ///

			if (usedPrototype.isConstantScale) {
				p.endScale = p.startScale;
			} else {
				ranScale = {
					randomMaker_->randomFloat(1.0f - usedPrototype.endScaleRandom.x, 1.0f + usedPrototype.endScaleRandom.x),
					randomMaker_->randomFloat(1.0f - usedPrototype.endScaleRandom.y, 1.0f + usedPrototype.endScaleRandom.y),
					randomMaker_->randomFloat(1.0f - usedPrototype.endScaleRandom.z, 1.0f + usedPrototype.endScaleRandom.z)
				};
				p.endScale = emitterScale * (usedPrototype.endScale * ranScale);
			}


			if (usedPrototype.isConstantRotation) {
				p.endRotate = p.startRotate;
			} else {
				ranRotate = {
				   randomMaker_->randomFloat(-usedPrototype.endRotationRandom.x, usedPrototype.endRotationRandom.x),
				   randomMaker_->randomFloat(-usedPrototype.endRotationRandom.y, usedPrototype.endRotationRandom.y),
				   randomMaker_->randomFloat(-usedPrototype.endRotationRandom.z, usedPrototype.endRotationRandom.z)
				};
				p.endRotate = emitterRotation + usedPrototype.endRotation + ranRotate;
			}

			if (usedPrototype.isConstantColor) {
				p.endColor = p.startColor;
			} else {
				ranColor = {
					randomMaker_->randomFloat(1.0f - usedPrototype.endColorRandom.x, 1.0f + usedPrototype.endColorRandom.x),
					randomMaker_->randomFloat(1.0f - usedPrototype.endColorRandom.y, 1.0f + usedPrototype.endColorRandom.y),
					randomMaker_->randomFloat(1.0f - usedPrototype.endColorRandom.z, 1.0f + usedPrototype.endColorRandom.z),
					randomMaker_->randomFloat(1.0f - usedPrototype.endColorRandom.w, 1.0f + usedPrototype.endColorRandom.w)
				};
				p.endColor.x = usedPrototype.endColor.x * ranColor.x;
				p.endColor.y = usedPrototype.endColor.y * ranColor.y;
				p.endColor.z = usedPrototype.endColor.z * ranColor.z;
				p.endColor.w = usedPrototype.endColor.w * ranColor.w;
			}

			/// ============================== 現在状態設定 ============================== ///

			p.nowTranslate = p.startTranslate;	/// 位置
			p.nowRotate = p.startRotate;		/// 旋轉
			p.nowScale = p.startScale;		/// 縮放
			p.nowColor = p.startColor;		/// 顏色

			particles_.push_back(p);
			emittingData_.push_back(p);
		}
	}
}

void ParticleEmitter::SwitchEmitter(bool isEmitting) {
	isEmitting_ = isEmitting;
}

void ParticleEmitter::Draw() {

	if (isObjectType_ == ParticleRenderType::Object) {
		if (!particles_.empty()) {
			system_->DrawParticle(prototype_.objectList, particles_);
		}
	}
}

std::vector<ParticleInstance>& ParticleEmitter::GetEmittingData(){
	return emittingData_;
}

std::vector<ParticleInstance>& ParticleEmitter::GetDexpiredData(){
	return dexpiredData_;
}

ParticleInstance* ParticleEmitter::FindParticleById(int id) {
	for (auto& p : particles_) {
		if (p.particleId == id) return &p;
	}
	return nullptr;
}

void ParticleEmitter::ClearEmittingData() {
	if(!emittingData_.empty()) {
		emittingData_.clear();
	}
}

void ParticleEmitter::ClearDexpiredData() {
	if (!dexpiredData_.empty()) {
		dexpiredData_.clear();
	}
}