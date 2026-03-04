#pragma once
#include "Object/Object.h"
#include "Object/Sprite.h"
#include <vector>

class DebugObject :public Object
{
public:
	DebugObject(kEngine* kEngine);
	~DebugObject();

	void Update(Camera* camera) override;
	void Draw() override;

	void SetFollowObject(ObjectPart* followObject);
	void ClearFollowObject();

	void SetShowCenterPoint(bool isShow);
	void SetShowNumber(bool isShow);
	void UpdateShowNumber(int Number);

private:
	/// ============= デバッグ用更新関数 ============= ///
	/// ///////全体更新///////// ///
	void updateDebugParts();
	/// ターゲットポジション更新 ///
	void updateTargetPosition();
	/// 中心点更新 ///
	void updateCenterPoint();
	/// 数字更新 ///
	void updateCenterNumber();

	/// 更新検査 ///
	bool isNeedUpdate();

private:
	
	/// ============= カメラ ============= ///
	Camera* camera_ = nullptr;	/// 借り

	/// ========= ターゲットポート ========= ///
	Vector3 targetPosition_{};

	/// ============= ハンドル ============= ///
	/// モデル ///

	/// テキスチャ ///
	int TH_centerPoint{};
	int TH_number{};


	/// ============= デバッグ用パーツ ============= ///
	SimpleSprite* centerPoint_ = nullptr;
	SimpleSprite* centerNumberSprite_ = nullptr;

	/// ============= フラグ ============= ///
	bool isShowCenterPoint_ = false;

	bool isCenterNumber_ = false;
	std::vector<int> centerNumberArray;
	int centerNumber_ = 0;


};

