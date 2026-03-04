#include "VertexResource.h"
#include "Vector4.h"
#include "VertexData.h"
#include "TransformationMatrix.h"


Microsoft::WRL::ComPtr< ID3D12Resource> CreateWVPResource(ID3D12Device* device, size_t sizeInBytes) {
	return CreateResource(device, sizeInBytes);
}

D3D12_VERTEX_BUFFER_VIEW CreateVertexBufferView(ID3D12Resource* vertexResource, uint32_t NumOfVertex) {

	// 頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	// リソースの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点分のサイズ
	vertexBufferView.SizeInBytes = sizeof(VertexData) * NumOfVertex;
	// 1頂点あたりのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	return vertexBufferView;
}

D3D12_INDEX_BUFFER_VIEW CreateIndexBufferView(ID3D12Resource* vertexResource, uint32_t NumOfIndex) {
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};
	/// リソースの先頭のアドレスから使う
	indexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	/// 使用するリソースのサイズはインデックス頂点分のサイズ
	indexBufferView.SizeInBytes = sizeof(uint32_t) * NumOfIndex;
	///　インデックスはuint32_tとする
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	return indexBufferView;
}

MeshBuffer::~MeshBuffer() {
	ClearAllResource();
	delete vertexResource_;
	delete indexResource_;
}

D3D12_VERTEX_BUFFER_VIEW MeshBuffer::CreateVertexBufferView_(uint32_t NumOfVertex) {
	vertexNum_ = NumOfVertex;
	return vertexBufferView = CreateVertexBufferView(vertexResource_->GetResource().Get(), NumOfVertex);
}

D3D12_INDEX_BUFFER_VIEW MeshBuffer::CreateIndexBufferView_(uint32_t NumOfIndex) {
	indexNum_ = NumOfIndex;
	return indexBufferView = CreateIndexBufferView(indexResource_->GetResource().Get(), NumOfIndex);
}

void MeshBuffer::ClearAllResource() {
	ClearVertexResource();
	ClearIndexResource();
}

void MeshBuffer::ClearVertexResource() {
	if (vertexResource_->GetResourceCounter() > 0)vertexResource_->ClearResource();
}

void MeshBuffer::ClearIndexResource() {
	if (indexResource_->GetResourceCounter() > 0)indexResource_->ClearResource();
}


