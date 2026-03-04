#include "TriangleMesh.h"
#include <d3d12.h>

ID3D12Resource* TriangleMesh::CreateVertexResource_(ID3D12Device* device) {
	vertexResource_->CreateResourceClass_(device, sizeof(VertexData) * 6);
	//CreateVertexBufferView_(6);

	// 頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	// 書き込むためのアドレスを取得
	vertexResource_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	// 左下
	vertexData[0].position = { -0.5f, -0.5f, 0.0f, 1.0f };
	vertexData[0].texcoord = { 0.0f, 1.0f };
	// 上
	vertexData[1].position = { 0.0f, 0.5f, 0.0f, 1.0f };
	vertexData[1].texcoord = { 0.5f, 0.0f };
	// 右下
	vertexData[2].position = { 0.5f, -0.5f, 0.0f, 1.0f };
	vertexData[2].texcoord = { 1.0f, 1.0f };
	// 左下
	vertexData[5].position = { -0.5f, -0.5f, 0.0f, 1.0f };
	vertexData[5].texcoord = { 0.0f, 1.0f };
	// 上
	vertexData[4].position = { 0.0f, 0.5f, 0.0f, 1.0f };
	vertexData[4].texcoord = { 0.5f, 0.0f };
	// 右下
	vertexData[3].position = { 0.5f, -0.5f, 0.0f, 1.0f };
	vertexData[3].texcoord = { 1.0f, 1.0f };

	// 法線計算
	Vector3 edge1 = { vertexData[1].position.x - vertexData[0].position.x,
					  vertexData[1].position.y - vertexData[0].position.y,
					  vertexData[1].position.z - vertexData[0].position.z };

	Vector3 edge2 = { vertexData[1].position.x - vertexData[2].position.x,
					  vertexData[1].position.y - vertexData[2].position.y,
					  vertexData[1].position.z - vertexData[2].position.z };

	Vector3 normal = { Cross(edge1, edge2) };

	vertexData[3].normal = normal;
	vertexData[4].normal = normal;
	vertexData[5].normal = normal;

	normal = { -normal.x,-normal.y,-normal.z, };

	vertexData[0].normal = normal;
	vertexData[1].normal = normal;
	vertexData[2].normal = normal;

	vertexResource_->GetResource()->Unmap(0, nullptr);

	SetVertexNum(6);

	return vertexResource_->GetResource().Get();
}