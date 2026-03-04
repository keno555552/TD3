#include "CubeMesh.h"
ID3D12Resource* CubeMesh::CreateVertexResource_(ID3D12Device* device) {
	vertexResource_->CreateResourceClass_(device, sizeof(VertexData) * 24);

	// 頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	// 書き込むためのアドレスを取得
	vertexResource_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	Vector4 point[8]{
		{ -0.5f, 0.5f, 0.5f, 1.0f },
		{ 0.5f, 0.5f, 0.5f , 1.0f },
		{ 0.5f, 0.5f, -0.5f , 1.0f },
		{ -0.5f, 0.5f, -0.5f , 1.0f },
		{ -0.5f, -0.5f, 0.5f, 1.0f },
		{ 0.5f, -0.5f, 0.5f , 1.0f } ,
		{ 0.5f, -0.5f, -0.5f , 1.0f },
		{ -0.5f, -0.5f, -0.5f , 1.0f }
	};

	for (int i = 0; i < 6; i++) {
		if (i < 4) {
			// 左下
			vertexData[0 + i * 4].position = point[(FRONT_BOTTOM_LEFT + i) % 4 + 4];
			vertexData[0 + i * 4].texcoord = { 1.0f, 1.0f };
			// 左上			  
			vertexData[1 + i * 4].position = point[(FRONT_BOTTOM_RIGHT + i) % 4 + 4];
			vertexData[1 + i * 4].texcoord = { 0.0f, 1.0f };
			// 右下			  
			vertexData[2 + i * 4].position = point[(FRONT_TOP_LEFT + i) % 4];
			vertexData[2 + i * 4].texcoord = { 1.0f, 0.0f };
			// 右上			  
			vertexData[3 + i * 4].position = point[(FRONT_TOP_RIGHT + i) % 4];
			vertexData[3 + i * 4].texcoord = { 0.0f, 0.0f };
		} else if (i == 4) {
			// 左下
			vertexData[0 + i * 4].position = point[FRONT_TOP_LEFT];
			vertexData[0 + i * 4].texcoord = { 0.0f, 0.0f };
			// 左上			  
			vertexData[1 + i * 4].position = point[FRONT_TOP_RIGHT];
			vertexData[1 + i * 4].texcoord = { 1.0f, 0.0f };
			// 右下			  
			vertexData[2 + i * 4].position = point[BACK_TOP_LEFT];
			vertexData[2 + i * 4].texcoord = { 0.0f, 1.0f };
			// 右上			  
			vertexData[3 + i * 4].position = point[BACK_TOP_RIGHT];
			vertexData[3 + i * 4].texcoord = { 1.0f, 1.0f };
		} else if (i == 5) {
			// 左下			  
			vertexData[0 + i * 4].position = point[FRONT_BOTTOM_RIGHT];
			vertexData[0 + i * 4].texcoord = { 0.0f, 0.0f };
			// 左上			  
			vertexData[1 + i * 4].position = point[FRONT_BOTTOM_LEFT];
			vertexData[1 + i * 4].texcoord = { 1.0f, 0.0f };
			// 右下			  
			vertexData[2 + i * 4].position = point[BACK_BOTTOM_RIGHT];
			vertexData[2 + i * 4].texcoord = { 0.0f, 1.0f };
			// 右下			  
			vertexData[3 + i * 4].position = point[BACK_BOTTOM_LEFT];
			vertexData[3 + i * 4].texcoord = { 1.0f, 1.0f };
		}

		// 法線計算
		Vector3 edge1 = { vertexData[1 + i * 4].position.x - vertexData[0 + i * 4].position.x,
						  vertexData[1 + i * 4].position.y - vertexData[0 + i * 4].position.y,
						  vertexData[1 + i * 4].position.z - vertexData[0 + i * 4].position.z };

		Vector3 edge2 = { vertexData[1 + i * 4].position.x - vertexData[2 + i * 4].position.x,
						  vertexData[1 + i * 4].position.y - vertexData[2 + i * 4].position.y,
						  vertexData[1 + i * 4].position.z - vertexData[2 + i * 4].position.z };

		Vector3 normal = { Cross(edge1, edge2) * -1 };

		vertexData[0 + i * 4].normal = normal;
		vertexData[1 + i * 4].normal = normal;
		vertexData[2 + i * 4].normal = normal;
		vertexData[3 + i * 4].normal = normal;
	}
	vertexResource_->GetResource()->Unmap(0, nullptr);


	return vertexResource_->GetResource().Get();
}

ID3D12Resource* CubeMesh::CreateIndexResource_(ID3D12Device* device) {
	indexResource_->CreateResourceClass_(device, sizeof(uint32_t) * 36);

	indexBufferView = {};
	/// リソースの先頭のアドレスから使う
	indexBufferView.BufferLocation = indexResource_->GetResource()->GetGPUVirtualAddress();
	/// 使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferView.SizeInBytes = sizeof(uint32_t) * 36;
	///　インデックスはuint32_tとする
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	uint32_t* indexDataSprite = nullptr;
	indexResource_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	// 順番を指定
	for (int i = 0; i < 6; i++) {
		if (i < 5) {
			indexDataSprite[0 + i * 6] = 0 + i * 4;
			indexDataSprite[1 + i * 6] = 1 + i * 4;
			indexDataSprite[2 + i * 6] = 2 + i * 4;
			indexDataSprite[3 + i * 6] = 2 + i * 4;
			indexDataSprite[4 + i * 6] = 1 + i * 4;
			indexDataSprite[5 + i * 6] = 3 + i * 4;
		} else if (i == 5) {
			indexDataSprite[0 + i * 6] = 0 + i * 4;
			indexDataSprite[1 + i * 6] = 1 + i * 4;
			indexDataSprite[2 + i * 6] = 2 + i * 4;
			indexDataSprite[3 + i * 6] = 3 + i * 4;
			indexDataSprite[4 + i * 6] = 2 + i * 4;
			indexDataSprite[5 + i * 6] = 1 + i * 4;
		}
	}
	indexResource_->GetResource()->Unmap(0, nullptr);

	SetVertexNum(36);

	return indexResource_->GetResource().Get();
}

