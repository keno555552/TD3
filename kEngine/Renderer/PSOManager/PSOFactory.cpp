#include "PSOFactory.h"
#include "Vector4.h"
#include "StringManage/ConvertString.h"

void PSOFactory::Initialize(DirectXCore* directXDriver) {
	directXDriver_ = directXDriver;
	shader_compile_ = std::make_unique<Shader_compile>();
	shader_compile_->Initialize();
}

void PSOFactory::Finalize() {
	rootSignatureList_.clear();
	rootSignature_.Reset();
	vertexShaderBlob_.Reset();
	pixelShaderBlob_.Reset();
	graphicsPipelineState_.Reset();
	shader_compile_->Finalize();
	shader_compile_.reset();
}

Microsoft::WRL::ComPtr <ID3D12PipelineState> PSOFactory::createPSO(LightModelType lightModelType) {
	createRootSignature();
	createInputLayout();
	SetBlendState();
	SetRasterizerState();
	ShaderCompile(lightModelType);
	SetDepthStencilState();

	SetGraphicsPipelineState();
	return graphicsPipelineState_;
}

ID3D12RootSignature* PSOFactory::createRootSignature() {
	///RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	/// DescriptorRange作成。SRVを使う
	D3D12_DESCRIPTOR_RANGE descriptorRange[1]{};
	descriptorRange[0].BaseShaderRegister = 0; // 0から始まる
	descriptorRange[0].NumDescriptors = 1; // 数は1つ
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // 連続している

	D3D12_STATIC_SAMPLER_DESC staticSampler[1]{};
	staticSampler[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;          //バイリニアフィルタ
	staticSampler[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;        // 0~1の範囲外をリピート
	staticSampler[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;      // 比較しない
	staticSampler[0].MaxLOD = D3D12_FLOAT32_MAX;                        // ありったけのMipmapを使う
	staticSampler[0].ShaderRegister = 0;                                // レジスタ0番を使う
	staticSampler[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  // PixelShaderで使う
	descriptionRootSignature.pStaticSamplers = staticSampler;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSampler);

	/// RootParameter作成。PixelShaderのMaterialとVertexShaderのTransform
	D3D12_ROOT_PARAMETER rootParameters[7] = {};                                                    

	// Material用
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;                                // CBV を使う
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;                             // PixelShader で使う
	rootParameters[0].Descriptor.ShaderRegister = 0;                                                // レジスタ番号 0 とバインド

	// Transform用
	static D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1]{};
	descriptorRangeForInstancing[0].BaseShaderRegister = 0;
	descriptorRangeForInstancing[0].NumDescriptors = 1;
	descriptorRangeForInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;						/// DescriptorTableを使う
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;								/// VertexShaderで使う
	rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;					/// Tableの中身の配列を指定
	rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);		/// Tableで利用する数


	// Texture用
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;						/// DescriptorTableを使う
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;									/// PixelShaderで使う
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;								/// Tableの中身の配列を指定
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);					/// Tableで利用する数

	// Lighting用
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParameters[3].Constants.Num32BitValues = 1;
	rootParameters[3].Constants.RegisterSpace = 0;
	rootParameters[3].Constants.ShaderRegister = 2; // b2

	// slot 4: InstanceOffset (b1, VertexShader)
	rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[4].Descriptor.ShaderRegister = 1;

	// Camera 用 (b1)
	rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;									/// 或 ALL
	rootParameters[5].Descriptor.ShaderRegister = 1;													/// b1

	// LightList 用 (StructuredBuffer<LightGPU> gLights : t1)
	static D3D12_DESCRIPTOR_RANGE lightListRange[1]{};
	lightListRange[0].BaseShaderRegister = 1; // t1
	lightListRange[0].NumDescriptors = 1;
	lightListRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	lightListRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[6].DescriptorTable.pDescriptorRanges = lightListRange;
	rootParameters[6].DescriptorTable.NumDescriptorRanges = _countof(lightListRange);


	descriptionRootSignature.pParameters = rootParameters;              // ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters);  // 配列の長さ


	// シリアライズしてバイナリにする
	signatureBlob_.Reset();
	errorBlob_.Reset();
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
												D3D_ROOT_SIGNATURE_VERSION_1, 
												signatureBlob_.GetAddressOf(),
												errorBlob_.GetAddressOf());
	if (FAILED(hr)) {
		if(errorBlob_){
			Logger::Log(reinterpret_cast<char*>(errorBlob_->GetBufferPointer()));
		}
		assert(false);
	}
	// バイナリを元に生成
	rootSignature_.Reset();
	hr = directXDriver_->GetDevice()->CreateRootSignature(0, 
														  signatureBlob_->GetBufferPointer(), 
														  signatureBlob_->GetBufferSize(), 
														  IID_PPV_ARGS(rootSignature_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	rootSignatureList_.push_back(rootSignature_);
	return rootSignature_.Get();
}

void PSOFactory::createInputLayout() {
	///InputLayout
	//D3D12_INPUT_ELEMENT_DESC inputElementDESCs[2] = {};
	inputElementDESCs_[0] = {};
	inputElementDESCs_[0].SemanticName = "POSITION";
	inputElementDESCs_[0].SemanticIndex = 0;
	inputElementDESCs_[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDESCs_[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDESCs_[1] = {};
	inputElementDESCs_[1].SemanticName = "TEXCOORD";
	inputElementDESCs_[1].SemanticIndex = 0;
	inputElementDESCs_[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDESCs_[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDESCs_[2] = {};
	inputElementDESCs_[2].SemanticName = "NORMAL";
	inputElementDESCs_[2].SemanticIndex = 0;
	inputElementDESCs_[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDESCs_[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputLayoutDESC_ = {};
	inputLayoutDESC_.pInputElementDescs = inputElementDESCs_;
	inputLayoutDESC_.NumElements = _countof(inputElementDESCs_);
}

void PSOFactory::SetBlendState() {
	// BlendStateの設定
	//D3D12_BLEND_DESC blendDesc{};
	blendDesc_ = {};
	// すべての色要素を書き込む
	blendDesc_.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc_.RenderTarget[0].BlendEnable = TRUE;
	blendDesc_.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc_.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc_.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc_.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc_.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc_.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
}

void PSOFactory::SetRasterizerState() {
	// RasterizerStateの設定
	//D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc_ = {};
	/// 裏面（時計回り）を表示しない
	rasterizerDesc_.CullMode = D3D12_CULL_MODE_BACK;
	/// 三角形の中を塗りつぶす
	rasterizerDesc_.FillMode = D3D12_FILL_MODE_SOLID;

}

void PSOFactory::ShaderCompile(LightModelType lightModelType) {
	/// Shaderをコンパイルする
	vertexShaderBlob_.Reset();
	pixelShaderBlob_.Reset();

	/// Shaderの置場
	std::string shaderFolder = "./kEngine/EngineAssets/Shader/";

	switch (lightModelType) {
	case LightModelType::Sprite2D:
		vertexShaderBlob_ = shader_compile_->CompileShader(ConvertString::SwitchStdStringWstring(shaderFolder + "Tile2D.VS.hlsl"), L"vs_6_0");
		pixelShaderBlob_ = shader_compile_->CompileShader(ConvertString::SwitchStdStringWstring(shaderFolder + "Tile2D.PS.hlsl"), L"ps_6_0");
		return;
	case LightModelType::Lambert:
	case LightModelType::HalfLambert:
	case LightModelType::PhongReflection:
	case LightModelType::BlinnPhongReflection:
		vertexShaderBlob_ = shader_compile_->CompileShader(ConvertString::SwitchStdStringWstring(shaderFolder + "Particle.VS.hlsl"), L"vs_6_0");
		pixelShaderBlob_ = shader_compile_->CompileShader(ConvertString::SwitchStdStringWstring(shaderFolder + "Particle.PS.hlsl"), L"ps_6_0", lightModelType);
		break;
	case LightModelType::FlameNeonGlow:
		vertexShaderBlob_ = shader_compile_->CompileShader(ConvertString::SwitchStdStringWstring(shaderFolder + "FlameNeonGlow.VS.hlsl"), L"vs_6_0");
		pixelShaderBlob_ = shader_compile_->CompileShader(ConvertString::SwitchStdStringWstring(shaderFolder + "FlameNeonGlow.PS.hlsl"), L"ps_6_0");
		break;
	}
	assert(vertexShaderBlob_.Get());
	assert(pixelShaderBlob_.Get());
}

void PSOFactory::SetDepthStencilState() {
	// DepthStencilStateの設定
	depthStencilDesc = {};
	// Depthの機能を有効化する
	depthStencilDesc.DepthEnable = true;
	// 書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	// 比較関数はLessEqual。つまり、近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
}

void PSOFactory::SetGraphicsPipelineState() {
	// graphicsPipelineStateに設定する情報をまとめる
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get(); // RootSignature
	graphicsPipelineStateDesc.InputLayout = inputLayoutDESC_; // InputLayout
	graphicsPipelineStateDesc.VS = { vertexShaderBlob_->GetBufferPointer(), vertexShaderBlob_->GetBufferSize() }; // VertexShader
	graphicsPipelineStateDesc.PS = { pixelShaderBlob_->GetBufferPointer(), pixelShaderBlob_->GetBufferSize() }; // PixelShader
	graphicsPipelineStateDesc.BlendState = blendDesc_; // BlendState
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc_; // RasterizerState
	// 書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	// 利用するトポロジ（形状）のタイプ。三角形
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// どのように画面に色を打ち込むかの設定（気にしなくて良い）
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	// DepthStencilの設定
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// 実際に生成
	graphicsPipelineState_.Reset();
	HRESULT hr = directXDriver_->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(graphicsPipelineState_.GetAddressOf()));
	assert(SUCCEEDED(hr));
}


