#pragma once
#include "DirectXCore.h"
#include <vector>
#include <wrl.h>

ID3D12Resource* CreateResource(ID3D12Device* device, size_t sizeInBytes);

class BasicResource
{
public:
	~BasicResource();
	
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateResourceClass_(ID3D12Device* device, size_t sizeInBytes);
	Microsoft::WRL::ComPtr<ID3D12Resource> SaveResource_(Microsoft::WRL::ComPtr<ID3D12Resource> resource);
	Microsoft::WRL::ComPtr<ID3D12Resource> GetResource(int Index = -1);
	int GetResourceCounter() { return resourceCounter; }
	void ClearResource();

private:
	std::vector <Microsoft::WRL::ComPtr<ID3D12Resource>> resource_;
	int resourceCounter = 0;

	/// Debug
	char buffer[128]{};
};

/// Removed legacy WVPResource — replaced 
/// by BasicResource + ResourceManager architecture.