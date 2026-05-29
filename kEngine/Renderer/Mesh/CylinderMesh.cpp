#include "CylinderMesh.h"

#include "RingMesh.h"

ID3D12Resource* CylinderMesh::CreateVertexResource_(ID3D12Device* device) {

	const float radianPerDivide = 2.0f * 3.1415926f / float(kCylinderDivide_);

	/// 頂点数を計算して、リソースを作成する
	int vertexCount = kCylinderDivide_ * 4;

	vertexResource_->CreateResourceClass_(device, sizeof(VertexData) * vertexCount);
	CreateVertexBufferView_(vertexCount);

	VertexData* v = nullptr;
	vertexResource_->GetResource()->Map(0, nullptr, (void**)&v);

	int halfDivide = kCylinderDivide_ / 2;

	for (uint32_t index = 0; index < (uint32_t)kCylinderDivide_; ++index) {


		float sin = std::sin(index * radianPerDivide);
		float cos = std::cos(index * radianPerDivide);
		float sinNext = std::sin((index + 1) * radianPerDivide);
		float cosNext = std::cos((index + 1) * radianPerDivide);
		float u = float(index) / float(kCylinderDivide_);
		float uNext = float(index + 1) / float(kCylinderDivide_);

		// position と uv。normal は必要ならそこで設定する
		uint32_t base = index * 4;

		// position, texcoord, normal
		v[base + 0] = { { -sin * kTopRadius, kHeight, cos * kTopRadius, 1.0f }			 , { u, 0.0f }		, { -sin		, 0.0f, cos } };
		v[base + 1] = { { -sin * kBottomRadius, 0.0f, cos * kBottomRadius, 1.0f }		 , { u, 1.0f }		, { -sin		, 0.0f, cos } };
		v[base + 2] = { { -sinNext * kTopRadius, kHeight, cosNext * kTopRadius, 1.0f }	 , { uNext, 0.0f }	, { -sinNext	, 0.0f, cosNext } };
		v[base + 3] = { { -sinNext * kBottomRadius, 0.0f, cosNext * kBottomRadius, 1.0f } , { uNext, 1.0f }	, { -sinNext	, 0.0f, cosNext } };

		if (isReverseY_) {
			std::swap(v[base + 0].texcoord.y ,v[base + 1].texcoord.y);
			std::swap(v[base + 2].texcoord.y ,v[base + 3].texcoord.y);
		}


	}

	vertexResource_->GetResource()->Unmap(0, nullptr);



	MeshBuffer::vertexNum_ = vertexCount;

	vertexResource_->SetName("VertexResource:Cylinder");

	vertexResource_->GetResource()->Unmap(0, nullptr);

	return vertexResource_->GetResource().Get();
}

ID3D12Resource* CylinderMesh::CreateIndexResource_(ID3D12Device* device) {
	uint32_t IndexNum = kCylinderDivide_ * 6;
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
	for (int Index = 0; Index < kCylinderDivide_; Index++) {
		indexData[0 + Index * 6] = Index * 4 + 0;
		indexData[1 + Index * 6] = Index * 4 + 1;
		indexData[2 + Index * 6] = Index * 4 + 2;
		indexData[3 + Index * 6] = Index * 4 + 2;
		indexData[4 + Index * 6] = Index * 4 + 1;
		indexData[5 + Index * 6] = Index * 4 + 3;
	}

	indexResource_->GetResource()->Unmap(0, nullptr);

	MeshBuffer::indexNum_ = IndexNum;

	indexResource_->SetName("IndexResource:Cylinder");

	return indexResource_->GetResource().Get();
}