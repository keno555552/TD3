#include "BlendStateFactory.h"
#include "Logger.h"

BlendStateFactory::BlendStateFactory() {

	blendStateRegistry[BlendModeType::AlphaBlend] = [this](PSOKey& key) { return MakeBlendNormal(); };
	blendStateRegistry[BlendModeType::AddBlend] = [this](PSOKey& key) { return MakeBlendAddAlpha(); };
	blendStateRegistry[BlendModeType::SubtractBlend] = [this](PSOKey& key) { return MakeBlendSubAlpha(); };
	blendStateRegistry[BlendModeType::MultiplyBlend] = [this](PSOKey& key) { return MakeBlendMultiply(); };
	blendStateRegistry[BlendModeType::ScreenBlend] = [this](PSOKey& key) { return MakeBlendScreen(); };
	blendStateRegistry[BlendModeType::Opaque] = [this](PSOKey& key) { return MakeBlendOpaque(); };
}

D3D12_BLEND_DESC BlendStateFactory::Make(PSOKey& key) {

	/// PSOKeyのRenderModelTypeからShaderを作る関数を呼び出す
	auto it = blendStateRegistry.find(key.blendModeType);
	if (it != blendStateRegistry.end()) {
		return it->second(key);
	} else {

		/// ここに止まるのはだいたい登録忘れ
		Logger::Log("[kEngine]BlendStateFactory::MakeBlendStateFactory() BlendModeType not found in blendStateRegistry_");
		assert(false);
	}

	return D3D12_BLEND_DESC();
}


D3D12_BLEND_DESC BlendStateFactory::MakeBlendOpaque() {

	// BlendStateの設定
	D3D12_BLEND_DESC blendDesc_ = {};

	// Blendしない
	blendDesc_.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc_.RenderTarget[0].BlendEnable = FALSE;

	return blendDesc_;
}

D3D12_BLEND_DESC BlendStateFactory::MakeBlendNormal() {

	// BlendStateの設定
	D3D12_BLEND_DESC blendDesc_ = {};
	// すべての色要素を書き込む
	blendDesc_.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc_.RenderTarget[0].BlendEnable = TRUE;
	// RGBブレンド
	blendDesc_.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc_.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc_.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	// Alphaブレンド
	blendDesc_.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc_.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc_.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

	return blendDesc_;
}

D3D12_BLEND_DESC BlendStateFactory::MakeBlendAddAlpha() {

	D3D12_BLEND_DESC blendDesc_ = {};
	
	blendDesc_.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc_.RenderTarget[0].BlendEnable = TRUE;
	// RGBブレンド
	blendDesc_.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc_.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc_.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	// Alphaブレンド
	blendDesc_.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc_.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc_.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

	return blendDesc_;
}

D3D12_BLEND_DESC BlendStateFactory::MakeBlendSubAlpha() {

	D3D12_BLEND_DESC blendDesc_ = {};

	blendDesc_.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc_.RenderTarget[0].BlendEnable = TRUE;
	// RGBブレンド
	blendDesc_.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc_.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
	blendDesc_.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	// Alphaブレンド
	blendDesc_.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc_.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc_.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

	return blendDesc_;
}

D3D12_BLEND_DESC BlendStateFactory::MakeBlendMultiply() {

	D3D12_BLEND_DESC blendDesc_ = {};

	blendDesc_.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc_.RenderTarget[0].BlendEnable = TRUE;
	// RGBブレンド
	blendDesc_.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
	blendDesc_.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc_.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
	// Alphaブレンド
	blendDesc_.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc_.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc_.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

	return blendDesc_;
}

D3D12_BLEND_DESC BlendStateFactory::MakeBlendScreen() {

	D3D12_BLEND_DESC blendDesc_ = {};

	blendDesc_.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc_.RenderTarget[0].BlendEnable = TRUE;
	// RGBブレンド
	blendDesc_.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
	blendDesc_.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc_.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	// Alphaブレンド
	blendDesc_.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc_.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc_.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

	return blendDesc_;
}
