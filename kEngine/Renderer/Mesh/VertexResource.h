#pragma once
#include "Resource/BasicResource.h"
#include "GPUData/DirectionalLightGPU.h"

D3D12_VERTEX_BUFFER_VIEW CreateVertexBufferView(ID3D12Resource* vertexResource, uint32_t NumOfVertex);
D3D12_INDEX_BUFFER_VIEW CreateIndexBufferView(ID3D12Resource* vertexResource, uint32_t NumOfVertex);

class MeshBuffer
{
public:
	~MeshBuffer();

	/// 頂点リソースの作成と解放
	virtual ID3D12Resource* CreateVertexResource_(ID3D12Device* device) { return 0; };
	virtual ID3D12Resource* CreateIndexResource_(ID3D12Device* device) { return 0; }
	void ClearAllResource();
	void ClearVertexResource();
	void ClearIndexResource();

	/// VertexBufferViewを作成する
	D3D12_VERTEX_BUFFER_VIEW CreateVertexBufferView_(uint32_t NumOfVertex);
	D3D12_INDEX_BUFFER_VIEW CreateIndexBufferView_(uint32_t NumOfIndex);

	/// Resourceを取得する
	Microsoft::WRL::ComPtr<ID3D12Resource> GetVertexResource() { return vertexResource_->GetResource(); };
	Microsoft::WRL::ComPtr<ID3D12Resource> GetIndexResource_() { return indexResource_->GetResource(); };
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() { return vertexBufferView; }
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() { return indexBufferView; }
	int GetVertexNum() { return vertexNum_; }
	int GetIndexNum() { return indexNum_; }

protected:

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};
	std::unique_ptr<BasicResource> vertexResource_ = std::make_unique<BasicResource>();
	std::unique_ptr<BasicResource> indexResource_ = std::make_unique<BasicResource>();
	int vertexNum_ = 0;
	int indexNum_ = 0;
};


/// TODO: vertex/indexリソースの作成をResourceManagerに移動するべき