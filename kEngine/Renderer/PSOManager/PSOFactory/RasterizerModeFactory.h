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

#include "ShaderManager/shader_compile.h"
#include "PSOManager/PSOKeys.h"

class RasterizerModeFactory {
public:

	RasterizerModeFactory();

	D3D12_RASTERIZER_DESC Make(PSOKey& key);

private:

	/// シェーダーコンパイルクラス
	std::unordered_map<RasterizerMode, std::function<D3D12_RASTERIZER_DESC(PSOKey&)>> rasterizerModeRegistry;

private:

	D3D12_RASTERIZER_DESC MakeRasterizerModeNormal();
	D3D12_RASTERIZER_DESC MakeRasterizerModeCullFront();
	D3D12_RASTERIZER_DESC MakeRasterizerModeCullNone();
	D3D12_RASTERIZER_DESC MakeRasterizerModeWireframe();

};


