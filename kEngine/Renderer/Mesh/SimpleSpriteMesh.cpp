#include "SimpleSpriteMesh.h"

void SimpleSpriteMesh::SetSize(Vector2 RBpos) {

	conerData.coner[(int)CornerName::TOP_LEFT] = { 0,		  0 };
	conerData.coner[(int)CornerName::BOTTOM_LEFT] = { 0,	RBpos.y };
	conerData.coner[(int)CornerName::BOTTOM_RIGHT] = RBpos;
	conerData.coner[(int)CornerName::TOP_RIGHT] = { RBpos.x,  0 };

	Mapping();
}

void SimpleSpriteMesh::SetSize(CornerData corner) {

	conerData = corner;

	Mapping();
}

void SimpleSpriteMesh::SetAnchor(Vector2 RBpos, Vector2 anchorPoint) {

	//float left = 0.0f - anchorPoint.x;
	//float right = 1.0f - anchorPoint.x;
	//float top = 0.0f - anchorPoint.y;
	//float bottom = 1.0f - anchorPoint.y;

	float left = 0.0f - anchorPoint.x;
	float right = RBpos.x - anchorPoint.x;
	float top = 0.0f - anchorPoint.y;
	float bottom = RBpos.y - anchorPoint.y;

	conerData.coner[(int)CornerName::TOP_LEFT] = { left,		top };
	conerData.coner[(int)CornerName::BOTTOM_LEFT] = { left,		bottom };
	conerData.coner[(int)CornerName::BOTTOM_RIGHT] = { right,	bottom };
	conerData.coner[(int)CornerName::TOP_RIGHT] = { right,	top };

	Mapping();
}

void SimpleSpriteMesh::SetTexcoord(Vector2 textureSize, Vector2 cropLT, Vector2 cropSize) {
	float leftX = cropLT.x / textureSize.x;
	float rightX = (cropLT.x + cropSize.x) / textureSize.x;
	float topY = cropLT.y / textureSize.y;
	float bottomY = (cropLT.y + cropSize.y) / textureSize.y;

	Texcoord[(int)CornerName::TOP_LEFT] = { leftX,	topY };
	Texcoord[(int)CornerName::BOTTOM_LEFT] = { leftX,	bottomY };
	Texcoord[(int)CornerName::BOTTOM_RIGHT] = { rightX,	bottomY };
	Texcoord[(int)CornerName::TOP_RIGHT] = { rightX,	topY };

	Mapping();
}

void SimpleSpriteMesh::ResetTexcoord() {

	Texcoord[0] = { 0,0 };
	Texcoord[1] = { 0,1 };
	Texcoord[2] = { 1,1 };
	Texcoord[3] = { 1,0 };
	Mapping();

}


ID3D12Resource* SimpleSpriteMesh::CreateVertexResource_(ID3D12Device* device) {

	vertexResource_->CreateResourceClass_(device, sizeof(VertexData) * 4);
	vertexNum_ = 4;

	Mapping();

	return vertexResource_->GetResource().Get();
}

void SimpleSpriteMesh::Mapping() {
	VertexData* vertexDataSprite = nullptr;
	vertexResource_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
	// 四点定義
	vertexDataSprite[(int)CornerName::TOP_LEFT].position = { conerData.coner[(int)CornerName::BOTTOM_LEFT].x, conerData.coner[(int)CornerName::BOTTOM_LEFT].y, 0.0f, 1.0f };
	vertexDataSprite[(int)CornerName::TOP_LEFT].texcoord = { Texcoord[(int)CornerName::BOTTOM_LEFT].x, Texcoord[(int)CornerName::BOTTOM_LEFT].y };
	vertexDataSprite[(int)CornerName::TOP_LEFT].normal = { 0.0f, 0.0f, -1.0f };
	vertexDataSprite[(int)CornerName::BOTTOM_LEFT].position = { conerData.coner[(int)CornerName::TOP_LEFT].x, conerData.coner[(int)CornerName::TOP_LEFT].y, 0.0f, 1.0f };
	vertexDataSprite[(int)CornerName::BOTTOM_LEFT].texcoord = { Texcoord[(int)CornerName::TOP_LEFT].x, Texcoord[(int)CornerName::TOP_LEFT].y };
	vertexDataSprite[(int)CornerName::BOTTOM_LEFT].normal = { 0.0f, 0.0f, -1.0f };
	vertexDataSprite[(int)CornerName::BOTTOM_RIGHT].position = { conerData.coner[(int)CornerName::TOP_RIGHT].x, conerData.coner[(int)CornerName::TOP_RIGHT].y, 0.0f, 1.0f };
	vertexDataSprite[(int)CornerName::BOTTOM_RIGHT].texcoord = { Texcoord[(int)CornerName::TOP_RIGHT].x, Texcoord[(int)CornerName::TOP_RIGHT].y };
	vertexDataSprite[(int)CornerName::BOTTOM_RIGHT].normal = { 0.0f, 0.0f, -1.0f };
	vertexDataSprite[(int)CornerName::TOP_RIGHT].position = { conerData.coner[(int)CornerName::BOTTOM_RIGHT].x, conerData.coner[(int)CornerName::BOTTOM_RIGHT].y, 0.0f, 1.0f };
	vertexDataSprite[(int)CornerName::TOP_RIGHT].texcoord = { Texcoord[(int)CornerName::BOTTOM_RIGHT].x, Texcoord[(int)CornerName::BOTTOM_RIGHT].y };
	vertexDataSprite[(int)CornerName::TOP_RIGHT].normal = { 0.0f, 0.0f, -1.0f };
	vertexResource_->GetResource()->Unmap(0, nullptr);
}

ID3D12Resource* SimpleSpriteMesh::CreateIndexResource_(ID3D12Device* device) {
	indexResource_->CreateResourceClass_(device, sizeof(uint32_t) * 6);
	indexNum_ = 6;

	indexBufferView = {};
	/// リソースの先頭のアドレスから使う
	indexBufferView.BufferLocation = indexResource_->GetResource()->GetGPUVirtualAddress();
	/// 使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferView.SizeInBytes = sizeof(uint32_t) * 6;
	///　インデックスはuint32_tとする
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	uint32_t* indexDataSprite = nullptr;
	indexResource_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	// 順番を指定
	indexDataSprite[0] = (int)CornerName::TOP_LEFT;
	indexDataSprite[1] = (int)CornerName::BOTTOM_LEFT;
	indexDataSprite[2] = (int)CornerName::BOTTOM_RIGHT;
	indexDataSprite[3] = (int)CornerName::TOP_LEFT;
	indexDataSprite[4] = (int)CornerName::BOTTOM_RIGHT;
	indexDataSprite[5] = (int)CornerName::TOP_RIGHT;
	indexResource_->GetResource()->Unmap(0, nullptr);
	return indexResource_->GetResource().Get();
}

bool SimpleSpriteMesh::CheckSize(Vector2 RBpos) {
	if (conerData.coner[(int)CornerName::BOTTOM_RIGHT].x == RBpos.x)return false;
	if (conerData.coner[(int)CornerName::BOTTOM_RIGHT].y == RBpos.y)return false;
	return true;
}

bool SimpleSpriteMesh::CheckIsDefaultSize() {
	if (Texcoord[0] != Vector2{ 0,0 })return false;
	if (Texcoord[0] != Vector2{ 0,1 })return false;
	if (Texcoord[0] != Vector2{ 1,1 })return false;
	if (Texcoord[0] != Vector2{ 1,0 })return false;
	return true;
}

bool SimpleSpriteMesh::operator==(const SimpleSpriteMesh target) {
	int i = 0;
	for (auto& cornerPos : conerData.coner) { if (cornerPos != target.conerData.coner[i])return false; }
	for (auto& texcoordPos : Texcoord) { if (texcoordPos != target.Texcoord[i])return false; }
	return true;
}


