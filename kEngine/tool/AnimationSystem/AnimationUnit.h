#pragma once
#include <vector>
#include <string>
#include <memory>
#include <map>
#include "Object/Object.h"
#include "Data/Animation/Animation.h"
#include "TimeManager/TimeManager.h"
#include "Data/Render/CPUData/ModelData.h"

class kEngine;
class AnimationUnit
{
public:

	AnimationUnit(kEngine* system);
	~AnimationUnit();

	/// ====================== アニメーションシステムの基本的な流れ ========================///
	/// アニメーションデータを読み込む
	AnimationUnit* ReadAnimationData(std::shared_ptr<ModelData> modelData,int animationIndex);

	/// 時間を調整する
	void SetTime(float time);

	/// ======================= 操作関連 ========================///

	void TakeControlObject(Object* object);
	void SetHaveSkeleton(bool have) { haveSkeleton_ = have; }
	bool GetHaveSkeleton() const { return haveSkeleton_; }

	Object* GetInstanceObject() const { return instanceObject_.get(); }

	void Update();

public:
	/// 外で調整できる変数
	struct AnimationBinding {
		std::vector<int> animToObject;
	};

	AnimationBinding animationBindings;

private:

	/// ============= システム関連 ============== ///
	kEngine* system_{};

	/// ============= アニメーション関連 ============== ///

	std::weak_ptr<ModelData> modelData_{};	// 借り
	int animationIndex_{};
	float allMaxTime_{};						// duration
	float allStartTime_{};
	bool isObjectChange_{};


	/// ============= 操作物体関連 ============== ///

	/// 操作しているオブジェクト
	Object* controlledObject_{};

	/// 計算用アニメーションのオブジェクト
	std::unique_ptr<Object> instanceObject_{};

	/// 計算用アニメーションのskeleton
	std::unique_ptr<Skeleton> instanceSkeleton_{};
	bool haveSkeleton_{};

	float nowTime_{};

private:

	/// ====================== プレイ関連 ======================== ///

	void UpdateLocalPose();
	void UpdateLocalMatrix();
	void UpdateGolbalMatrixAndFinalMatrix();

	/// ======================= ツール関数 ======================= ///
	float ChangeEasing(AnimationType type, float t, float rate = 0);

	void BindingAnimationNodeToObjectPart();

	/// 補間関連
	template<class T>
	T LerpValue(const T& a, const T& b, float t) {
		return a * (1.0f - t) + b * t;
	}

	// Vector3 專用
	template<>
	Vector3 LerpValue<Vector3>(const Vector3& a, const Vector3& b, float t) {
		return {
			linearity(a.x, b.x, t),
			linearity(a.y, b.y, t),
			linearity(a.z, b.z, t)
		};
	}

	// Quaternion 專用
	template<>
	Quaternion LerpValue<Quaternion>(const Quaternion& a, const Quaternion& b, float t) {
		return Slerp(a, b, t);
	}

	// main関数
	template<class T>
	T MakeTimeValue(const std::vector<KeyFrame<T>>& keys, float nowTime) {

		if (keys.empty()) {
			return T{};
		}

		if (keys.size() == 1) {
			return keys[0].value_;
		}

		if (nowTime <= keys.front().time_) {
			return keys.front().value_;
		}

		if (nowTime >= keys.back().time_) {
			return keys.back().value_;
		}

		for (size_t i = 0; i + 1 < keys.size(); i++) {
			if (keys[i].time_ <= nowTime && keys[i + 1].time_ >= nowTime) {

				float t = (nowTime - keys[i].time_) / (keys[i + 1].time_ - keys[i].time_);
				float easingT = ChangeEasing(keys[i].animationType_, t, keys[i].easeRate_);

				return LerpValue(keys[i].value_, keys[i + 1].value_, easingT);
			}
		}

		return keys.back().value_; // fallback
	}


};

