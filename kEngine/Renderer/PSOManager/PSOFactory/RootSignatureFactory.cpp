#include "RootSignatureFactory.h"
#include "Logger.h"

#include "Data/Render/GPUData/RenderCommandGPU.h"

RootSignatureFactory::RootSignatureFactory() {

	rootSignatureRegistry[RenderModelType::Sprite2D] = [this](DirectXCore* directXDriver_, PSOKey& key) { return MakeStatic(directXDriver_, key); };
	rootSignatureRegistry[RenderModelType::Static] = [this](DirectXCore* directXDriver_, PSOKey& key) { return MakeStatic(directXDriver_, key); };
	rootSignatureRegistry[RenderModelType::Skinned] = [this](DirectXCore* directXDriver_, PSOKey& key) { return MakeStaticSkinning(directXDriver_, key); };
	rootSignatureRegistry[RenderModelType::DebugLine] = [this](DirectXCore* directXDriver_, PSOKey& key) { return MakeStatic(directXDriver_, key); };
	rootSignatureRegistry[RenderModelType::Environment] = [this](DirectXCore* directXDriver_, PSOKey& key) { return MakeStatic(directXDriver_, key); };
	rootSignatureRegistry[RenderModelType::FlameNeonGlow] = [this](DirectXCore* directXDriver_, PSOKey& key) { return MakeStatic(directXDriver_, key); };

	/// PostProcess用の描画モデル
	rootSignatureRegistry[RenderModelType::FullscreenQuad] = [this](DirectXCore* directXDriver_, PSOKey& key) { return MakeStaticFullscreenQuad(directXDriver_, key); };
	rootSignatureRegistry[RenderModelType::ColorGradient] = [this](DirectXCore* directXDriver_, PSOKey& key) { return MakeStaticFullscreenQuad(directXDriver_, key); };
	rootSignatureRegistry[RenderModelType::Vignette] = [this](DirectXCore* directXDriver_, PSOKey& key) { return MakeStaticFullscreenQuad(directXDriver_, key); };
	rootSignatureRegistry[RenderModelType::Blur] = [this](DirectXCore* directXDriver_, PSOKey& key) { return MakeStaticFullscreenQuad(directXDriver_, key); };
}

Microsoft::WRL::ComPtr <ID3D12RootSignature> RootSignatureFactory::Make(PSOKey& key, DirectXCore* directXDriver_) {

	auto checker = rootSignatureMap_.find(key.renderModelType);
	if (checker != rootSignatureMap_.end()) {
		return checker->second;
	}

	auto it = rootSignatureRegistry.find(key.renderModelType);
	if (it == rootSignatureRegistry.end()) {

		/// ここに止まるのはだいたい登録忘れ
		Logger::Log("[kEngine]RootSignatureFactory::Make() RenderModelType not found in rootSignatureRegistry");
		assert(false);

	} else {
		auto rootSignature = it->second(directXDriver_, key);
		rootSignatureMap_[key.renderModelType] = rootSignature;
		return rootSignature;
	}

	return nullptr;
}

Microsoft::WRL::ComPtr <ID3D12RootSignature> RootSignatureFactory::MakeStatic(DirectXCore* directXDriver_, PSOKey& key) {

	/// b0:TransformMatrices		(VS)
	/// b1:InstanceOffset			(VS)
	
	/// b1:Camera					(PS)
	/// b2:LightingCount			(PS)
	/// t0:Texture					(PS)
	/// t1:LightList				(PS)
	/// t2:ENVIRONMENT REFLECTION	(PS)

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


	/// RootParameter作成。

	/// ============================================ 常駐スロット =========================================== ///
	D3D12_ROOT_PARAMETER rootParameters[8] = {};

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

	// TransformMatrices（b0, VertexShader）
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;						/// DescriptorTableを使う
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;								/// VertexShaderで使う
	rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;					/// Tableの中身の配列を指定
	rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);		/// Tableで利用する数


	// Texture用 (t0)
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;						/// DescriptorTableを使う
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;									/// PixelShaderで使う
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;								/// Tableの中身の配列を指定
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);					/// Tableで利用する数

	// LightingCount (b2, GlowSphereに使える)
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

	// EnvironmentReflection 用 (t2)
	static D3D12_DESCRIPTOR_RANGE environmentReflectionRange[1]{};
	environmentReflectionRange[0].BaseShaderRegister = 2; // t2
	environmentReflectionRange[0].NumDescriptors = 1;
	environmentReflectionRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	environmentReflectionRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[7].DescriptorTable.pDescriptorRanges = environmentReflectionRange;
	rootParameters[7].DescriptorTable.NumDescriptorRanges = _countof(environmentReflectionRange);


	/// =========================================== 非常駐スロット ========================================== ///




	descriptionRootSignature.pParameters = rootParameters;              // ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters);  // 配列の長さ


	// シリアライズしてバイナリにする
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob_;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob_;
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1,
		signatureBlob_.GetAddressOf(),
		errorBlob_.GetAddressOf());
	if (FAILED(hr)) {
		if (errorBlob_) {
			Logger::Log(reinterpret_cast<char*>(errorBlob_->GetBufferPointer()));
		}
		assert(false);
	}
	// バイナリを元に生成
	Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature;
	hr = directXDriver_->GetDevice()->CreateRootSignature(0,
		signatureBlob_->GetBufferPointer(),
		signatureBlob_->GetBufferSize(),
		IID_PPV_ARGS(rootSignature.GetAddressOf()));
	assert(SUCCEEDED(hr));

	return rootSignature;
}

Microsoft::WRL::ComPtr <ID3D12RootSignature> RootSignatureFactory::MakeStaticSkinning(DirectXCore* directXDriver_, PSOKey& key) {

	/// t0:TransformMatrices	(VS)
	/// b1:InstanceOffset		(VS)
	/// t1:Well(skeletonSpace)	(VS)

	/// b1:Camera				(PS)
	/// b2:LightingCount		(PS)
	/// t0:Texture				(PS)
	/// t1:LightList			(PS)


	///RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

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
	/// ============================================ 常駐スロット ========================================== ///
	D3D12_ROOT_PARAMETER rootParameters[9] = {};

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

	// TransformMatrices（b0, VS）
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;						/// DescriptorTableを使う
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;								/// VertexShaderで使う
	rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;					/// Tableの中身の配列を指定
	rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);		/// Tableで利用する数

	// DescriptorRange作成。SRVを使う
	D3D12_DESCRIPTOR_RANGE descriptorRange[1]{};
	descriptorRange[0].BaseShaderRegister = 0; // 0から始まる
	descriptorRange[0].NumDescriptors = 1; // 数は1つ
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // 連続している

	// Texture用 (t0, PS）
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;						/// DescriptorTableを使う
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;									/// PixelShaderで使う
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;								/// Tableの中身の配列を指定
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);					/// Tableで利用する数

	// LightingCount (b2, PS)
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS; // b
	rootParameters[3].Constants.Num32BitValues = 1;
	rootParameters[3].Constants.RegisterSpace = 0;
	rootParameters[3].Constants.ShaderRegister = 2;								 // 2

	// InstanceOffset (b1, VS)
	rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[4].Descriptor.ShaderRegister = 1;

	// Camera 用 (b1, PS)
	rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;									/// 或 ALL
	rootParameters[5].Descriptor.ShaderRegister = 1;													/// b1

	// LightList 用 (t1, PS)
	static D3D12_DESCRIPTOR_RANGE lightListRange[1]{};
	lightListRange[0].BaseShaderRegister = 1; // t1
	lightListRange[0].NumDescriptors = 1;
	lightListRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	lightListRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[6].DescriptorTable.pDescriptorRanges = lightListRange;
	rootParameters[6].DescriptorTable.NumDescriptorRanges = _countof(lightListRange);

	// EnvironmentReflection 用 (t2)
	static D3D12_DESCRIPTOR_RANGE environmentReflectionRange[1]{};
	environmentReflectionRange[0].BaseShaderRegister = 2; // t2
	environmentReflectionRange[0].NumDescriptors = 1;
	environmentReflectionRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	environmentReflectionRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[7].DescriptorTable.pDescriptorRanges = environmentReflectionRange;
	rootParameters[7].DescriptorTable.NumDescriptorRanges = _countof(environmentReflectionRange);

	descriptionRootSignature.pParameters = rootParameters;              // ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters);  // 配列の長さ

	/// =========================================== 非常駐スロット ========================================== ///

	// SkinningのWell (t1, VS)
	static D3D12_DESCRIPTOR_RANGE wellRange[1]{};
	wellRange[0].BaseShaderRegister = 1; // t1
	wellRange[0].NumDescriptors = 1;
	wellRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	wellRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParameters[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[8].DescriptorTable.pDescriptorRanges = wellRange;
	rootParameters[8].DescriptorTable.NumDescriptorRanges = _countof(wellRange);


	descriptionRootSignature.pParameters = rootParameters;              // ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters);  // 配列の長さ


	// シリアライズしてバイナリにする
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob_;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob_;
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1,
		signatureBlob_.GetAddressOf(),
		errorBlob_.GetAddressOf());
	if (FAILED(hr)) {
		if (errorBlob_) {
			Logger::Log(reinterpret_cast<char*>(errorBlob_->GetBufferPointer()));
		}
		assert(false);
	}
	// バイナリを元に生成
	Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature;
	hr = directXDriver_->GetDevice()->CreateRootSignature(0,
		signatureBlob_->GetBufferPointer(),
		signatureBlob_->GetBufferSize(),
		IID_PPV_ARGS(rootSignature.GetAddressOf()));
	assert(SUCCEEDED(hr));

	return rootSignature;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignatureFactory::MakeStaticFullscreenQuad(DirectXCore* directXDriver_, PSOKey& key) {
	key;	/// 使われなかった
	
	/// s0:Sampler 				(PS)
	/// t0:SourceTexture		(PS)
	/// b0:RenderCommand		(PS)

	///RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	/// -------------------------------- Static Sampler (s0) -------------------------------- ///

	// StaticSampler (s0, PixelShader)
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
	/// ------------------------------- Descriptor Range (t0) ------------------------------- ///
	D3D12_ROOT_PARAMETER rootParameters[2] = {};

	// Transform用descripter
	static D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1]{};
	descriptorRangeForInstancing[0].BaseShaderRegister = 0;
	descriptorRangeForInstancing[0].NumDescriptors = 1;
	descriptorRangeForInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// TransformMatrices（t0, PixelShader）
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;						/// DescriptorTableを使う
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;									/// PixelShaderで使う
	rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;					/// Tableの中身の配列を指定
	rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);		/// Tableで利用する数

	// RenderCommand（b0, PixelShader）
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;							/// 32ビット定数を使う
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;									/// PixelShaderで使う
	rootParameters[1].Constants.Num32BitValues = sizeof(RenderCommandGPU) / sizeof(uint32_t);			/// 定数の数

	descriptionRootSignature.pParameters = rootParameters;              // ルートパラメータ配列へのポインタ
	descriptionRootSignature.NumParameters = _countof(rootParameters);  // 配列の長さ


	// シリアライズしてバイナリにする
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob_;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob_;
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1,
		signatureBlob_.GetAddressOf(),
		errorBlob_.GetAddressOf());
	if (FAILED(hr)) {
		if (errorBlob_) {
			Logger::Log(reinterpret_cast<char*>(errorBlob_->GetBufferPointer()));
		}
		assert(false);
	}
	// バイナリを元に生成
	Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature;
	hr = directXDriver_->GetDevice()->CreateRootSignature(0,
		signatureBlob_->GetBufferPointer(),
		signatureBlob_->GetBufferSize(),
		IID_PPV_ARGS(rootSignature.GetAddressOf()));
	assert(SUCCEEDED(hr));

	return rootSignature;
}
