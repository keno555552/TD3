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

class BlendStateFactory {
public:

	BlendStateFactory();

	D3D12_BLEND_DESC Make(PSOKey& key);

private:

	/// シェーダーコンパイルクラス
	std::unordered_map<BlendModeType, std::function<D3D12_BLEND_DESC(PSOKey&)>> blendStateRegistry;

private:

	D3D12_BLEND_DESC MakeBlendOpaque();
	D3D12_BLEND_DESC MakeBlendNormal();
	D3D12_BLEND_DESC MakeBlendAddAlpha();
	D3D12_BLEND_DESC MakeBlendSubAlpha();
	D3D12_BLEND_DESC MakeBlendMultiply();
	D3D12_BLEND_DESC MakeBlendScreen();

};