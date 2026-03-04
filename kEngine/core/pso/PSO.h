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

#include "DirectXCore.h"
#include "config.h"
#include <vector>

#include "ConvertString.h"

#include "shader_compile.h"

class PSO{
public:
	~PSO();

	void Initialize(DirectXCore* directXDriver);

	ID3D12RootSignature* getRootSignature(int handle) { return rootSignatureList_[handle]; }

	ID3D12RootSignature* createRootSignature(); // <<<<<<

	ID3D12PipelineState* createPSO(LightModelType lightMadelType);

private:
	//D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	DirectXCore* directXDriver_;
	std::string LMName_;
	ID3DBlob* signatureBlob_ = nullptr;
	ID3DBlob* errorBlob_ = nullptr;
	std::vector<ID3D12RootSignature*> rootSignatureList_;
	ID3D12RootSignature* rootSignature_;					///Listからもセーブしたから解放しなくていい
	D3D12_INPUT_ELEMENT_DESC inputElementDESCs_[3] = {};
	D3D12_INPUT_LAYOUT_DESC inputLayoutDESC_ = {};
	D3D12_BLEND_DESC blendDesc_ = {};
	D3D12_RASTERIZER_DESC rasterizerDesc_ = {};
	IDxcBlob* vertexShaderBlob_ = nullptr;
	IDxcBlob* pixelShaderBlob_ = nullptr;
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	ID3D12PipelineState* graphicsPipelineState_ = nullptr; ///外に出して、そこで解放したからしなくていい
	Shader_compile* shader_compile_ = nullptr;

private:
	void createInputLayout();
	void SetBlendState();
	void SetRasterizerState();
	void ShaderCompile(LightModelType lightModelType);
	void SetDepthStencilState();
	void SetGraphicsPipelineState();
};