#pragma once
#include <vector>
#include "MathsIncluder.h"
#include <memory>
#include "Transform.h"
#include "TransformationMatrix.h"
#include "MaterialConfig.h"

class drawEngine;
class DrawDataCollector;
class ResourceManager;
class AnimationUnit;
class ObjectPart {
public:

	friend class drawEngine;
	friend class DrawDataCollector;
	friend class ResourceManager;
	friend class AnimationUnit;

	/// ============= 基本変換情報 ============= ///

	Transform transform = CreateDefaultTransform();

	std::shared_ptr<MaterialConfig>materialConfig;

	ObjectPart* parentPart = nullptr;

	Vector3 forward = { 0.0f,0.0f,0.0f };

	bool operator==(const ObjectPart& target);

	/// =============== 更新関数 =============== ///

	Matrix4x4 UpdateWorldMatrix();

	/// ========== エンジン側用データ =========== ///

private:
	TransformationMatrix transformationMatrix{};
	Matrix4x4 worldAnchorPointMatrix{};

};


class ObjectData {
public:
	std::vector<ObjectPart> objectParts_;

	ObjectPart mainPosition{};

	int modelHandle_ = 0;

	bool isDelete_ = false;

	bool isBillboard_ = false;

	ObjectPart* followObject_ = nullptr;
};

