#include "SkyCube.h"

ID3D12Resource* SkyCube::CreateVertexResource_(ID3D12Device* device) {
	vertexResource_->CreateResourceClass_(device, sizeof(VertexData) * 24);
	CreateVertexBufferView_(24);

	VertexData* v = nullptr;
	vertexResource_->GetResource()->Map(0, nullptr, (void**)&v);

	Vector4 p[8]{
		{ -1,  1,  1, 1 }, // 0
		{  1,  1,  1, 1 }, // 1
		{  1,  1, -1, 1 }, // 2
		{ -1,  1, -1, 1 }, // 3
		{ -1, -1,  1, 1 }, // 4
		{  1, -1,  1, 1 }, // 5
		{  1, -1, -1, 1 }, // 6
		{ -1, -1, -1, 1 }  // 7
	};


	//
	//// 每個面 4 個頂點
	//int face[6][4] = {
	//	{ 0, 1, 5, 4 }, // +Z (front)
	//	{ 1, 2, 6, 5 }, // +X (right)
	//	{ 2, 3, 7, 6 }, // -Z (back)
	//	{ 3, 0, 4, 7 }, // -X (left)
	//	{ 3, 2, 1, 0 }, // +Y (top)
	//	{ 4, 5, 6, 7 }  // -Y (bottom)
	//};
	//
	//for (int i = 0; i < 6; i++) {
	//	v[i * 4 + 0].position = p[face[i][0]];
	//	v[i * 4 + 1].position = p[face[i][1]];
	//	v[i * 4 + 2].position = p[face[i][2]];
	//	v[i * 4 + 3].position = p[face[i][3]];
	//}


	for (int i = 0; i < 6; i++) {
		if (i < 4) {
			// 左下
			v[0 + i * 4].position = p[(FRONT_BOTTOM_LEFT + i) % 4 + 4];
			v[0 + i * 4].texcoord = { 1.0f, 1.0f };
			// 左上			  
			v[1 + i * 4].position = p[(FRONT_BOTTOM_RIGHT + i) % 4 + 4];
			v[1 + i * 4].texcoord = { 0.0f, 1.0f };
			// 右下			  
			v[2 + i * 4].position = p[(FRONT_TOP_LEFT + i) % 4];
			v[2 + i * 4].texcoord = { 1.0f, 0.0f };
			// 右上			  
			v[3 + i * 4].position = p[(FRONT_TOP_RIGHT + i) % 4];
			v[3 + i * 4].texcoord = { 0.0f, 0.0f };
		} else if (i == 4) {
			// 左下
			v[0 + i * 4].position = p[FRONT_TOP_LEFT];
			v[0 + i * 4].texcoord = { 0.0f, 0.0f };
			// 左上			  
			v[1 + i * 4].position = p[FRONT_TOP_RIGHT];
			v[1 + i * 4].texcoord = { 1.0f, 0.0f };
			// 右下			  
			v[2 + i * 4].position = p[BACK_TOP_LEFT];
			v[2 + i * 4].texcoord = { 0.0f, 1.0f };
			// 右上			  
			v[3 + i * 4].position = p[BACK_TOP_RIGHT];
			v[3 + i * 4].texcoord = { 1.0f, 1.0f };
		} else if (i == 5) {
			// 左下			  
			v[0 + i * 4].position = p[FRONT_BOTTOM_RIGHT];
			v[0 + i * 4].texcoord = { 0.0f, 0.0f };
			// 左上			  
			v[1 + i * 4].position = p[FRONT_BOTTOM_LEFT];
			v[1 + i * 4].texcoord = { 1.0f, 0.0f };
			// 右下			  
			v[2 + i * 4].position = p[BACK_BOTTOM_RIGHT];
			v[2 + i * 4].texcoord = { 0.0f, 1.0f };
			// 右下			  
			v[3 + i * 4].position = p[BACK_BOTTOM_LEFT];
			v[3 + i * 4].texcoord = { 1.0f, 1.0f };
		}

		// 法線計算
		Vector3 edge1 = { v[1 + i * 4].position.x - v[0 + i * 4].position.x,
						  v[1 + i * 4].position.y - v[0 + i * 4].position.y,
						  v[1 + i * 4].position.z - v[0 + i * 4].position.z };

		Vector3 edge2 = { v[1 + i * 4].position.x - v[2 + i * 4].position.x,
						  v[1 + i * 4].position.y - v[2 + i * 4].position.y,
						  v[1 + i * 4].position.z - v[2 + i * 4].position.z };

		Vector3 normal = { Cross(edge1, edge2) * -1 };

		v[0 + i * 4].normal = normal;
		v[1 + i * 4].normal = normal;
		v[2 + i * 4].normal = normal;
		v[3 + i * 4].normal = normal;
	}

	vertexResource_->GetResource()->Unmap(0, nullptr);

	vertexResource_->SetName("SkyCubeVertexResource");

	return vertexResource_->GetResource().Get();
}

ID3D12Resource* SkyCube::CreateIndexResource_(ID3D12Device* device) {
	indexResource_->CreateResourceClass_(device, sizeof(uint32_t) * 36);
	CreateIndexBufferView_(36);

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
		int base = i * 4;
		int out = i * 6;

		// 反轉順序（面朝內）
		indexDataSprite[out + 0] = base + 0;
		indexDataSprite[out + 1] = base + 2;
		indexDataSprite[out + 2] = base + 1;

		indexDataSprite[out + 3] = base + 2;
		indexDataSprite[out + 4] = base + 3;
		indexDataSprite[out + 5] = base + 1;
	}
	indexResource_->GetResource()->Unmap(0, nullptr);

	MeshBuffer::vertexNum_ = 24;
	MeshBuffer::indexNum_ = 36;

	indexResource_->SetName("SkyCubeIndexResource");

	return indexResource_->GetResource().Get();
}

