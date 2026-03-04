#pragma once
#include "BasicResource.h"
#include "Data/DirectionalLightGPU.h"

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
	Microsoft::WRL::ComPtr<ID3D12Resource> GetVertexResource() { return vertexResource_->GetResource(); }
	Microsoft::WRL::ComPtr<ID3D12Resource> GetIndexResource_() { return indexResource_->GetResource(); };
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() { return vertexBufferView; }
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() { return indexBufferView; }
	int GetVertexNum() { return vertexNum_; }
	int GetIndexNum() { return indexNum_; }

protected:

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};
	BasicResource* vertexResource_ = new BasicResource;
	BasicResource* indexResource_ = new BasicResource;
	int vertexNum_ = 0;
	int indexNum_ = 0;
};


/// 将来のVertexResourceは図形ことで生成されるべき
/// 独立できリソースの頂点を管理するクラスであるべき