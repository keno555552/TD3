#include "WVPResource.h"

WVPResource::WVPResource(ID3D12Device* device) {
}

WVPResource::~WVPResource() {
	ClearWVPResource();
	delete wvpResource_;
}

ID3D12Resource* WVPResource::CreateWVPResource_(ID3D12Device* device, TransformationMatrix* wvpData) {
	wvpResource_->CreateResourceClass_(device, sizeof(TransformationMatrix));

	TransformationMatrix* newWvpData_ = nullptr;
	/// 書き込むためのアドレスを取得
	wvpResource_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&newWvpData_));
	/// 単位行列を書きこんでおく
	*newWvpData_ = *wvpData;

	wvpResource_->GetResource()->Unmap(0, nullptr);

	return wvpResource_->GetResource().Get();
}

void WVPResource::ClearWVPResource() {
	if (wvpResource_)wvpResource_->ClearResource();
}