#include "Sprite2DMesh.h"

void Sprite2DMesh::SetSize(Vector2 LTpos, Vector2 LBpos,Vector2 RTpos, Vector2 RBpos, float TsizeX, float TsizeY, Vector2 TCLTPos, Vector2 TCRBPos) {
	coner[	  TOP_LEFT] = LTpos;
	coner[ BOTTOM_LEFT] = LBpos;
	coner[	 TOP_RIGHT] = RTpos;
	coner[BOTTOM_RIGHT] = RBpos;

	center = { ((LTpos.x + LBpos.x + RTpos.x + RBpos.x) / 4.0f),
			   ((LTpos.y + LBpos.y + RTpos.y + RBpos.y) / 4.0f) };

	TsizeX_ = TsizeX;
	TsizeY_ = TsizeY;

	TexcoordLT_.x = TCLTPos.x / TsizeX;
	TexcoordLT_.y = TCLTPos.y / TsizeY;
	TexcoordRB_.x = TCRBPos.x / TsizeX;
	TexcoordRB_.y = TCRBPos.y / TsizeY;
}

ID3D12Resource* Sprite2DMesh::CreateVertexResource_(ID3D12Device* device) {

	vertexResource_->CreateResourceClass_(device, sizeof(VertexData) * 5);
	vertexNum_ = 5;
	//CreateVertexBufferView_(6);

	VertexData* vertexDataSprite = nullptr;
	vertexResource_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
	// 四点定義
	vertexDataSprite[TOP_LEFT].position = { coner[BOTTOM_LEFT].x, coner[BOTTOM_LEFT].y, 0.0f, 1.0f };
	vertexDataSprite[TOP_LEFT].texcoord = { TexcoordLT_.x, TexcoordRB_.y };
	vertexDataSprite[TOP_LEFT].normal = { 0.0f, 0.0f, -1.0f };
	vertexDataSprite[BOTTOM_LEFT].position = { coner[TOP_LEFT].x, coner[TOP_LEFT].y, 0.0f, 1.0f };
	vertexDataSprite[BOTTOM_LEFT].texcoord = { TexcoordLT_.x, TexcoordLT_.y };
	vertexDataSprite[BOTTOM_LEFT].normal = { 0.0f, 0.0f, -1.0f };
	vertexDataSprite[BOTTOM_RIGHT].position = { coner[TOP_RIGHT].x, coner[TOP_RIGHT].y, 0.0f, 1.0f };
	vertexDataSprite[BOTTOM_RIGHT].texcoord = { TexcoordRB_.x, TexcoordLT_.y };
	vertexDataSprite[BOTTOM_RIGHT].normal = { 0.0f, 0.0f, -1.0f };
	vertexDataSprite[TOP_RIGHT].position = { coner[BOTTOM_RIGHT].x, coner[BOTTOM_RIGHT].y, 0.0f, 1.0f };
	vertexDataSprite[TOP_RIGHT].texcoord = { TexcoordRB_.x, TexcoordRB_.y };
	vertexDataSprite[TOP_RIGHT].normal = { 0.0f, 0.0f, -1.0f };
	vertexDataSprite[CENTER].position = { center.x, center.y, 0.0f, 1.0f };
	vertexDataSprite[CENTER].texcoord = { (TexcoordLT_.x + TexcoordRB_.x) / 2.0f,  
										  (TexcoordLT_.y + TexcoordRB_.y) / 2.0f };
	vertexDataSprite[CENTER].normal = { 0.0f, 0.0f, -1.0f };
	vertexResource_->GetResource()->Unmap(0, nullptr);

	return vertexResource_->GetResource().Get();
}

ID3D12Resource* Sprite2DMesh::CreateIndexResource_(ID3D12Device* device) {
	indexResource_->CreateResourceClass_(device, sizeof(uint32_t) * 3 * 4);
	indexNum_ = 3 * 4;
	indexBufferView = {};
	/// リソースの先頭のアドレスから使う
	indexBufferView.BufferLocation = indexResource_->GetResource()->GetGPUVirtualAddress();
	/// 使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferView.SizeInBytes = sizeof(uint32_t) * 3 * 4;
	///　インデックスはuint32_tとする
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	uint32_t* indexDataSprite = nullptr;
	indexResource_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	// 順番を指定
	indexDataSprite[ 0] = TOP_LEFT;
	indexDataSprite[ 1] = BOTTOM_LEFT;
	indexDataSprite[ 2] = CENTER;
	indexDataSprite[ 3] = BOTTOM_LEFT;
	indexDataSprite[ 4] = BOTTOM_RIGHT;
	indexDataSprite[ 5] = CENTER;
	indexDataSprite[ 6] = BOTTOM_RIGHT;
	indexDataSprite[ 7] = TOP_RIGHT;
	indexDataSprite[ 8] = CENTER;
	indexDataSprite[ 9] = TOP_RIGHT;
	indexDataSprite[10] = TOP_LEFT;
	indexDataSprite[11] = CENTER;
	indexResource_->GetResource()->Unmap(0, nullptr);
	return indexResource_->GetResource().Get();
}

bool Sprite2DMesh::CheckSize(Vector2 LTpos, Vector2 LBpos, Vector2 RTpos, Vector2 RBpos, float TsizeX, float TsizeY, Vector2 TCLTPos, Vector2 TCRBPos) {
	if(coner[	 TOP_LEFT].x == LTpos.x)return false;
	if(coner[	 TOP_LEFT].y == LTpos.y)return false;
	if(coner[ BOTTOM_LEFT].x == LBpos.x)return false;
	if(coner[ BOTTOM_LEFT].y == LBpos.y)return false;
	if(coner[	TOP_RIGHT].x == RTpos.x)return false;
	if(coner[	TOP_RIGHT].y == RTpos.y)return false;
	if(coner[BOTTOM_RIGHT].x == RBpos.x)return false;
	if(coner[BOTTOM_RIGHT].y == RBpos.y)return false;
	if(TsizeX_ == TsizeX)return false;
	if(TsizeY_ == TsizeY)return false;
	if(TexcoordLT_.x == TCLTPos.x / TsizeX)return false;
	if(TexcoordLT_.y == TCLTPos.y / TsizeY)return false;
	if(TexcoordRB_.x == TCRBPos.x / TsizeX)return false;
	if(TexcoordRB_.y == TCRBPos.y / TsizeY)return false;
	return true;
}


