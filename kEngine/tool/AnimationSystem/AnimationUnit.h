#pragma once
#include <vector>
#include "kEngine.h"
#include "Transform.h"
#include "Geometry/Collision/crashDecision.h"
#include "Object/Object.h"


enum class AnimationType {
	LINEARITY,
	EASY_IN,
	EASY_OUT,
	EASY_IN_OUT,
	EASY_IN_BACK,
	EASY_OUT_BACK,
};

struct KeyFrame {
	int index_{};
	float time_{};
	AnimationType animationType_{};
	float easeRate_{};
	Object transformData{};
	HitBox hitBox_{};
};

struct AnimationObjectData {
	int animationID{};
	std::vector <KeyFrame> keyList{};
	AnimationObjectData* parent_ = nullptr;

	Object SimpleObject{};
	void SetSimpleObject(const Object& obj);
	void AddKeyFrame(float time_ = 0);
};

class AnimationUnit
{
public:
	AnimationUnit(kEngine* system);
	~AnimationUnit();

	void ReadAnimationData(AnimationObjectData* animationData);
	void SetTime(float time_);
	void TakeControlObject(Object* object);
	void RelistControlObject();

	void Update(Camera* camera);

	void KariDraw();

	float GetStartTime() const { return allStartTime_; }
	float GetMaxTime() const { return allMaxTime_; }

private:
	kEngine* system_{};
	
	AnimationObjectData* animationData_{};
	float allMaxTime_{};
	float allStartTime_{};

	Object* controlledObject_{};             // 借り
	std::unique_ptr<Object> instanceObject_{};

	int usingKeyFrameIndex_{};
	float usingStartTime_{};
	float usingEndTime_{};
	float nowTime_{};
	std::unique_ptr <Timer> time_{};

private:
	bool CheckObjectNumMeet(Object* target);

	void TunningTime();
	void ResetTimer();

	void UpdateInstanceObject();
	void ControlleObject(Camera* camera);

	float ChangeEasing(AnimationType type,float R = 0);



#ifdef USE_IMGUI
	void ImguiPart();
#endif
};