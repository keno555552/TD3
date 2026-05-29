#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include "PSOManager/PSOFactory/PSOFactory.h"
#include "PSOManager/PSOKeys.h"

#include <fstream>
#include "externals/nlohmann/json.hpp"

using std::map;
using std::unordered_map;
using std::vector;
using std::shared_ptr;
using Microsoft::WRL::ComPtr;



class PSOManager{
public:

	void Initialize(DirectXCore* directXDriver);
	void Finalize();

	Microsoft::WRL::ComPtr <ID3D12PipelineState> GetPso(PSOKey keys);
	void SetPSO(PSOKey keys);
	void SetPSOStrong(PSOKey keys);

private:

	/// system
	DirectXCore* directXDriver_{};					/*依存*/
	ID3D12GraphicsCommandList* commandList_{};		/*依存*/


	/// 現在のPSO
	int currentPSOHandle_ = -1;
	ID3D12RootSignature* rootSignature_ = nullptr; 			// Listからもセーブしたから解放しなくていい	

	/// PSOListとPSOKeyのマップ
	unordered_map <int, ComPtr<ID3D12PipelineState>> psoMap_;
	map<PSOKey, int> psoKeyToHandleMap_;
	PSOFactory psoFactory_;

	/// PSOcacheのファイルに位置
	std::string psoCacheFilePath_;

private:

	/// =========== PSOを作成する関数 =========== ///

	/// 記録されたPSOKeyからPSOを作成する
	void LasyCreatePSO();
	void CreateDefaultPSOcacheJson(std::string fullPath);
	void AppendPSOKeyRecord(const PSOKey& key);
	std::string FormatPSOJson(const nlohmann::json& file);
	

	/// 全部のPSOを作成する。もはやバガのやり方(現時点でもう20000くらいのPSOがある
	void CreateAllPSO();
	int GetPSOHandle(PSOKey keys);

};

/// =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+= ///
/// 
/// Note: RootSignatureが入力の構造を定義する。全部別々じゃないといけない。入力側みたいな構造。
///		  b,t,u,sはPSやVSの中に定義されてる。0,1,2,3はShader内で被らないようにするための番号。
///		  bは定数バッファ、tはSRV、uはUAV、sはSampler。Shader側で定義されてるものと合わせる必要がある。
///		  tはSRVを使うことが多い。uはRWTexture2DとかRWBufferとかで使う。sはSamplerStateで使う。
///       uはRWTexture2DとかRWBufferとかで使う。sはSamplerStateで使う。
///       sはSamplerStateで使う。
/// 
///       b = D3D12_ROOT_PARAMETER_TYPE_CBV
///		  t = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE
///		  u = D3D12_ROOT_PARAMETER_TYPE_UAV
///		  s = D3D12_ROOT_PARAMETER_TYPE_SAMPLER
/// 
/// =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+= ///