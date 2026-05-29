#pragma once
#include <wrl/client.h>      // Microsoft::WRL::ComPtr
#include <d3d12.h>           // ID3D12Resource, descriptor handles
#include <dxgiformat.h>      // DXGI_FORMAT
#include "Vector4.h"


struct RenderTexture {

	/// RenderTextureのリソースとビューハンドルをまとめた構造体
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleCPU{};
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU{};
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU{};

	D3D12_RESOURCE_STATES currentState{};

	/// DepthStencil用のリソースとビューハンドル
	Microsoft::WRL::ComPtr<ID3D12Resource> depthResource;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandleCPU{};

	/// 描画する大きさ
	UINT width{};
	UINT height{};

	/// フォーマット
	DXGI_FORMAT format;

	/// クリアカラー
	Vector4 clearColor;
};