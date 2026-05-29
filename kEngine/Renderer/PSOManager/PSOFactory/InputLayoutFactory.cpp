#include "InputLayoutFactory.h"
#include "Logger.h"

InputLayoutFactory::InputLayoutFactory() {
	
	inputLayoutRegistry[RenderModelType::Sprite2D] = [this](PSOKey& key) { return MakeInputLayoutStatic(); };
	inputLayoutRegistry[RenderModelType::Static] = [this](PSOKey& key) { return MakeInputLayoutStatic(); };
	inputLayoutRegistry[RenderModelType::Skinned] = [this](PSOKey& key) { return MakeInputLayoutSkinning(); };
	inputLayoutRegistry[RenderModelType::DebugLine] = [this](PSOKey& key) { return MakeInputLayoutDebugLine(); };
	inputLayoutRegistry[RenderModelType::Environment] = [this](PSOKey& key) { return MakeInputLayoutStatic(); };
	inputLayoutRegistry[RenderModelType::FlameNeonGlow] = [this](PSOKey& key) { return MakeInputLayoutStatic(); };

	/// PostProcess用の描画モデル
	inputLayoutRegistry[RenderModelType::FullscreenQuad] = [this](PSOKey& key) { return MakeInputLayoutFullscreenQuad(); };
	inputLayoutRegistry[RenderModelType::ColorGradient] = [this](PSOKey& key) { return MakeInputLayoutFullscreenQuad(); };
	inputLayoutRegistry[RenderModelType::Vignette] = [this](PSOKey& key) { return MakeInputLayoutFullscreenQuad(); };
	inputLayoutRegistry[RenderModelType::Blur] = [this](PSOKey& key) { return MakeInputLayoutFullscreenQuad(); };
}

D3D12_INPUT_LAYOUT_DESC InputLayoutFactory::Make(PSOKey& key) {

	auto it = inputLayoutRegistry.find(key.renderModelType);
	if (it == inputLayoutRegistry.end()) {

		/// ここに止まるのはだいたい登録忘れ
		Logger::Log("[kEngine]InputLayoutFactory::MakeInputLayout() RenderModelType not found in inputLayoutRegistry");
		assert(false);

	} else {
		return it->second(key);
	}

	return D3D12_INPUT_LAYOUT_DESC();
}

D3D12_INPUT_LAYOUT_DESC InputLayoutFactory::MakeInputLayoutStatic() {

	///InputLayout
	static D3D12_INPUT_ELEMENT_DESC inputElementDESCs[3] = {};	/// staticにしてるのは、この循環終わったあとで消されないようにするため
	inputElementDESCs[0] = {};
	inputElementDESCs[0].SemanticName = "POSITION";
	inputElementDESCs[0].SemanticIndex = 0;
	inputElementDESCs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDESCs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDESCs[1] = {};
	inputElementDESCs[1].SemanticName = "TEXCOORD";
	inputElementDESCs[1].SemanticIndex = 0;
	inputElementDESCs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDESCs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDESCs[2] = {};
	inputElementDESCs[2].SemanticName = "NORMAL";
	inputElementDESCs[2].SemanticIndex = 0;
	inputElementDESCs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDESCs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDESC = {};
	inputLayoutDESC.pInputElementDescs = inputElementDESCs;
	inputLayoutDESC.NumElements = _countof(inputElementDESCs);

	return inputLayoutDESC;
}

D3D12_INPUT_LAYOUT_DESC InputLayoutFactory::MakeInputLayoutSkinning() {
	
	///InputLayout
	static D3D12_INPUT_ELEMENT_DESC inputElementDESCs[5] = {};	/// staticにしてるのは、この循環終わったあとで消されないようにするため
	inputElementDESCs[0] = {};
	inputElementDESCs[0].SemanticName = "POSITION";
	inputElementDESCs[0].SemanticIndex = 0;
	inputElementDESCs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDESCs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDESCs[1] = {};
	inputElementDESCs[1].SemanticName = "TEXCOORD";
	inputElementDESCs[1].SemanticIndex = 0;
	inputElementDESCs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDESCs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDESCs[2] = {};
	inputElementDESCs[2].SemanticName = "NORMAL";
	inputElementDESCs[2].SemanticIndex = 0;
	inputElementDESCs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDESCs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDESCs[3] = {};
	inputElementDESCs[3].SemanticName = "WEIGHT";
	inputElementDESCs[3].SemanticIndex = 0;
	inputElementDESCs[3].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDESCs[3].InputSlot = 1;
	inputElementDESCs[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDESCs[4] = {};
	inputElementDESCs[4].SemanticName = "INDEX";
	inputElementDESCs[4].SemanticIndex = 0;
	inputElementDESCs[4].Format = DXGI_FORMAT_R32G32B32A32_SINT;
	inputElementDESCs[4].InputSlot = 1;
	inputElementDESCs[4].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDESC = {};
	inputLayoutDESC.pInputElementDescs = inputElementDESCs;
	inputLayoutDESC.NumElements = _countof(inputElementDESCs);

	return inputLayoutDESC;
}

D3D12_INPUT_LAYOUT_DESC InputLayoutFactory::MakeInputLayoutDebugLine() {
	///InputLayout
	static D3D12_INPUT_ELEMENT_DESC inputElementDESCs[2] = {};
	inputElementDESCs[0] = {};
	inputElementDESCs[0].SemanticName = "POSITION";
	inputElementDESCs[0].SemanticIndex = 0;
	inputElementDESCs[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDESCs[0].InputSlot = 0;
	inputElementDESCs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDESCs[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	inputElementDESCs[0].InstanceDataStepRate = 0;

	inputElementDESCs[1] = {};
	inputElementDESCs[1].SemanticName = "COLOR";
	inputElementDESCs[1].SemanticIndex = 0;
	inputElementDESCs[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDESCs[1].InputSlot = 0;
	inputElementDESCs[1].AlignedByteOffset = 12; // float3 = 12 bytes
	inputElementDESCs[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	inputElementDESCs[1].InstanceDataStepRate = 0;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDESC = {};
	inputLayoutDESC.pInputElementDescs = inputElementDESCs;
	inputLayoutDESC.NumElements = _countof(inputElementDESCs);

	return inputLayoutDESC;
}

D3D12_INPUT_LAYOUT_DESC InputLayoutFactory::MakeInputLayoutFullscreenQuad() {
	///InputLayout
	static D3D12_INPUT_ELEMENT_DESC inputElementDESCs[2] = {};
	inputElementDESCs[0] = {};
	inputElementDESCs[0].SemanticName = "POSITION";
	inputElementDESCs[0].SemanticIndex = 0;
	inputElementDESCs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDESCs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDESCs[1] = {};
	inputElementDESCs[1].SemanticName = "TEXCOORD";
	inputElementDESCs[1].SemanticIndex = 0;
	inputElementDESCs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDESCs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDESC = {};
	inputLayoutDESC.pInputElementDescs = inputElementDESCs;
	inputLayoutDESC.NumElements = _countof(inputElementDESCs);

	return inputLayoutDESC;
}
