#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#include <dxgidebug.h>
#pragma comment(lib,"dxguid.lib")

#include <dxcapi.h>
#pragma comment(lib,"dxcompiler.lib")

#include "DirectXCore.h"


class Shader_compile{
public:
	~Shader_compile();

	void Initialize();

	IDxcBlob* CompileShader(// CompilerするShaderファイルへのパス
					  const std::wstring& filePath,
					  // Compilerに使用するProfile
					  const wchar_t* profile,
					  LightModelType modelType = LightModelType::NONE);


	IDxcUtils* getDxcUtils() { return dxcUtils; }
	IDxcCompiler3* getDxcCompiler() { return dxcCompiler; }
	IDxcIncludeHandler* getIncludeHandler() { return includeHandler; }

private:
	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;
	IDxcIncludeHandler* includeHandler = nullptr;
private:
	/// シェーダーをビルドしてコンパイルする
	IDxcResult* BuildAndCompileShader(const std::wstring& filePath, const wchar_t* profile, LightModelType modelType, HRESULT& hr, DxcBuffer& shaderSourceBuffer);
};
