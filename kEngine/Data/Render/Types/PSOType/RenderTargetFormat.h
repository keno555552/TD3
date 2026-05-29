#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgiformat.h>

enum class RenderTargetFormatType {

	/// スグリントに適したフォーマット
	BackBuffer,
	/// Textureによく使われているフォーマット
	RGBA8,
	/// HDRレンダリングに適したフォーマット
	RGBA16F,
	/// 深度バッファ、明度やらに適したフォーマット
	R32F,
	/// 深度テキスチャに適したフォーマット
	Depth32F,
};

DXGI_FORMAT GetDXGIFormat(RenderTargetFormatType format);
