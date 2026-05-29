#pragma once
#include "Tool/EffectSystem/ParticleSystem/ParticleManager.h"

class kEngine;
class EffectManager
{
public:

	/// --- コンストラクタ・デストラクタ --- ///
	EffectManager() = default;
	~EffectManager() = default;

	/// --- システム管理 --- ///
	void Initialize(kEngine* system);
	void Finalize();

	/// --- 基本行為 --- ///
	void Update();
	void Draw();

	/// --- エフェクトマネージャーをとる --- ///
	ParticleManager* GetParticleManager() { return particleManager_.get(); }

private:

	/// 借りのDevice
	kEngine* system_ = nullptr;

	/// 持ってるドライブ
	std::unique_ptr<ParticleManager> particleManager_;

};

