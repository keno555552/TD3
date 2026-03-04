#include "SphereMesh.h"
#define M_PI 3.1415926f

ID3D12Resource* SphereMesh::CreateVertexResource_(ID3D12Device* device) {
	kSudivision_ = kMaxSudivision_;
	uint32_t VertexNum = kMaxSudivision_ * kMaxSudivision_ * 4;
	vertexResource_->CreateResourceClass_(device, sizeof(VertexData) * VertexNum);
	CreateVertexBufferView_(VertexNum);

	VertexData* vertexData = nullptr;// 頂点リソースにデータを書き込む
	vertexResource_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	const float kLonEvery = M_PI * 2.0f / (kMaxSudivision_); // 経度分割1つ分の角度
	const float kLatEvery = M_PI / (kMaxSudivision_); // 緯度分割1つ分の角度
	// 緯度方向に分割 -π/2 ～ π/2
	for (int latIndex = 0; latIndex < kMaxSudivision_; latIndex++) {
		float lat = -M_PI / 2.0f + kLatEvery * latIndex; // 現在の緯度
		// 経度方向に分割 0 ～ 2π
		for (int lonIndex = 0; lonIndex < kMaxSudivision_; lonIndex++) {
			uint32_t start = (latIndex * kMaxSudivision_ + lonIndex) * 4;
			float lon = lonIndex * kLonEvery; // 現在の経度
			// 書き込むためのアドレスを取得
			//a
			vertexData[start].position.x = cosf(lat) * cosf(lon);
			vertexData[start].position.y = sinf(lat);
			vertexData[start].position.z = cosf(lat) * sinf(lon);
			vertexData[start].position.w = 1.0f;
			vertexData[start].texcoord.x = float(lonIndex) / float(kMaxSudivision_);
			vertexData[start].texcoord.y = 1.0f - float(latIndex) / float(kMaxSudivision_);
			vertexData[start].normal.x = vertexData[start].position.x;
			vertexData[start].normal.y = vertexData[start].position.y;
			vertexData[start].normal.z = vertexData[start].position.z;
			//b
			vertexData[start + 1].position.x = cosf(lat + kLatEvery) * cosf(lon);
			vertexData[start + 1].position.y = sinf(lat + kLatEvery);
			vertexData[start + 1].position.z = cosf(lat + kLatEvery) * sinf(lon);
			vertexData[start + 1].position.w = 1.0f;
			vertexData[start + 1].texcoord.x = float(lonIndex) / float(kMaxSudivision_);
			vertexData[start + 1].texcoord.y = 1.0f - float(latIndex + 1) / float(kMaxSudivision_);
			vertexData[start + 1].normal.x = vertexData[start + 1].position.x;
			vertexData[start + 1].normal.y = vertexData[start + 1].position.y;
			vertexData[start + 1].normal.z = vertexData[start + 1].position.z;
			//c
			vertexData[start + 2].position.x = cosf(lat) * cosf(lon + kLonEvery);
			vertexData[start + 2].position.y = sinf(lat);
			vertexData[start + 2].position.z = cosf(lat) * sinf(lon + kLonEvery);
			vertexData[start + 2].position.w = 1.0f;
			vertexData[start + 2].texcoord.x = float(lonIndex + 1) / float(kMaxSudivision_);
			vertexData[start + 2].texcoord.y = 1.0f - float(latIndex) / float(kMaxSudivision_);
			vertexData[start + 2].normal.x = vertexData[start + 2].position.x;
			vertexData[start + 2].normal.y = vertexData[start + 2].position.y;
			vertexData[start + 2].normal.z = vertexData[start + 2].position.z;
			//d
			vertexData[start + 3].position.x = cosf(lat + kLatEvery) * cosf(lon + kLonEvery);
			vertexData[start + 3].position.y = sinf(lat + kLatEvery);
			vertexData[start + 3].position.z = cosf(lat + kLatEvery) * sinf(lon + kLonEvery);
			vertexData[start + 3].position.w = 1.0f;
			vertexData[start + 3].texcoord.x = float(lonIndex + 1) / float(kMaxSudivision_);
			vertexData[start + 3].texcoord.y = 1.0f - float(latIndex + 1) / float(kMaxSudivision_);
			vertexData[start + 3].normal.x = vertexData[start + 3].position.x;
			vertexData[start + 3].normal.y = vertexData[start + 3].position.y;
			vertexData[start + 3].normal.z = vertexData[start + 3].position.z;

		};
	}
	vertexResource_->GetResource()->Unmap(0, nullptr);
	return vertexResource_->GetResource().Get();
}

ID3D12Resource* SphereMesh::CreateIndexResource_(ID3D12Device* device) {
	uint32_t IndexNum = kMaxSudivision_ * kMaxSudivision_ * 6;
	indexResource_->CreateResourceClass_(device, sizeof(uint32_t) * IndexNum);
	CreateIndexBufferView_(IndexNum);
	VertexNum_ = IndexNum;

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
	for (int latIndex = 0; latIndex < kMaxSudivision_; latIndex++) {
		for (int lonIndex = 0; lonIndex < kMaxSudivision_; lonIndex++) {
			uint32_t start = (latIndex * kMaxSudivision_ + lonIndex);
			indexData[0 + start * 6] = start * 4 + 0; // a
			indexData[1 + start * 6] = start * 4 + 1; // b
			indexData[2 + start * 6] = start * 4 + 2; // c
			indexData[3 + start * 6] = start * 4 + 3; // c
			indexData[4 + start * 6] = start * 4 + 2; // b
			indexData[5 + start * 6] = start * 4 + 1; // d
		}
	}
	indexResource_->GetResource()->Unmap(0, nullptr);
	return indexResource_->GetResource().Get();
}