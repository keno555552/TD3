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

#include "PSOManager/PSOKeys.h"

class InputLayoutFactory {
public:

	InputLayoutFactory();

	D3D12_INPUT_LAYOUT_DESC Make(PSOKey& key);

private:

	/// シェーダーコンパイルクラス
	std::unordered_map<RenderModelType, std::function<D3D12_INPUT_LAYOUT_DESC(PSOKey&)>> inputLayoutRegistry;

private:

	D3D12_INPUT_LAYOUT_DESC MakeInputLayoutStatic();
	D3D12_INPUT_LAYOUT_DESC MakeInputLayoutSkinning();
	D3D12_INPUT_LAYOUT_DESC MakeInputLayoutDebugLine();
	D3D12_INPUT_LAYOUT_DESC MakeInputLayoutFullscreenQuad();

};