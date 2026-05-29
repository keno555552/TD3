#pragma once
#include <vector>
#include <memory>
#include "Transform.h"
#include "TransformationMatrix.h"
#include "Data/Render/CPUData/MaterialConfig.h"
#include "MathsIncluder.h"
#include "Camera/Camera.h"
#include "Data/Render/CPUData/ObjectData.h"
#include "Geometry/Collision/crashDecision.h"

class kEngine;
class Object : public ObjectData {
public:
	~Object();

	void IntObject(kEngine* system);

	virtual void Update(Camera* camera = 0);

	virtual void Draw();

	int GetPartIndex()const { return (int)objectParts_.size(); }

	void CreateDefaultData();
	void CreateModelData(int modelHandle);

	///衝突したときめり込まない場合 
	virtual void Conflict(const std::vector<Object*>& obj);
	virtual  Object* Collision(const Object* obj);

	/// <summary>
	/// 複数衝突したオブジェクトのポインターを渡す
	/// </summary>
	/// <param name="obj"></param>
	/// <returns></returns>
	virtual  Object*  Collision(const std::vector<Object*>& obj);
	// obj の transform から AABB を計算して返す
    AABB GetAABB(const Object* obj) const;

	void CopyObject(Object* target);

protected:
	kEngine* system_ = nullptr; /// 借り
};
