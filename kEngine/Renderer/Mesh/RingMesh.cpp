#include "RingMesh.h"

ID3D12Resource* RingMesh::CreateVertexResource_(ID3D12Device* device) {

	/// 頂点数を計算して、リソースを作成する
	int vertexCount = kRingDivide_ * 4;

	vertexResource_->CreateResourceClass_(device, sizeof(VertexData) * vertexCount);
	CreateVertexBufferView_(vertexCount);

	VertexData* v = nullptr;
	vertexResource_->GetResource()->Map(0, nullptr, (void**)&v);

	if (kRingDivide_ < 3) {
		// エラー処理
		Logger::Log("[kError]RingMesh:CreateVertexResource() RingDivide must be greater than 2!");
		return nullptr;
	}

	if (kOuterRadius_ < kInnerRadius_) {
		// エラー処理
		std::swap(kOuterRadius_, kInnerRadius_);
		Logger::Log("[kError]RingMesh:CreateVertexResource() OuterRadius must be greater than InnerRadius! But it was automatically fixed by swapping the values.");
	}

	if (kOuterRadius_ == kInnerRadius_) {
		// エラー処理
		Logger::Log("[kError]RingMesh:CreateVertexResource() OuterRadius must be greater than InnerRadius!");
		return nullptr;
	}

	float radianPerDivide = 2.0f * 3.1415926f / float(kRingDivide_);

	for (uint32_t index = 0; index < (uint32_t)kRingDivide_; ++index) {
		float sin = std::sin(index * radianPerDivide);
		float cos = std::cos(index * radianPerDivide);
		float sinNext = std::sin((index + 1) * radianPerDivide);
		float cosNext = std::cos((index + 1) * radianPerDivide);
		float u = float(index) / float(kRingDivide_);
		float uNext = float(index + 1) / float(kRingDivide_);

		// position と uv。normal は必要ならそこで設定する
		uint32_t base = index * 4;

		v[base + 0] = { { -sin * kOuterRadius_, cos * kOuterRadius_, 0.0f, 1.0f }, { u,     0.0f } };
		v[base + 1] = { { -sinNext * kOuterRadius_, cosNext * kOuterRadius_, 0.0f, 1.0f }, { uNext, 0.0f } };
		v[base + 2] = { { -sin * kInnerRadius_, cos * kInnerRadius_, 0.0f, 1.0f }, { u,     1.0f } };
		v[base + 3] = { { -sinNext * kInnerRadius_, cosNext * kInnerRadius_, 0.0f, 1.0f }, { uNext, 1.0f } };

		for (int i = 0; i < 4; ++i) {
			v[base + i].normal = { 0.0f, 0.0f, -1.0f };
		}
	}

	MeshBuffer::vertexNum_ = vertexCount;

	vertexResource_->GetResource()->Unmap(0, nullptr);

	vertexResource_->SetName("VertexResource:Ring");

	return vertexResource_->GetResource().Get();
}

ID3D12Resource* RingMesh::CreateIndexResource_(ID3D12Device* device) {
	uint32_t IndexNum = kRingDivide_ * 6;
	indexResource_->CreateResourceClass_(device, sizeof(uint32_t) * IndexNum);
	CreateIndexBufferView_(IndexNum);
	vertexNum_ = IndexNum;

	indexBufferView = {};
	/// リソースの先頭のアドレスから使う
	indexBufferView.BufferLocation = indexResource_->GetResource()->GetGPUVirtualAddress();
	/// 使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferView.SizeInBytes = sizeof(uint32_t) * IndexNum;
	///　インデックスはuint32_tとする
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	uint32_t* indexData = nullptr;
	indexResource_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
	// 順番を指定
	for (int Index = 0; Index < kRingDivide_; Index++) {
			indexData[0 + Index * 6] = Index * 4 + 2; // a
			indexData[1 + Index * 6] = Index * 4 + 1; // b
			indexData[2 + Index * 6] = Index * 4 + 0; // c
			indexData[3 + Index * 6] = Index * 4 + 1; // c
			indexData[4 + Index * 6] = Index * 4 + 2; // b
			indexData[5 + Index * 6] = Index * 4 + 3; // d
	}

	indexResource_->GetResource()->Unmap(0, nullptr);

	MeshBuffer::indexNum_ = IndexNum;

	indexResource_->SetName("IndexResource:Ring");

	return indexResource_->GetResource().Get();
}