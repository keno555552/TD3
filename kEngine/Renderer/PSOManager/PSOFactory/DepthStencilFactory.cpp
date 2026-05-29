#include "DepthStencilFactory.h"
#include "Logger.h"

DepthStencilFactory::DepthStencilFactory() {

	depthStenctilRegistry[DepthStencilType::Default] = [this](PSOKey& key) {return MakeDepthStenctilStateNormal(); };
	depthStenctilRegistry[DepthStencilType::ReadOnly] = [this](PSOKey& key) {return MakeDepthStenctilStateReadOnly(); };
	depthStenctilRegistry[DepthStencilType::Disable] = [this](PSOKey& key) {return MakeDepthStenctilStateDisable(); };
	depthStenctilRegistry[DepthStencilType::Greater] = [this](PSOKey& key) {return MakeDepthStenctilStateGreater(); };

}

D3D12_DEPTH_STENCIL_DESC DepthStencilFactory::Make(PSOKey& key) {

	/// PSOKeyのRenderModelTypeからShaderを作る関数を呼び出す
	auto it = depthStenctilRegistry.find(key.depthStencilType);
	if (it != depthStenctilRegistry.end()) {
		return it->second(key);
	} else {

		/// ここに止まるのはだいたい登録忘れ
		Logger::Log("[kEngine]DepthStenctilStateFactory::MakeDepthStenctilState() DepthStenctilStateFactory not found in depthStenctilRegistry_");
		assert(false);
	}

	return D3D12_DEPTH_STENCIL_DESC();
}

D3D12_DEPTH_STENCIL_DESC DepthStencilFactory::MakeDepthStenctilStateNormal() {

	// DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
	// Depthの機能を有効化する
	depthStencilDesc.DepthEnable = true;
	// 書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	// 比較関数はLessEqual。つまり、近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// Stencilの設定はデフォルトでいいと思うので、特にいじらない(またいじったことないどころ
	//depthStencilDesc.StencilEnable = FALSE;
	//depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	//depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	//depthStencilDesc.FrontFace = D3D12_DEFAULT_STENCIL_OP;
	//depthStencilDesc.BackFace = D3D12_DEFAULT_STENCIL_OP;

	return depthStencilDesc;
}

D3D12_DEPTH_STENCIL_DESC DepthStencilFactory::MakeDepthStenctilStateReadOnly() {

	// DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
	// Depthの機能を有効化する
	depthStencilDesc.DepthEnable = true;
	// 深度の書き込みをしない
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	// 比較関数はLessEqual。つまり、近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	return depthStencilDesc;
}

D3D12_DEPTH_STENCIL_DESC DepthStencilFactory::MakeDepthStenctilStateDisable() {

	// DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
	// Depthの機能を無効化する
	depthStencilDesc.DepthEnable = FALSE;
	// 深度の書き込みをしない
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	// 比較関数はAlways。つまり、常に描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	return depthStencilDesc;
}

D3D12_DEPTH_STENCIL_DESC DepthStencilFactory::MakeDepthStenctilStateGreater() {

	// DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
	// Depthの機能を有効化する
	depthStencilDesc.DepthEnable = true;
	// 書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	// 比較関数はGreater。つまり、遠ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;

	return depthStencilDesc;
}
