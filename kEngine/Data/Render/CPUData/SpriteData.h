#pragma once
#include <vector>
#include "MathsIncluder.h"
#include <memory>
#include "Transform.h"
#include "TransformationMatrix.h"
#include "MaterialConfig.h"
#include "Data/Render/CPUData/ObjectData.h"
#include "Data/Render/CPUData/CornerData.h"

struct SpritePart
{
	///================= デイタ保存 =================///
	Transform transform = CreateDefaultTransform();

	Transform worldTransform = CreateDefaultTransform();

	/// 中心点調整
	Vector2 anchorPoint{};
	Vector2 worldAnchorPoint{};

	/// 四角デイタ
	CornerData conerData{};

	/// UVCrop用
	Vector2 cropLT{};
	Vector2 cropSize{};

	///=============== 計算、資料渡し用 ==============///
	std::shared_ptr<MaterialConfig>materialConfig;

	SpritePart* parentPart = nullptr;
};


#pragma region SimpleSprite
struct SimpleSpritePart : SpritePart
{
	
};

class SpriteData {
public:
	std::vector<SpritePart> objectParts_{};
	SpritePart mainPosition{};
	int modelHandle_ = 0;
	bool isDelete_ = false;
	SpriteData* followObject_{};
};

class SimpleSpriteData : public SpriteData {
public:

};
#pragma endregion



#pragma region DeformableSprite
struct DeformableSpriteVertex {
	Vector2 LTpos;
	Vector2 LBpos;
	Vector2 RTpos;
	Vector2 RBpos;
	float TsizeX;
	float TsizeY;
};

struct DeformableSpritePart : SpritePart
{

};

class DeformableSpriteData : public SpriteData {
public:
	std::vector<DeformableSpritePart> objectParts_;
	DeformableSpritePart mainPosition;
	int modelHandle_ = 0;
	bool isDelete_ = false;
	SpritePart* followObject_;
};
#pragma endregion