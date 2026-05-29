#include "Model.h"
#include "filesystem"

void Model::SetModelData(std::shared_ptr<ModelData> modelData,uint32_t meshIndex) {
	modelData_ = modelData;
	meshIndex_ = meshIndex;
}

ID3D12Resource* Model::CreateVertexResource_(ID3D12Device* device) {

	MeshData& modelData = modelData_.lock()->meshDataList[meshIndex_];

	vertexResource_->CreateResourceClass_(device, sizeof(VertexData) * modelData.vertices.size());
	CreateVertexBufferView_(int(modelData.vertices.size()));
	VertexNum_ = int(modelData.vertices.size());

	// 頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	vertexResource_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
	vertexResource_->GetResource()->Unmap(0, nullptr);

	// スキニングデータがある場合、SkinClusterDataを準備する
	if (modelData_.lock()->haveSkinning) {
		int skinIndex = modelData_.lock()->meshDataList[meshIndex_].skinIndex;
		skinClusterData_.SetModelData(modelData_.lock(), skinIndex);
		skinClusterData_.CreateResourceClass(device)	;
		skinClusterData_.SetVertexInfluences();
	}

	vertexResource_->SetName("ModelVertexResource:" + modelData.name);

	return vertexResource_->GetResource().Get();
}

ID3D12Resource* Model::CreateIndexResource_(ID3D12Device* device) {

	MeshData& modelData = modelData_.lock()->meshDataList[meshIndex_];

	indexResource_->CreateResourceClass_(device, sizeof(uint32_t) * modelData.indices.size());
	CreateIndexBufferView_(int(modelData.indices.size()));
	indexNum_ = int(modelData.indices.size());

	// インデックスリソースにデータを書き込む
	uint32_t* indexData = nullptr;
	indexResource_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	std::memcpy(indexData, modelData.indices.data(), sizeof(uint32_t) * modelData.indices.size());
	indexResource_->GetResource()->Unmap(0, nullptr);

	indexResource_->SetName("ModelIndexResource:" + modelData.name);

	return indexResource_->GetResource().Get();
}

std::string Model::GetTexturePatch() {
	if (modelData_.expired())return std::string();
	int textureHandle = modelData_.lock()->meshDataList[meshIndex_].materialIndex;

	return modelData_.lock()->materialList[textureHandle].textureFilePath;
}

void Model::SetVertexNum(int num) {
	VertexNum_ = num;
}



