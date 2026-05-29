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

#include "DirectXCore.h"

#include "PSOManager/PSOKeys.h"

class RootSignatureFactory {
public:

	RootSignatureFactory();

	Microsoft::WRL::ComPtr <ID3D12RootSignature> Make(PSOKey& key, DirectXCore* directXDriver_);

private:

	/// シェーダーコンパイルクラス
	std::unordered_map<RenderModelType, std::function<Microsoft::WRL::ComPtr <ID3D12RootSignature>(DirectXCore*,PSOKey&)>> rootSignatureRegistry;

private:

	std::unordered_map<RenderModelType, Microsoft::WRL::ComPtr <ID3D12RootSignature>> rootSignatureMap_;

	Microsoft::WRL::ComPtr <ID3D12RootSignature> MakeStatic(DirectXCore* directXDriver_, PSOKey& key);

	Microsoft::WRL::ComPtr <ID3D12RootSignature> MakeStaticSkinning(DirectXCore* directXDriver_, PSOKey& key);

	Microsoft::WRL::ComPtr <ID3D12RootSignature> MakeStaticFullscreenQuad(DirectXCore* directXDriver_, PSOKey& key);

};

/// RootSignatureは特別で
/// RootSignatureの管理も兼ねている