#include "PSOManager.h"

void PSOManager::Initialize(DirectXCore* directXDriver) {

	/// 
	psoFactory_.Initialize(directXDriver);
	directXDriver_ = directXDriver;
	commandList_ = directXDriver_->GetCommandList();

	/// PSOcacheのファイルの位置を保存
	psoCacheFilePath_ = config::GetPsoCacheFolderPath() + config::GetPsoCacheFolderName();

	///// PSOを全部作る
	//CreateAllPSO();
	/// PSOcacheからPSOを作る
	LasyCreatePSO();
}

void PSOManager::Finalize() {

	psoKeyToHandleMap_.clear();
	psoMap_.clear();

}

Microsoft::WRL::ComPtr<ID3D12PipelineState> PSOManager::GetPso(PSOKey keys) {

	/// PSOKeyからハンドルを探す
	int handle = GetPSOHandle(keys);
	if (handle == -1) {
		Logger::Log("[kEngine]PSOManager::GetPso() PSOKey not found");
		assert(false);
	}
	return psoMap_[handle];

}

void PSOManager::SetPSO(PSOKey keys) {

	/// PSOKeyからハンドルを探す
	int newPSOHandle = GetPSOHandle(keys);

	/// PSOがない場合PSO生成する
	if (newPSOHandle == -1) {
		Logger::Log("[kEngine]PSOManager::SetPso() pso not found, create Pso");

		// factoryで新しいPSO生成
		auto pso = psoFactory_.createPSO(keys);

		// PSOKeyとハンドルのマップに追加
		int handle = (int)psoMap_.size();
		psoMap_[handle] = pso;
		psoKeyToHandleMap_[keys] = handle;

		// 記録
		AppendPSOKeyRecord(keys);

		// PSO更新
		newPSOHandle = handle;
	}

	/// ある場合、今使ってるPSOと比較。違う場合更新する
	if (currentPSOHandle_ != newPSOHandle) {

		// PSOをセット
		commandList_->SetPipelineState(psoMap_[newPSOHandle].Get());
		currentPSOHandle_ = newPSOHandle;

		// RootSignatureをセット
		rootSignature_ = psoFactory_.getRootSignatureFactory()->Make(keys, directXDriver_).Get();
		commandList_->SetGraphicsRootSignature(rootSignature_);
	}
}


void PSOManager::SetPSOStrong(PSOKey keys) {

	/// PSOKeyからハンドルを探す
	int newPSOHandle = GetPSOHandle(keys);

	/// PSOがない場合PSO生成する
	if (newPSOHandle == -1) {
		Logger::Log("[kEngine]PSOManager::SetPSOStrong() pso not found, create Pso");

		// factoryで新しいPSO生成
		auto pso = psoFactory_.createPSO(keys);

		// PSOKeyとハンドルのマップに追加
		int handle = (int)psoMap_.size();
		psoMap_[handle] = pso;
		psoKeyToHandleMap_[keys] = handle;

		// 記録
		AppendPSOKeyRecord(keys);

		// PSO更新
		newPSOHandle = handle;
	}

	///　どうしても更新する
	
	// PSOをセット
	commandList_->SetPipelineState(psoMap_[newPSOHandle].Get());
	currentPSOHandle_ = newPSOHandle;

	// RootSignatureをセット
	rootSignature_ = psoFactory_.getRootSignatureFactory()->Make(keys, directXDriver_).Get();
	commandList_->SetGraphicsRootSignature(rootSignature_);
}

void PSOManager::LasyCreatePSO() {
	const std::string fullPath = psoCacheFilePath_;

	std::ifstream input(fullPath);
	if (!input.is_open()) {
		std::cout << "[kEngine] PSOManager:LasyCreatePSO() Cache file not found, creating a new one: "
			<< fullPath << std::endl;
		CreateDefaultPSOcacheJson(fullPath);
		return;
	}

	nlohmann::json file;
	try {
		input >> file;
	} catch (...) {
		std::cout << "[kEngine] PSOManager:LasyCreatePSO() Cache corrupted, recreating.\n";
		CreateDefaultPSOcacheJson(fullPath);
		return;
	}

	auto& arr = file["pso_keys"];

	// 找到分隔符 0
	size_t startIndex = arr.size();
	for (size_t i = 0; i < arr.size(); i++) {
		if (arr[i].is_number_integer() && arr[i].get<int>() == 0) {
			startIndex = i + 1;
			break;
		}
	}

	if (startIndex >= arr.size()) {
		std::cout << "[kEngine] PSOManager: No valid start index found in cache." << std::endl;
		return;
	}

	// 讀取 PSOKey
	for (size_t i = startIndex; i < arr.size(); i++) {

		const auto& record = arr[i];

		// record 必須是 array，且大小為 2
		if (!record.is_array() || record.size() != 2) {
			std::cout << "[kEngine] PSOManager: Invalid record at index " << i << std::endl;
			continue;
		}

		const auto& psoFields = record[0];
		const auto& flags = record[1];

		if (!psoFields.is_array() || psoFields.size() != 6) {
			std::cout << "[kEngine] PSOManager: Invalid PSO fields at index " << i << std::endl;
			continue;
		}

		PSOKey key;
		key.lightModelType = (LightModelType)psoFields[0].get<int>();
		key.renderModelType = (RenderModelType)psoFields[1].get<int>();
		key.blendModeType = (BlendModeType)psoFields[2].get<int>();
		key.rasterizerMode = (RasterizerMode)psoFields[3].get<int>();
		key.depthStencilType = (DepthStencilType)psoFields[4].get<int>();
		key.primitiveType = (PrimitiveType)psoFields[5].get<int>();

		if (flags.is_array() && flags.size() > 0 && flags[0].get<int>() == 1) {
			key.featureMask |= FeatureFlags::EnvReflection;
		}

		auto pso = psoFactory_.createPSO(key);

		int handle = (int)psoMap_.size();
		psoMap_[handle] = pso;
		psoKeyToHandleMap_[key] = handle;
	}
}
void PSOManager::CreateDefaultPSOcacheJson(std::string fullPath) {

	// 直接生成一個新的 JSON 檔案
	nlohmann::json file;
	file["pso_keys"] = nlohmann::json::array();
	file["pso_keys"].push_back("OrderBy:");
	file["pso_keys"].push_back("LightModel,RenderModel,BlendMode,RasterizerMode,DepthStencil,PrimitiveType");
	file["pso_keys"].push_back("EnvReflection");
	file["pso_keys"].push_back(0);

	std::ofstream output(fullPath);
	if (!output.is_open()) {
		throw std::runtime_error("[kEngine] Failed to create JSON file: " + fullPath);
	}

	output << file.dump(4);
	output.close();
}

void PSOManager::AppendPSOKeyRecord(const PSOKey& key) {

	/// まずは予定したところにPSOKeyを保存したファイルがあるか探す
	const std::string fullPath = psoCacheFilePath_;

	/// 読み込み
	nlohmann::json file;
	{
		std::ifstream input(fullPath);
		if (!input.is_open()) {
			std::cout << "[kEngine] PSOManager: Cache file missing, creating new one.(weird)\n";
			CreateDefaultPSOcacheJson(fullPath);
			return;
		}

		try {
			input >> file;
		} catch (...) {
			std::cout << "[kEngine] PSOManager: Cache file corrupted, recreating.\n";
			CreateDefaultPSOcacheJson(fullPath);
			return;
		}
	}

	auto& arr = file["pso_keys"];

	// 只從 0 之後開始檢查重複
	size_t startIndex = 0;
	for (size_t i = 0; i < arr.size(); i++) {
		if (arr[i].is_number_integer() && arr[i].get<int>() == 0) {
			startIndex = i + 1;
			break;
		}
	}

	/// 重複チェック
	for (size_t i = startIndex; i < arr.size(); i++) {
		if (arr[i].is_array() && arr[i].size() == 2) {
			auto& psoFields = arr[i][0];
			auto& flags = arr[i][1];

			bool same =
				psoFields[0] == (int)key.lightModelType &&
				psoFields[1] == (int)key.renderModelType &&
				psoFields[2] == (int)key.blendModeType &&
				psoFields[3] == (int)key.rasterizerMode &&
				psoFields[4] == (int)key.depthStencilType &&
				psoFields[5] == (int)key.primitiveType &&
				psoFields[6] == (int)key.renderTargetFormatType &&
				flags[0] == (int)(key.featureMask & (uint64_t)FeatureFlags::EnvReflection);

			if (same)return;
		}
	}

	/// 書き込み
	arr.push_back(
		{
			{
				(int)key.lightModelType,
				(int)key.renderModelType,
				(int)key.blendModeType,
				(int)key.rasterizerMode,
				(int)key.depthStencilType,
				(int)key.renderTargetFormatType,
				(int)key.primitiveType
			},

			{
				(int)(key.featureMask & (uint64_t)FeatureFlags::EnvReflection)
			}
		});

	/// ファイルに保存
	std::ofstream output(fullPath);
	output << FormatPSOJson(file);
}

void PSOManager::CreateAllPSO() {

	int handle = 0;
	for (int LightNB = 0; LightNB < (int)LightModelType::NumLightModels; LightNB++) {
		for (int RenderNB = 0; RenderNB < (int)RenderModelType::NumOfRenderModels; RenderNB++) {
			for (int RasterNB = 0; RasterNB < (int)RasterizerMode::NumRasterizerModes; RasterNB++) {
				for (int BlendNB = 0; BlendNB < (int)BlendModeType::NumOfBlends; BlendNB++) {
					for (int DepthStencilNB = 0; DepthStencilNB < (int)DepthStencilType::NumberOfDepthStencilStates; DepthStencilNB++) {
						for (int PrimitiveNB = 0; PrimitiveNB < (int)PrimitiveType::NumOfPrimitive; PrimitiveNB++) {
							if (PrimitiveNB == (int)PrimitiveType::UNDEFINED)continue;
							/// このループの組み合わせのPSOKeyを作る
							PSOKey key;
							key.lightModelType = (LightModelType)LightNB;
							key.renderModelType = (RenderModelType)RenderNB;
							key.rasterizerMode = (RasterizerMode)RasterNB;
							key.blendModeType = (BlendModeType)BlendNB;
							key.depthStencilType = (DepthStencilType)DepthStencilNB;
							key.primitiveType = (PrimitiveType)PrimitiveNB;
							key.renderTargetFormatType = (RenderTargetFormatType)config::default_RenderTargetFormatType_;

							/// PSOKeyとハンドルのマップに追加
							psoMap_[handle] = psoFactory_.createPSO(key);

							/// 探す用のマップに追加
							psoKeyToHandleMap_[key] = handle;

							handle++;
						}
					}
				}
			}
		}
	}

}

int PSOManager::GetPSOHandle(PSOKey keys) {

	auto it = psoKeyToHandleMap_.find(keys);
	if (it != psoKeyToHandleMap_.end()) {
		return it->second;
	}
	return -1;

}

std::string PSOManager::FormatPSOJson(const nlohmann::json& file) {

	std::string s = file.dump(4);
	std::string out;
	out.reserve(s.size());

	bool afterZero = false;
	int bracketDepth = -1;
	bool compressingRecord = false;

	for (size_t i = 0; i < s.size(); ++i) {
		char c = s[i];

		// ============================
		// 0 之前不壓縮
		// ============================
		if (!afterZero) {
			out += c;

			if (c == '0') {
				bool isZero = true;
				if (i > 0 && std::isdigit(s[i - 1])) isZero = false;
				if (i + 1 < s.size() && std::isdigit(s[i + 1])) isZero = false;
				if (isZero) afterZero = true;
				bracketDepth = 0;
			}
			continue;
		}

		// ============================
		// 0 之後開始壓縮 PSOKey
		// ============================

		if (c == '[') {
			bracketDepth++;
		}


		if (c == ']') {
			bracketDepth--;
		}

		// 壓縮 record：只保留數字與逗號
		if (bracketDepth > 0) {
			if (!(c == ' ' || c == '\n')) {
				out += c;
			}
			continue;
		}

		// 其他正常輸出
		out += c;
	}

	return out;
}

