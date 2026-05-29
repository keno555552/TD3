#pragma once
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include "Object/Object.h"
#include "AnimationUnit.h"

class kEngine;
class AnimationManager
{
public:

	AnimationManager(kEngine* system);
	~AnimationManager();

	/// ====================== アニメーションシステムの基本的な流れ ========================///
	std::vector<int> LoadAnimation(const std::string& filePath);

	/// 時間を調整する
	void UnitSetTime(int unitHandle, float time);


	/// ======================= 操作関連 ========================///

	int TakeControlObject(int animationHandle, Object* object);

	int GetControlObjectPartHandle(ObjectData* object);

	Object* GetInstanceObjectByUnitHandle(int unitHandle);

	void Update();

private:

	/// ============= システム関連 ============== ///
	kEngine* system_{};

	/// ============= アニメーション関連 ============== ///
	struct AnimationClip {
		int modelDataHandle{};
		std::weak_ptr<ModelData> modelDataPtr;
		int animationIndex{};
		std::string filePath;
	};

	// アニメーションデータのリスト
	std::vector<AnimationClip> animationDataList_{};

	// Unitのリスト
	std::map<int, std::unique_ptr<AnimationUnit>> unitList_{};
	std::unordered_map<ObjectData*, int> objectToUnitHandleList_{};
	int unitHandleCounter_{};

private:
	
	/// 残るのはやり直しのために,Jsonの記録と読み込みはまだ支援しない
	static Animation LoadAnimationDataFromJson(const std::string& filePath);


	/// Unitのハンドルが存在するか確認する,存在しないならばエラーを出してfalseを返す
	bool CheckHaveHandle(int animHandle);
};

