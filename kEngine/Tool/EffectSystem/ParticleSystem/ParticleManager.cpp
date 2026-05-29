#include "ParticleManager.h"
#include "kEngine.h"

ParticleManager::ParticleManager(kEngine* system) :
	system_(system) {}

ParticleManager::~ParticleManager() {}

void ParticleManager::Update() {

	for (auto& [id, emitter] : emitterList_) {
		emitter->Update();
	}

	UpdateEmitterLinks();

	for (auto& [id, emitter] : emitterList_) {
		emitter->ClearEmittingData();
		emitter->ClearDexpiredData();
	}
}

void ParticleManager::Draw() {

	for (auto& [id, emitter] : emitterList_) {
		emitter->Draw();
	}

}

int ParticleManager::CreateEmitter(const ParticlePrototype& proto, int maxParticles) {
	int id = nextEmitterId_++;
	emitterList_[id] = std::make_unique<ParticleEmitter>(system_, proto, maxParticles);
	return id;
}

void ParticleManager::ClearEmitter(int emitterId) {

	auto it = emitterList_.find(emitterId);
	if (it != emitterList_.end()) {
		emitterList_.erase(emitterId);
	} else {
		Logger::Log("[kError] ParticleManager::ClearEmitter() Emitter ID not found: " + std::to_string(emitterId));
	}
}

void ParticleManager::ShootEmitter(int emitterId, int count) {
	auto it = emitterList_.find(emitterId);
	if (it != emitterList_.end()) {
		it->second->Emit(count);
	} else {
		Logger::Log("[kError] ParticleManager::ShootEmitter() Emitter ID not found: " + std::to_string(emitterId));
	}
}

void ParticleManager::SwitchEmitter(int emitterId, bool isEmitting) {
	auto it = emitterList_.find(emitterId);
	if (it != emitterList_.end()) {
		it->second->SwitchEmitter(isEmitting);
	} else {
		Logger::Log("[kError] ParticleManager::SwitchEmitter() Emitter ID not found: " + std::to_string(emitterId));
	}
}

void ParticleManager::LinkEmitterToEmitter(EmitterLink& linkData) {

	/// リンクデータを保存
	EmitterLink emitterLinks{ linkData };

	/// リンクデータをリストに追加
	emitterLinks_.push_back(emitterLinks);
}

void ParticleManager::UpdateEmitterLinks() {

	for (int i = 0; i < emitterLinks_.size(); i++) {

		auto& linkData = emitterLinks_[i];

		auto sourceIt = emitterList_.find(linkData.sourceId);
		auto targetIt = emitterList_.find(linkData.targetId);
		if (sourceIt == emitterList_.end()) {
			Logger::Log("[kError] ParticleMkanager::UpdateEmitterLinks() Emitter ID not found: " + std::to_string(linkData.sourceId));
			continue;
		}
		if (targetIt == emitterList_.end()) {
			Logger::Log("[kError] ParticleManager::UpdateEmitterLinks() Emitter ID not found: " + std::to_string(linkData.targetId));
			continue;
		}

		/// 連動処理
		ParticleEmitter* sourceEmitter = sourceIt->second.get();

		std::vector<ParticleInstance> emittingData;
		if (linkData.emitterTiming != EmitterTiming::SourceEnd) {
			emittingData = sourceEmitter->GetEmittingData();
			if (emittingData.size() != 0) {
				Logger::Log("[kInfo] ParticleManager::UpdateEmitterLinks() Found " + std::to_string(emittingData.size()) + " emitting particles in source emitter ID: " + std::to_string(linkData.sourceId));
			}
		} else {
			emittingData = sourceEmitter->GetDexpiredData();
			if (emittingData.size() != 0) {
				Logger::Log("[kInfo] ParticleManager::UpdateEmitterLinks() Found " + std::to_string(emittingData.size()) + " Dexpired particles in source emitter ID: " + std::to_string(linkData.sourceId));
			}
		}


		if (emittingData.empty())
			continue;

		std::vector<const ParticleInstance*> eventUnits;

		if (linkData.linkMode == LinkMode::PerBurst) {
			eventUnits.push_back(&emittingData[0]);
		} else {
			for (const auto& p : emittingData) {
				eventUnits.push_back(&p);
			}
		}

		std::vector<delayData> newDelayDataList;

		/// 発射したらデータはあるのでチェック
		for (auto* p : eventUnits) {

			delayData d;
			d.emitterLinkIndex = i;
			d.sourceEmitterId = linkData.sourceId;
			d.targetEmitterId = linkData.targetId;

			if (linkData.linkFollow == LinkFollow::Emitter) {
				d.followParticle = false;
				d.particleId = -1;

				d.positionOffset = sourceEmitter->GetPosition();
				d.rotationOffset = sourceEmitter->GetRotation();
				d.scaleOffset = sourceEmitter->GetScale();
			} else {
				d.followParticle = true;
				d.particleId = p->particleId;
				d.positionOffset = p->nowTranslate;
				d.rotationOffset = p->nowRotate;
				d.scaleOffset = p->nowScale;
			}



			if (linkData.emitterTiming == EmitterTiming::SourceEmit ||
				linkData.emitterTiming == EmitterTiming::SourceEnd) {
				d.activeNow = true;
				d.timer.Init0(0.0f, system_->GetTimeManager());
			} else {
				d.activeNow = false;
				d.timer.Init0(linkData.delayTime, system_->GetTimeManager());
			}

			d.emittyCount = linkData.emitCount;

			newDelayDataList.push_back(d);
		}

		// 把 newDelayDataList 丟進 emitterList_
		delayDataList_.insert(
			delayDataList_.end(),
			newDelayDataList.begin(),
			newDelayDataList.end()
		);
	}
	// 既にあるイベントの処理

	for (auto& delayData : delayDataList_) {

		/// タイマーが動いてて、タイマーがMixになるかactiveNowであるか、次のParticleをEmittingする
		bool ready =
			(delayData.activeNow) ||        // 立即觸發
			(delayData.timer.ToMix());      // 延遲觸發

		if (!ready)
			continue;

		auto& linkData = emitterLinks_[delayData.emitterLinkIndex];
		auto& targetEmitter = emitterList_[linkData.targetId];
		auto& sourceEmitter = emitterList_[linkData.sourceId];

		ParticlePrototypeOverride protoOverride{};
		protoOverride.hasStartPosition = true;
		if(linkData.followScale)protoOverride.hasStartScale = true;
		if (linkData.followRotation)protoOverride.hasStartRotation = true;

		auto particle = sourceEmitter->FindParticleById(delayData.particleId);
		if (linkData.followPosition) {
			protoOverride.startPosition = (particle) ?
				protoOverride.startPosition = particle->nowTranslate :
				protoOverride.startPosition = delayData.positionOffset;
		} else {
			protoOverride.startPosition = delayData.positionOffset;
		}

		if (linkData.followScale) {
			protoOverride.startScale = (particle) ?
				protoOverride.startScale = particle->nowScale :
				protoOverride.startScale = delayData.scaleOffset;
		} else {
				protoOverride.startScale = delayData.scaleOffset;
		}
		if (linkData.followRotation) {
			protoOverride.startRotation = (particle) ?
				protoOverride.startRotation = particle->nowRotate :
				protoOverride.startRotation = delayData.rotationOffset;
		} else {
			protoOverride.startRotation = delayData.rotationOffset;
		}

		targetEmitter->Emit(delayData.emittyCount, &protoOverride);

		delayData.hasEmitted = true;
	}

	/// 無序刪除
	for (int i = (int)delayDataList_.size() - 1; i >= 0; --i) {
		if (delayDataList_[i].hasEmitted) {
			delayDataList_[i] = std::move(delayDataList_.back());
			delayDataList_.pop_back();
		}
	}
}

