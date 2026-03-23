#pragma once
#include "Object/Object.h"
#include <vector>
#include <functional>
#include "Vector3.h"
#include "kEngine.h"
#include "BaseScene.h"
#include "randomMaker.h"

class Particle{
public:
	Particle(kEngine* system);
	~Particle();

	virtual void Update(Camera* camera);
	virtual void Draw();

	/// ========== 全体設定 ==========///
	void SetCommonMaterialConfig(const MaterialConfig& material);

	void SetRootPos(Vector3 pos);
	void SetRootRotate(Vector3 pos);
	void SetRootScale(Vector3 pos);

	void AddObject();
	std::vector<Object*> GetObjectList();

	/// ========= ランダム関連 ==========///
	void SetSeed(unsigned int seed);
	int GetSeedGenerator() { return randomMaker_->GetSeedGenerator(); }

	void SetTimer(float maxTime);

protected:
	/// ============= データ内容 ===========///
	struct ParticleData {
		Timer lifeTimeTimer;
		Transform direction = CreateDefaultTransform();
		Vector3 velocity{ 0.0f,0.0f,0.0f };		
		float scaleSpeed = 1.0f;
		float rotateSpeed = 0.0f;
		Vector4 color{ 1.0f ,1.0f ,1.0f ,1.0f };
		Vector4 changeColor{ 0.0f ,0.0f ,0.0f ,0.0f };
		float size = 1.0f;
		Object* part{};
		bool isAlive = true;
	};
	std::vector<ParticleData*> particleObjectList_{};

	/// ========= 共通変数 ==========///
	kEngine* system_ = nullptr;				// 借り

	float defaultParticleInterval_ = 0.5f;

	ObjectPart anchorPart_;

	/// ========= エラーフラグ =========///
	bool isError_ = false;

protected:

	/// ========== 共通マテリアル ==========///
	std::shared_ptr<MaterialConfig>commonMaterialConfig;

	/// =========== ランダム関連 ===========///
	std::shared_ptr<RandomMaker> randomMaker_ = std::make_unique<RandomMaker>();
	Timer createTimer{};

};

