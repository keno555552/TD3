#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include "Particle/Particle.h"

//class P_Fire :public Particle
//{
//public:
//
//	void IntFire(kEngine* system);
//
//	void Update(Camera* camera)override;
//	void Draw()override;
//
//	int GetParticleNum() { return (int)fireDataList_.size(); }
//
//private:
//
//	struct FireData {
//		Timer lifeTimeTimer;
//		Vector3 direction{};
//		Vector4 color{ 1.0f ,1.0f ,1.0f ,1.0f };
//		float size = {};
//		ObjectPart* part{};
//		bool isAlive = true;
//	};
//
//	std::vector<FireData> fireDataList_{};
//
//private:
//	/// ============= fireデータ ===========///
//	float maxLifeTime = 3.0f;
//	float minLifeTime = 1.0f;
//	Vector2 unitMaxSize{ 1.0f,1.2f };
//	Vector2 unitMinSize{ 1.0f,1.0f };
//	Vector3	effectArea{ 100.0f, 100.0f ,100.0f};
//	Vector3 direction{ 100.0f, 100.0f ,100.0f};
//	float speed = 10.0f;
//	float speedReduce = 1.0f;
//	float createNumber = 1.0f;
//
//	int maxParticleNum = 500;
//
//private:
//
//	void CreateFire();
//	void UpdateFire();
//	void DeleteFire();
//
//#ifdef USE_IMGUI
//	void ImguiPart();
//#endif
//
//};

