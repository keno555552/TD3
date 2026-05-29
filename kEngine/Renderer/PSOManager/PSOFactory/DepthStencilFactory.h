#pragma once
#include <memory>
#include <unordered_map>
#include <functional>

///DirectX12
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#include <dxgidebug.h>
#pragma comment(lib,"dxguid.lib")
#include <wrl/client.h>
#include <dxcapi.h>

#pragma comment(lib,"dxcompiler.lib")

#include "PSOManager/PSOKeys.h"

class DepthStencilFactory {
public:

	DepthStencilFactory();

	D3D12_DEPTH_STENCIL_DESC Make(PSOKey& key);

private:

	/// シェーダーコンパイルクラス
	std::unordered_map<DepthStencilType, std::function<D3D12_DEPTH_STENCIL_DESC(PSOKey&)>> depthStenctilRegistry;

private:

	D3D12_DEPTH_STENCIL_DESC MakeDepthStenctilStateNormal();
	D3D12_DEPTH_STENCIL_DESC MakeDepthStenctilStateReadOnly();
	D3D12_DEPTH_STENCIL_DESC MakeDepthStenctilStateDisable();
	D3D12_DEPTH_STENCIL_DESC MakeDepthStenctilStateGreater();

};