#pragma once
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include "materialConfig.h"
#include "DrawData/ObjectData.h"
#include "DrawData/SpriteData.h"
#include "PSOType.h"
#include "Camera/Camera.h"
#include "VertexResource.h"
#include "Render/Queue/RenderData.h"
#include "Data/LightGPU.h"
#include <cstdint>
#include "Model.h"
using ModelID = int;

inline const float layerDepth_Sprite = 0.0001f;
inline const float layeredSpriteDepth_ = 0.4f;

/// 2D = sprite
/// 3D = model
/// PC = CPUパーティクル

class LightManager;
class ResourceManager;
class InstanceManager;
class CameraManager;
class DrawDataCollector
{
public:

	void Initialize(CameraManager* cm,
		LightManager* lm);

	void Finalize();

	/// ============ カメラ関連 ==============///
	Vector3 GetCameraPosition() const;

	/// ============ 収集関連 ==============///

	void PreCollect();
	void EndCollect();

	/// ============== 2D関連 ===============///

	void Collect2D(SpriteData* sprite);

	std::unordered_map <PSOType,
		std::unordered_map <MaterialID,
		std::unordered_map <ModelID,
		std::vector<RenderData>>>
		>& GetOpaqueBuckets2D() { return opaqueBucket2D_; }

	std::vector<RenderData>& GetTransparentObjectParts2D() { return transparentBucket2D_; }

	/// ============== 3D関連 ===============///

	void Collect3D(ObjectData* object);

	std::unordered_map <PSOType,
		std::unordered_map <MaterialID,
		std::unordered_map <ModelID,
		std::vector<RenderData>>>
		>& GetOpaqueBuckets3D() { return opaqueBuckets3D_; }

	std::vector<RenderData>& GetTransparentObjectParts3D() { return transparentBucket3D_; }

	/// ========== パーティクル関連 ===========///

	void CollectParticleC(ObjectData* object);
	
	std::vector<RenderData>& GetBucketsParticleC() { return bucketParticleC_; }

	/// =========== Instance関連 ============///

	void SetInstanceList2D(TransformationMatrix* instancingList2D) { instancingList2D_ = instancingList2D; }
	void SetInstanceList3D(TransformationMatrix* instancingList3D) { instancingList3D_ = instancingList3D; }
	TransformationMatrix* GetInstancingList2D() { return instancingList2D_; }
	TransformationMatrix* GetInstancingList3D() { return instancingList3D_; }
	void BuildInstanceList2D();
	void BuildInstanceList3D();

	/// ============ Light関連 ==============///

	void UpdateLightData();
	std::vector<LightGPU> GetLightGPUBuffer();
	uint32_t GetLightCount();

private:

	/// ============ スプライト関連 ==============///
	/// ｚバッファ調整
	float SpriteLayerManagement(float zBuffer);
	/// フォローマトリックス作成
	Matrix4x4 MakeFollowObjectMatrix2D(SpriteData* sprite);
	/// WVP調整
	TransformationMatrix SpriteWVPAdjustment2D(SpriteData& sprite, SpritePart& part);
	/// バケット追加
	void AddSpriteToBucket2D(RenderData& renderData, int meshID);

	/// =============== 3D関連 ================///
	/// フォローマトリックス作成 
	Matrix4x4 MakeFollowObjectMatrix3D(ObjectData* object);
	/// WVP調整
	TransformationMatrix ObjectWVPAdjustment3D(ObjectData& object, ObjectPart& part, ModelData modelData);
	/// バケット追加
	void AddObjectToBucket3D(RenderData& renderData, int meshID);

	/// ============ パーティクル関連 ============///
	/// フォローマトリックス作成 
	// 3Dのフォローマトリックスを流用
	/// WVP調整
	TransformationMatrix ObjectWVPAdjustmentPC(ObjectData& object, ObjectPart& part, ModelData modelData);
	/// バケット追加
	void AddObjectToBucketPC(RenderData& renderData);

	/// ============ マテリアル関連 ==============///

	uint32_t PSODecision(MaterialConfig& material);

private:
	InstanceManager* instanceManager_ = nullptr; /*借り*/

	/// ============ Light関連 ==============///
	LightManager* lightManager_ = nullptr; /*借り*/

	/// ============ カメラ関連 ==============///
	CameraManager* cameraManager_ = nullptr; /*借り*/

	/// ==================== 2Dデータ ====================///
	/// 不透明オブジェクトバケット
	std::unordered_map <PSOType,
		std::unordered_map <MaterialID,
		std::unordered_map <ModelID,
		std::vector<RenderData>>>
		> opaqueBucket2D_;


	/// 透明オブジェクトリスト
	std::vector<RenderData> transparentBucket2D_;

	/// 2Dスブライドカンター
	int simpleSpriteCounter_{};
	int unlayeredSpriteCounter_{};

	/// ==================== 3Dデータ ====================///
	/// 不透明オブジェクトバケット
	std::unordered_map <PSOType,
		std::unordered_map <MaterialID,
		std::unordered_map <ModelID,
		std::vector<RenderData>>>
		> opaqueBuckets3D_;

	/// 透明オブジェクトバケット
	std::vector<RenderData> transparentBucket3D_;

	/// ========== パーティクル関連 ===========///

	/// パーティクルバケット
	std::vector<RenderData> bucketParticleC_;

	/// ================ インスタンスデータ =================///

	TransformationMatrix* instancingList2D_ = nullptr;
	TransformationMatrix* instancingList3D_ = nullptr;
	TransformationMatrix* instancingListParticleC_ = nullptr;

	int instanceCounter2D_ = 0;
	int instanceCounter3D_ = 0;
	int instanceCounterParticleC_ = 0;

};