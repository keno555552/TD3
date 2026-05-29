#include "RasterizerModeFactory.h"

RasterizerModeFactory::RasterizerModeFactory() {

	rasterizerModeRegistry[RasterizerMode::CullBack] = [this](PSOKey& key) { return MakeRasterizerModeNormal(); };
	rasterizerModeRegistry[RasterizerMode::CullFront] = [this](PSOKey& key) { return MakeRasterizerModeCullFront(); };
	rasterizerModeRegistry[RasterizerMode::CullNone] = [this](PSOKey& key) { return MakeRasterizerModeCullNone(); };
	rasterizerModeRegistry[RasterizerMode::Wireframe] = [this](PSOKey& key) { return MakeRasterizerModeWireframe(); };

}

D3D12_RASTERIZER_DESC RasterizerModeFactory::Make(PSOKey& key) {

	/// PSOKeyのRenderModelTypeからShaderを作る関数を呼び出す
	auto it = rasterizerModeRegistry.find(key.rasterizerMode);
	if (it != rasterizerModeRegistry.end()) {
		return it->second(key);
	} else {

		/// ここに止まるのはだいたい登録忘れ
		Logger::Log("[kEngine]BlendStateFactory::MakeBlendStateFactory() BlendModeType not found in blendStateRegistry_");
		assert(false);
	}

	return D3D12_RASTERIZER_DESC();
}

D3D12_RASTERIZER_DESC RasterizerModeFactory::MakeRasterizerModeNormal() {

	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	/// 裏面（時計回り）を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	/// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	return rasterizerDesc;
}

D3D12_RASTERIZER_DESC RasterizerModeFactory::MakeRasterizerModeCullFront() {

	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	/// 表面（時計回り）を表示する
	rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;
	/// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	return rasterizerDesc;

}

D3D12_RASTERIZER_DESC RasterizerModeFactory::MakeRasterizerModeCullNone() {

	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	/// 全部表示する
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	/// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	return rasterizerDesc;

}

D3D12_RASTERIZER_DESC RasterizerModeFactory::MakeRasterizerModeWireframe() {

	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	/// 裏面（時計回り）を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	/// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;

	return rasterizerDesc;
}
