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

	void Initialize();
	void Finalize();

	Microsoft::WRL::ComPtr <IDxcBlob> CompileShader(// CompilerするShaderファイルへのパス
					  const std::wstring& filePath,
					  // Compilerに使用するProfile
					  const wchar_t* profile,
					  LightModelType modelType = LightModelType::NONE);

private:
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils = nullptr;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler = nullptr;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler = nullptr;
private:
	/// シェーダーをビルドしてコンパイルする
	Microsoft::WRL::ComPtr<IDxcResult> BuildAndCompileShader(const std::wstring& filePath, const wchar_t* profile, LightModelType modelType, HRESULT& hr, DxcBuffer& shaderSourceBuffer);
};
