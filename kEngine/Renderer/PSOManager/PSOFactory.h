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

#include "StringManage/ConvertString.h"

#include "ShaderManager/shader_compile.h"

class PSOFactory {
public:

	void Initialize(DirectXCore* directXDriver);
	void Finalize();

	ID3D12RootSignature* getRootSignature(int handle) { return rootSignatureList_[handle].Get(); }

	ID3D12RootSignature* createRootSignature(); // <<<<<<

	Microsoft::WRL::ComPtr <ID3D12PipelineState> createPSO(LightModelType lightMadelType);

private:
	//D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	DirectXCore* directXDriver_;													//借り
	std::string LMName_;
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob_{};
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob_{};
	std::vector<Microsoft::WRL::ComPtr <ID3D12RootSignature>> rootSignatureList_;
	Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature_{};					///Listからもセーブしたから解放しなくていい
	D3D12_INPUT_ELEMENT_DESC inputElementDESCs_[3] = {};
	D3D12_INPUT_LAYOUT_DESC inputLayoutDESC_ = {};
	D3D12_BLEND_DESC blendDesc_ = {};
	D3D12_RASTERIZER_DESC rasterizerDesc_ = {};
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob_{};
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob_{};
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	Microsoft::WRL::ComPtr <ID3D12PipelineState> graphicsPipelineState_{};			///外に出して、そこで解放したからしなくていい
	std::unique_ptr<Shader_compile> shader_compile_{};

private:
	void createInputLayout();
	void SetBlendState();
	void SetRasterizerState();
	void ShaderCompile(LightModelType lightModelType);
	void SetDepthStencilState();
	void SetGraphicsPipelineState();
};