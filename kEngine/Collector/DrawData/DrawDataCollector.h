#pragma once
#include <vector>
#include <memory>
#include <map>
#include <span>
#include <unordered_map>
#include "Data/Render/CPUData/materialConfig.h"
#include "Data/Render/CPUData/ObjectData.h"
#include "Data/Render/CPUData/SpriteData.h"
#include "Camera/Camera.h"
#include "Mesh/VertexResource.h"
#include "Data/Render/Queue/RenderData.h"
#include "Data/Render/Queue/ParticleInstanceForDDC.h"

#include "Tool/EffectSystem/ParticleSystem/ParticleInstance.h"
#include "GPUData/LightGPU.h"
#include <cstdint>
#include "Mesh/Model.h"
#include "Data/Render/CPUData/DebugLine.h"
#include "Data/Render/GPUData/DebugLineGPU.h"
#include "AnimationSystem/AnimationManager.h"

#include "Data/Render/GPUData/WellForGPU.h"
#include "Data/Render/CPUData/VertexInfluence.h"
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

	void Initialize(
		CameraManager* cm,
		LightManager* lm,
		AnimationManager* am);

	void Finalize();

	/// ============ カメラ関連 ==============///
	Vector3 GetCameraPosition() const;

	/// ============ 収集関連 ==============///

	void PreCollect();
	void EndCollect();

	/// =========== DebugLine 関連 ============///

	void CollectDebugLine(DebugLine* debugLine);

	std::vector<DebugLineVertexGPU>& GetDebugLineVertexBucket() { return debugLinesVertexBucket_; }

	/// ============== 2D関連 ===============///

	void Collect2D(SpriteData* sprite);

	std::unordered_map <PSOKey,
		std::unordered_map <MaterialID,
		std::unordered_map <ModelID,
		std::vector<RenderData>>>
		>& GetOpaqueBuckets2D() { return opaqueBucket2D_; }

	std::vector<RenderData>& GetTransparentObjectParts2D() { return transparentBucket2D_; }

	/// ============== 3D関連 ===============///

	void Collect3D(ObjectData* object);

	std::unordered_map <PSOKey,
		std::unordered_map <MaterialID,
		std::vector<RenderData>>
		>& GetOpaqueBuckets3D() { return opaqueBucket3D_; }

	std::vector<RenderData>& GetTransparentObjectParts3D() { return transparentBucket3D_; }


	/// ========== パーティクル関連 ===========///

	void CollectParticle(std::vector<ObjectData>& objectList, std::vector<ParticleInstance>& instance);

	std::unordered_map<MeshBuffer*,
		std::unordered_map<PSOKey,
		std::unordered_map<MaterialID,
		std::vector<TransformationMatrix>>>
		>& GetParticleBucket() { return bucketParticleC_; }

	/// ================= skinning関連 ===================///

	/// NOTE: これも一時的な実装。まだUnloadがない
	int SetSkinningData(WellForGPU* mappedPalette, int mappedNum, VertexInfluence* influenceSpan, int VertexNum);
	void ClearSkinningData(int index);

	std::span<VertexInfluence> GetInfluenceSpan(int index) { return skinningDataList_[index].influenceSpan; }
	std::span<WellForGPU> GetMappedPalette(int index) { return skinningDataList_[index].mappedPalette; }

	/// =========== Instance関連 ============///

	void SetInstanceListDL(TransformationMatrix* instancingListDL) { instancingListDL_ = instancingListDL; }
	void SetInstanceList2D(TransformationMatrix* instancingList2D) { instancingList2D_ = instancingList2D; }
	void SetInstanceList3D(TransformationMatrix* instancingList3D) { instancingList3D_ = instancingList3D; }
	void SetInstanceListParticle(TransformationMatrix* instancingListParticleC) { instancingListParticleC_ = instancingListParticleC; }
	TransformationMatrix* GetInstanceListDL() { return instancingListDL_; }
	TransformationMatrix* GetInstancingList2D() { return instancingList2D_; }
	TransformationMatrix* GetInstancingList3D() { return instancingList3D_; }
	TransformationMatrix* GetInstancingListParticleC() { return instancingListParticleC_; }
	void BuildInstanceList2D();
	void BuildInstanceList3D();
	void BuildInstanceListParticle();

	/// ============ Light関連 ==============///

	void UpdateLightData();
	std::vector<LightGPU> GetLightGPUBuffer();
	uint32_t GetLightCount();

private:
	InstanceManager* instanceManager_ = nullptr; /*借り*/

	/// ============ Light関連 ==============///
	LightManager* lightManager_ = nullptr; /*借り*/

	/// ============ カメラ関連 ==============///
	CameraManager* cameraManager_ = nullptr; /*借り*/

	/// ============ アニメーション関連 ==============///
	AnimationManager* animationManager_ = nullptr; /*借り*/

	/// ================ debugDraw関連 =================///

	std::vector<DebugLineVertexGPU> debugLinesVertexBucket_;
	int maxDebugLineInstance = config::GetDebugLineNumInstance();

	/// ==================== 2Dデータ ====================///
	/// 不透明オブジェクトバケット
	std::unordered_map <
		PSOKey,
		std::unordered_map <MaterialID,
		std::unordered_map <ModelID,
		std::vector<RenderData>
		>>> opaqueBucket2D_;


	/// 透明オブジェクトリスト
	std::vector<RenderData> transparentBucket2D_;

	/// 2Dスブライドカンター
	int simpleSpriteCounter_{};
	int unlayeredSpriteCounter_{};

	/// ==================== 3Dデータ ====================///
	/// 不透明オブジェクトバケット
	std::unordered_map <
		PSOKey,
		std::unordered_map <MaterialID,
		std::vector<RenderData>
		>> opaqueBucket3D_;

	/// 透明オブジェクトバケット
	std::vector<RenderData> transparentBucket3D_;

	/// ========== パーティクル関連 ===========///

	/// パーティクルバケット
	std::unordered_map<
		MeshBuffer*,
		std::unordered_map<PSOKey,
		std::unordered_map<MaterialID,
		std::vector<TransformationMatrix>
		>>> bucketParticleC_;

	/// ================= skinning関連 ====================///

	struct SkinningData
	{
		std::span<VertexInfluence> influenceSpan;
		std::span<WellForGPU> mappedPalette;
	};

	std::vector<SkinningData> skinningDataList_;

	/// ================ インスタンスデータ =================///

	TransformationMatrix* instancingListDL_ = nullptr;
	TransformationMatrix* instancingList2D_ = nullptr;
	TransformationMatrix* instancingList3D_ = nullptr;
	TransformationMatrix* instancingListParticleC_ = nullptr;

	int instanceCounterDL_ = 0;
	int instanceCounter2D_ = 0;
	int instanceCounter3D_ = 0;
	int instanceCounterParticleC_ = 0;

private:

	/// =========== DebugLine 関連 ============///

	TransformationMatrix DLWVPAdjustment(DebugLine* debugLine);

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
	/// Animationマトリックス作成
	Matrix4x4 MakeAnimationMatrix(ObjectPart& part);
	/// WVP調整
	TransformationMatrix ObjectWVPAdjustment3D(ObjectData& object, ObjectPart& part, ModelData* modelData, int modelCounter);
	/// バケット追加
	void AddObjectToBucket3D(RenderData& renderData, int meshID);
	/// rotateのダーティフラグを確認して、必要ならば回転行列を作成する
	void DirtyEulerToQuat(ObjectPart& part);
	void DirtyEulerToQuat(ObjectData& part);

	/// ================= skinning関連 ===================///

	/// ============ パーティクル関連 ============///
	/// フォローマトリックス作成 
	// 3Dのフォローマトリックスを流用
	/// WVP調整
	TransformationMatrix ObjectWVPAdjustmentPC(ObjectData& object, ModelData& modelData, int modelCounter, ParticleInstance particleData);
	/// バケット追加
	void AddObjectToBucketPC(ParticleInstanceForDDC& renderData);

	/// ============ マテリアル関連 ==============///

	PSOKey PSODecision(MaterialConfig& material);


};