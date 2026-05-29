#include "PSOFactory.h"
#include "Vector4.h"
#include "StringManage/ConvertString.h"

void PSOFactory::Initialize(DirectXCore* directXDriver){

	/// DirectXCoreのポインタを保存
	directXDriver_ = directXDriver;

	/// 各Factoryのインスタンスを作成
	inputLayoutFactory_ = std::make_unique<InputLayoutFactory>();
	rootSignatureFactory_ = std::make_unique<RootSignatureFactory>();
	shaderFactory_ = std::make_unique<ShaderFactory>();
	blendStateFactory_ = std::make_unique<BlendStateFactory>();
	rasterizerModeFactory_ = std::make_unique<RasterizerModeFactory>();
	depthStencilFactory_ = std::make_unique<DepthStencilFactory>();

}

Microsoft::WRL::ComPtr <ID3D12PipelineState> PSOFactory::createPSO(PSOKey& key) {
	/// graphicsPipelineStateに設定する情報をまとめる
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};

	/// InputLayout
	graphicsPipelineStateDesc.InputLayout = inputLayoutFactory_->Make(key);

	/// RootSignature
	graphicsPipelineStateDesc.pRootSignature = rootSignatureFactory_->Make(key, directXDriver_).Get();

	/// Shader
	ShaderPair shaderPair = shaderFactory_->MakeShaderBlob(key);
	graphicsPipelineStateDesc.VS = { shaderPair.vs->GetBufferPointer(), shaderPair.vs->GetBufferSize() };
	graphicsPipelineStateDesc.PS = { shaderPair.ps->GetBufferPointer(), shaderPair.ps->GetBufferSize() };

	/// BlendState
	graphicsPipelineStateDesc.BlendState = blendStateFactory_->Make(key);

	/// RasterizerState
	graphicsPipelineStateDesc.RasterizerState = rasterizerModeFactory_->Make(key);

	/// 書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = GetDXGIFormat(key.renderTargetFormatType);

	/// どのように画面に色を打ち込むかの設定（気にしなくて良い）
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	/// DepthStencilの設定
	graphicsPipelineStateDesc.DepthStencilState = depthStencilFactory_->Make(key);
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	/// PrimitiveType
	switch (key.primitiveType) {
	case PrimitiveType::POINT:

		graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		break;

	case PrimitiveType::LINE:

		graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		break;

	case PrimitiveType::TRIANGLE:

		graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;

	//case PrimitiveType::PATCH:
	//
	//	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	//	break;
	//
	}

	// 実際に生成
	Microsoft::WRL::ComPtr <ID3D12PipelineState> graphicsPipelineState_;
	HRESULT hr = directXDriver_->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(graphicsPipelineState_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	return graphicsPipelineState_;
}
