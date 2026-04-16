#include "Model.h"
#include "filesystem"

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {

	///1. 中で必要となる変数の宣言
	MaterialData materialData;
	std::string line;
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());
	///2. ファイルを開く
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		// identifierに応じた処理
		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			// 連結してファイルパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}
	///3. 実際にファイルを読む、MaterialDataを構築していく
	///4. MaterialDataを返す
	return materialData;
}

MaterialData LoadTargetMaterialTemplateFile(const std::string& directoryPath, const std::string& filename, const std::string& target) {

	// 1. 読み込もうとしているフルパスを構築
	std::string fullPath = directoryPath + "/" + filename;

	// 2. コンソール（黒い画面）に出力
	std::cout << "[Loading MTL] " << fullPath << std::endl;

	// 3. Visual Studio の「出力」ウィンドウにも出す（これがあると超便利です）
	std::string debugMsg = "--- Loading MTL: " + fullPath + " ---\n";
	OutputDebugStringA(debugMsg.c_str());

	// --- ここでファイルを開く ---
	std::ifstream file(fullPath);

	if (!file.is_open()) {
		// 4. 失敗した場合、より詳細なエラーメッセージを出す
		std::string errorMsg = "!!! FAILED TO OPEN FILE !!!\nPath: " + fullPath + "\n";
		OutputDebugStringA(errorMsg.c_str());

		// ここで止まるので、上のログが最後に表示されたものが「犯人」です
		assert(false && "Check the Output window for the failed file path.");
	}

	///1. 中で必要となる変数の宣言
	MaterialData materialData;
	std::string line;
	/*std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());*/
	bool isNewmtlRight = false;
	///2. ファイルを開く
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;
		if (identifier == "newmtl") {
			std::string name;
			s >> name;
			if (name == target) { isNewmtlRight = true;}
		}
		if (!isNewmtlRight)continue;

		// identifierに応じた処理
		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			// 連結してファイルパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
			return materialData;
		}
	}
	///3. 実際にファイルを読む、MaterialDataを構築していく
	///4. MaterialDataを返す
	return materialData;
}


ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename) {

	///1. 中で必要となる変数の宣言
	ModelData modelData;														// 構築するModelData
	std::vector<Vector4> positions;												//　位置
	std::vector<Vector3> normals;												//　法線
	std::vector<Vector2> texcoords;												//　テクスチャ座標
	std::string line;															//　ファイルから読んだ１行を格納する変数
	///2. ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);							// ファイルを開く
	assert(file.is_open());													// とりあえずあけなかったら止める
	///3. 実際にファイルを読む、ModelDataを構築していく
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;														// 1行目の識別子を取得
		/// identifierによって処理を分岐
		if (identifier == "v") {
			Vector4 postion;
			s >> postion.x >> postion.y >> postion.z;
			//postion.x *= -1.0f;
			//postion.y *= -1.0f;
			postion.z *= -1.0f;
			postion.w = 1.0f;													// 位置は4次元ベクトルなのでw成分を1に設定
			positions.push_back(postion);
		} else if (identifier == "vt") {
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			//texcoord.x *= -1.0f;
			texcoord.y *= -1.0f;
			texcoords.push_back(texcoord);
		} else if (identifier == "vn") {
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			//normal.x *= -1.0f;
			//normal.y *= -1.0f;
			normal.z *= -1.0f;
			normals.push_back(normal);
		} else if (identifier == "f") {	/// 面は三角形限定。その他は未対応
			VertexData triangle[3];												// 左右手座標形転換用(->左)
			for (int faceVertex = 0; faceVertex < 3; faceVertex++) {
				std::string vertexDeinition;
				s >> vertexDeinition;
				/// 頂点の要素へのindexは[位置/UV/法線]で格納されているので、分解してIndexを取得する
				std::istringstream v(vertexDeinition);
				int elementIndices[3];
				for (int element = 0; element < 3; element++) {
					std::string index;
					std::getline(v, index, '/');								// '/'で分割して要素のインデックスを取得
					if (index.empty()) {
						if (element != 0)elementIndices[element] = 0;
						continue;
					}
					elementIndices[element] = std::stoi(index);					// OBJのインデックスは1から始まるので0に変換
				}
				/// 要素へのIndexから、実際の要素の値を取得して、頂点を構築する
				Vector4 position = positions[elementIndices[0] - 1];

				Vector3 normal;
				if (!(elementIndices[2])) {
					normal = { 0,1,0 };
					//if ((normals[elementIndices[2] - 1]) == Vector3 {0, 0, 0}) {normal = { 0,1,0 };
				} else { normal = normals[elementIndices[2] - 1]; }

				Vector2 texcoord;
				if (!elementIndices[1]) { texcoord = Vector2{ normal.x, normal.y } *0.5f + Vector2{ 0.5f,0.5f }; }
				//if (!(elementIndices[1])) { texcoord = { 0,0 }; }
				else { texcoord = texcoords[elementIndices[1] - 1]; }

				VertexData vertex = { position, texcoord, normal };
				modelData.vertices.push_back(vertex);							// ModelDataに頂点を追加
				triangle[faceVertex] = { position, texcoord, normal };
			}
			for (int i = 0; i < 3; i++) {
				modelData.vertices.push_back(triangle[(3 - 1) - i]);
			}
		} else if (identifier == "mtllib") {
			// materialTemplateLibraryファイルの名前を取得する
			std::string materialFilename;
			s >> materialFilename;
			// 基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}

	}
	///4. ModelDataを返す
	return modelData;														// 構築したModelDataを返す
}


void Model::SetModelData(ModelData modelData) {
	modelData_ = modelData;
}

ID3D12Resource* Model::CreateVertexResource_(ID3D12Device* device) {

	ModelData modelData = LoadObjFile(directoryPath_, objName_);
	vertexResource_->CreateResourceClass_(device, sizeof(VertexData) * modelData.vertices.size());
	CreateVertexBufferView_(int(modelData.vertices.size()));
	VertexNum_ = int(modelData.vertices.size());

	// ついてにModelのテキスチャーもセーブ
	texturePath_ = modelData.material.textureFilePath;

	// 頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	vertexResource_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
	vertexResource_->GetResource()->Unmap(0, nullptr);

	return vertexResource_->GetResource().Get();
}

ID3D12Resource* Model::CreateVertexResourceG_(ID3D12Device* device) {

	vertexResource_->CreateResourceClass_(device, sizeof(VertexData) * modelData_.vertices.size());
	CreateVertexBufferView_(int(modelData_.vertices.size()));
	texturePath_ = modelData_.material.textureFilePath;
	VertexNum_ = int(modelData_.vertices.size());

	// 頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	vertexResource_->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData_.vertices.data(), sizeof(VertexData) * modelData_.vertices.size());
	vertexResource_->GetResource()->Unmap(0, nullptr);

	return vertexResource_->GetResource().Get();
}

void Model::SetModelObj(std::string Path) {
	std::filesystem::path path(Path);
	directoryPath_ = path.parent_path().string();
	objName_ = path.filename().string();
}

void Model::SetVertexNum(int num) {
	VertexNum_ = num;
}



