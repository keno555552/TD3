#pragma once
#include "BasicResource.h"
#include "TransformationMatrix.h"

class WVPResource
{
public:
	WVPResource(ID3D12Device* device);
	~WVPResource();

	ID3D12Resource* CreateWVPResource_(ID3D12Device* device, TransformationMatrix* wvpData);
	void ClearWVPResource();


private:
	BasicResource* wvpResource_ = new BasicResource;

};

