#pragma once
#include <vector>
#include <memory>
#include "MathsIncluder.h"
#include "material.h"
#include "materialConfig.h"
#include "TransformationMatrix.h"
//#include "SpriteInstance.h"
//#include "ModelInstance.h"
#include "Transform.h"

#include <algorithm>
#include <map>
#include <unordered_map>
#include <Data/Render/CPUData/ObjectData.h>

class ObjectPart;
class InstanceManager
{
public:

	void Finalize();

	void Update();

	//void AddSpriteInstance(Vector2 pos, MaterialConfig material);
	void AddModelInstance(TransformationMatrix* wvpData, MaterialConfig material, int vertexNum, int modelHandle = 0, bool useDefaultModel = false);
	void Add2DTileInstance(Vector2 pos, MaterialConfig material);
	//void RemoveSpriteInstance();
	//void UpdateTileInstance();
	void Add3DTileInstance(TransformationMatrix* wvpData, MaterialConfig material, int vertexNum, int modelHandle = 0, bool useDefaultModel = false);

	void Add2DInstance(Transform wvpData, MaterialConfig material, CornerData cornerData = {}, Vector2 anchorPoint = {}, Vector2 cropLT = {}, Vector2 cropSize = {});
	void Add3DInstance(TransformationMatrix wvpData, MaterialConfig material, int vertexNum, int modelHandle = 0, int modelGroupHandle = 0);
	void Add3DInstance(ObjectData* object);
	void SpriteLayerManagement();
	//void RemoveModelInstance();
	//void UpdateModelInstance();

public:

	enum DrawState {
		STANDBY,
		ISDRAW,
		DRAWLAST,
	};

public:
	std::vector< MaterialConfig* > materialConfigList_;
	std::vector< SpriteInstance* > spriteList_;
	std::vector< SpriteInstance* > tile2DList_;
	std::vector< ModelInstance*> modelList_;
	std::vector< ModelInstance*> tile3DList_;




public:
	int tileLayerCount = 0;
	int spriteLayerCount = 0;

	float layeredSpriteDepth_ = 0.4f;

private:

	template<typename TInstance>
	void ClearInstance(std::vector< TInstance*>& list) {
		for (auto& ptr : list) {
			if (ptr) {
				delete ptr;
				ptr = nullptr;
			}
		}
		list.clear();
	}

	template<typename TInstance>
	void UpdateInstance(std::vector<TInstance*>& list) {
		std::erase_if(list, [](TInstance* ptr) {
			if (!ptr) return false;

			if (ptr && ptr->drawState == DRAWLAST) {
				delete ptr;
				return true;
			}
			if (ptr && ptr->drawState == ISDRAW) {
				ptr->drawState = DRAWLAST;
			}
			return false;
			}
		);
	}
};

// このクラムは、今は使われていないが、将来的にStage内でのオブジェクト管理に使う予定。ObjectDataをInstanceManagerで管理するイメージ