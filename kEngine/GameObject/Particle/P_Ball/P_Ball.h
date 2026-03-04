#pragma once
#include "../Particle.h"

class P_Ball :public Particle
{
public:

	P_Ball(kEngine* system);
	~P_Ball();

	void Update(Camera* camera)override;
	void Draw()override;

	void SetTextureHandle(int handle);
	int GetParticleNum() { return (int)particleObjectList_.size(); }

private:
	/// ============= ボールデータ ===========///
	float maxLifeTime_ = 3.0f;
	float minLifeTime_ = 1.0f;
	Vector2 unitMaxSize_{ 1.0f,1.2f };
	Vector2 unitMinSize_{ 1.0f,1.0f };
	Vector3	effectArea_{ 100.0f, 100.0f ,100.0f };
	Vector3 direction_{ 100.0f, 100.0f ,100.0f };
	float speed_ = 1.0f;
	float speedReduce_ = 1.0f;
	float createNumber_ = 3.0f;
	float lifeTime_ = 2.0f;

	int maxParticleNum = 500;

	/// ========== デフォルトマテリアル ==========///
	int defaultTextureHandle = -1;

	int defaultModelHandle = -1;

private:

	void CreateBall();
	void UpdateBall();
	void DeleteBall();

#ifdef USE_IMGUI
	void ImguiPart();
#endif

};

