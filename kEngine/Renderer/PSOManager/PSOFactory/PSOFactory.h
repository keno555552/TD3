#pragma once

///DirectX12
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#include <dxgidebug.h>
#pragma comment(lib,"dxguid.lib")

#include <dxcapi.h>
#pragma comment(lib,"dxcompiler.lib")

/// Vector
#include <vector>

/// Config
#include "config.h"

/// Core
#include "DirectXCore.h"

/// ConvertString
#include "StringManage/ConvertString.h"

/// ShaderCompile
#include "ShaderManager/shader_compile.h"

/// PSOType関連
#include "Data/Render/Types/PSOType/PSOType.h"
#include "Renderer/PSOManager/PSOKeys.h"

/// PSOFactory関連
#include "Renderer/PSOManager/PSOFactory/InputLayoutFactory.h"
#include "Renderer/PSOManager/PSOFactory/RootSignatureFactory.h"
#include "Renderer/PSOManager/PSOFactory/ShaderFactory.h"
#include "Renderer/PSOManager/PSOFactory/BlendStateFactory.h"
#include "Renderer/PSOManager/PSOFactory/RasterizerModeFactory.h"
#include "Renderer/PSOManager/PSOFactory/DepthStencilFactory.h"


class PSOFactory {
public:

	/// 初期化
	void Initialize(DirectXCore* directXDriver);

	/// PSOを作成する関数
	Microsoft::WRL::ComPtr <ID3D12PipelineState> createPSO(PSOKey& key);
	RootSignatureFactory* getRootSignatureFactory() { return rootSignatureFactory_.get(); }

private:

	/// Coreを保存
	DirectXCore* directXDriver_{};

	/// 各Factoryのインスタンス
	std::unique_ptr<InputLayoutFactory> inputLayoutFactory_{};
	std::unique_ptr<RootSignatureFactory> rootSignatureFactory_{};
	std::unique_ptr<ShaderFactory> shaderFactory_{};
	std::unique_ptr<BlendStateFactory> blendStateFactory_{};
	std::unique_ptr<RasterizerModeFactory> rasterizerModeFactory_{};
	std::unique_ptr<DepthStencilFactory> depthStencilFactory_{};
};