#pragma once
#include <vector>
#include "MathsIncluder.h"
#include <memory>
#include <string>
#include "LinearAlgebra/Transform.h"
#include "LinearAlgebra/TransformationMatrix.h"
#include "MaterialConfig.h"

class drawEngine;
class DrawDataCollector;
class ResourceManager;
class AnimationUnit;
class Object;
class ObjectPart {
public:

	friend class drawEngine;
	friend class DrawEngine;
	friend class DrawDataCollector;
	friend class ResourceManager;
	friend class AnimationUnit;
	friend class ObjectData;

	/// ============== 雑用データ ============== ///

	std::string name{};

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
	/// 描画のときに、DrawDataCollectorのどこにこskinningMatrixを入れるどころのハンドル
	int wellHandle = -1;

	/// 計算用のワールド行列 
	TransformationMatrix transformationMatrix{};

	/// アンカーポイントを考慮したワールド行列
	Matrix4x4 worldAnchorPointMatrix{};

};


class ObjectData {
public:
	std::vector<ObjectPart> objectParts_;

	ObjectPart mainPosition{};

	int modelHandle_ = 0;

	bool isDelete_ = false;

	bool isBillboard_ = false;

	Object* ownerObject = nullptr;	// ObjectDataをObject(皮)のポインタを持つと色々便利になるからやる

	ObjectPart* followObject_ = nullptr;
};

