#include "PlaneMesh.h"

ID3D12Resource* PlaneMesh::CreateVertexResource_(ID3D12Device* device) {
	vertexResource_->CreateResourceClass_(device, sizeof(VertexData) * 4);
	CreateVertexBufferView_(4);

	VertexData* v = nullptr;
	vertexResource_->GetResource()->Map(0, nullptr, (void**)&v);


	v[0].position = p[TOP_LEFT];
	v[0].texcoord = { 0.0f, 0.0f };

	v[1].position = p[TOP_RIGHT];
	v[1].texcoord = { 1.0f, 0.0f };

	v[2].position = p[BOTTOM_LEFT];
	v[2].texcoord = { 0.0f, 1.0f };

	v[3].position = p[BOTTOM_RIGHT];
	v[3].texcoord = { 1.0f, 1.0f };

	for (int i = 0; i < 4; i++) {
		v[i].normal = { 0.0f, 0.0f, 1.0f };
	}

	MeshBuffer::vertexNum_ = 4;

	vertexResource_->GetResource()->Unmap(0, nullptr);

	vertexResource_->SetName("PlaneVertexResource");

	return vertexResource_->GetResource().Get();
}

ID3D12Resource* PlaneMesh::CreateIndexResource_(ID3D12Device* device) {
	indexResource_->CreateResourceClass_(device, sizeof(uint32_t) * 6);
	CreateIndexBufferView_(6);

	indexBufferView = {};
	/// リソースの先頭のアドレスから使う
	indexBufferView.BufferLocation = indexResource_->GetResource()->GetGPUVirtualAddress();
	/// 使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferView.SizeInBytes = sizeof(uint32_t) * 6;
	///　インデックスはuint32_tとする
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	uint32_t* indexDataPlane = nullptr;
	indexResource_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&indexDataPlane));

	// 順番を指定
	indexDataPlane[0] = 0;
	indexDataPlane[1] = 1;
	indexDataPlane[2] = 2;

	indexDataPlane[3] = 1;
	indexDataPlane[4] = 3;
	indexDataPlane[5] = 2;

	indexResource_->GetResource()->Unmap(0, nullptr);

	MeshBuffer::indexNum_ = 6;

	indexResource_->SetName("PlaneIndexResource");

	return indexResource_->GetResource().Get();
}

