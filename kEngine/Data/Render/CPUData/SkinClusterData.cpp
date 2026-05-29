#include "SkinClusterData.h"
#include "Renderer/Mesh/VertexResource.h"


void SkinClusterData::SetModelData(std::shared_ptr<ModelData> modelData,int modelIndex) {
	modelData_ = modelData;
	modelIndex_ = modelIndex;
	vertexNum_ = (int)modelData->meshDataList[modelIndex].vertices.size();
}

void SkinClusterData::CreateResourceClass(ID3D12Device* device) {
	
	/// まず初期化
	skinClusterResource_.Reset();

	/// ID3D12Resourceを作成する
	skinClusterResource_ = CreateResource(device, vertexNum_);
	
	/// 新しいVertexInfluenceを作って、spanを作る
	VertexInfluence* newInfluence = nullptr;
	skinClusterResource_->Map(0, nullptr, reinterpret_cast<void**>(&newInfluence));
	std::memset(newInfluence, 0, sizeof(VertexInfluence) * vertexNum_);
	vertexInfluences = { newInfluence , (size_t)vertexNum_ };

	/// VertexInfluenceのspanを作る
	influenceBufferView = CreateVertexBufferView(skinClusterResource_.Get(), sizeof(VertexInfluence) * vertexNum_);

	/// InverseBindPoseMatrixを格納する場所を作る、単位行列で埋める
	inverseBindPoseMatrices.resize(modelData_.lock()->skeleton.jointList.size(), Identity());
}

void SkinClusterData::SetVertexInfluences() {
	
	/// まずModelDataからこのskinのindexを見つける
	const auto& modelData = modelData_.lock();
	int skinIndex = modelData->meshDataList[modelIndex_].skinIndex;

	/// skinIndexをもとに、目標のSkinDataを見つける
	const SkinData& skinData = modelData->skinListList[skinIndex];

	/// skinDataに回って、行動を行う
	for (int jointIndex = 0; jointIndex < skinData.jointNames.size(); ++jointIndex) {

		/// jointMapにskinDataのjointIndexがあるか確認する
		auto checker = modelData->skeleton.jointMap.find(skinData.jointNames[jointIndex]);
		/// 見つからなければ次に行く
		if (checker == modelData->skeleton.jointMap.end()) continue;

		/// (*it).secondはjointのindexに対応のinverseBindPoseMatrixを入れる
		inverseBindPoseMatrices[(int)(*checker).second] = skinData.inverseBindPoseMatrices[jointIndex];
		/// skinDataのvertexWeightsを見て、vertexInfluencesに入れていく
		for (const auto& vertexWeight : skinData.skinClusterData[jointIndex].vertexWeights) {
			auto& currectInfluence = vertexInfluences[vertexWeight.vertexIndex];
			for (uint32_t i = 0; i < kNumMaxInflunce; ++i) {
				if(currectInfluence.weights[i] == 0.0f) {
					currectInfluence.weights[i] = vertexWeight.weight;
					currectInfluence.jointIndices[i] = (int)(*checker).second;
					break;
				}
			}
		}
	}
}


std::string SkinClusterData::GetTexturePatch() {
	return std::string();
}

void SkinClusterData::SetVertexNum(int num) {
	vertexNum_ = num;
}

// Note：老師在這裡特意教了std::generate
// 這個函數可以用來生成一個範圍內的連續整數序列，非常適合用來初始化vertexInfluences中的jointIndex，讓它們從0開始遞增。
// std::generate(inverseBindPoseMatrices.begin(), inverseBindPoseMatrices.end(), []() { return Identity(); });
// 當中數字1是起始點，2是終點（到前一個為止），
// 3是函數，如果單純想生成1就寫[]() { return 1; }
// 這裡Identity()是一個函數，會返回一個單位矩陣。所以直接填上Identity()也照樣能正常運行。