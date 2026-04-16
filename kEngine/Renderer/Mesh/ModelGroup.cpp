#include "ModelGroup.h"
#include "filesystem"

std::vector<ModelData> LoadFileTop(const std::string& filepath) {

	std::string ext = std::filesystem::path(filepath).extension().string();

	if (ext == ".obj") {
		return LoadMuitObjFile(
			std::filesystem::path(filepath).parent_path().string(),
			std::filesystem::path(filepath).filename().string()
		);
	} else if (ext == ".fbx") {



	} else if (ext == ".gltf" || ext == ".glb") {
		return LoadAssimpFile(
			std::filesystem::path(filepath).parent_path().string(),
			std::filesystem::path(filepath).filename().string()
		);
	}
	return std::vector<ModelData>();
}


NodeData ReadNode(aiNode* node) {

	NodeData result{};
	aiMatrix4x4 aiLocalMatrix = node->mTransformation; // nodeのlocalMatrixを取得
	aiLocalMatrix.Transpose(); // 行列を転置

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result.localMatrix.m[i][j] = aiLocalMatrix[i][j];
		}
	}

	result.name = node->mName.C_Str(); // nodeの名前を取得
	result.children.resize(node->mNumChildren); // 子ノードの数だけchildrenを確保

	for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
		result.children[childIndex] = ReadNode(node->mChildren[childIndex]); // 再帰的に子ノードを読み込む
	}

	return result;
}

std::vector<ModelData> LoadMuitObjFile(const std::string& directoryPath, const std::string& filename) {

	///1. 中で必要となる変数の宣言
	std::vector<ModelData>modelGroup;

	ModelData modelData;														// 構築するModelData
	std::vector<Vector4> positions;												//　位置
	std::vector<Vector3> normals;												//　法線
	std::vector<Vector2> texcoords;												//　テクスチャ座標
	std::string line;															//　ファイルから読んだ１行を格納する変数
	std::string materialFilename;

	bool startCopy = false;

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
			VertexData triangle[3]{};												// 左右手座標形転換用(->左)
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
				if (!elementIndices[1]) { texcoord = Vector2{ normal.x, normal.y } * 0.5f + Vector2{ 0.5f,0.5f }; }
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
			//// materialTemplateLibraryファイルの名前を取得する
			//std::string materialFilename;
			s >> materialFilename;
			//// 基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
			//modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);

		} else if (identifier == "usemtl") {
			std::string materialName;
			s >> materialName;
			modelData.material.mtlName_ = materialName;

		} else if (identifier == "o") {
			//if (!startCopy) {
			//	startCopy = true; continue;
			//} else {
			//	startCopy = false;
			//}
			if (!modelData.vertices.empty()) {
				modelData.material = LoadTargetMaterialTemplateFile(directoryPath, materialFilename, modelData.material.mtlName_);
				modelGroup.push_back(modelData);
			}
			modelData = {};
			//positions.clear();
			//normals.clear();
			//texcoords.clear();
		}
	}
	if (!modelData.vertices.empty()) {
		modelData.material = LoadTargetMaterialTemplateFile(directoryPath, materialFilename, modelData.material.mtlName_);
		modelGroup.push_back(modelData);
	}

	///4. ModelDataを返す
	return modelGroup;														// 構築したModelDataを返す
}

std::vector<ModelData> LoadAssimpFile(const std::string& directoryPath, const std::string& filename) {

	std::string debugMessage = "Loading model: " + directoryPath + "\n";
	OutputDebugStringA(debugMessage.c_str());

	///1. 中で必要となる変数の宣言
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;
	const aiScene* scene = importer.ReadFile(filePath,
		aiProcess_FlipWindingOrder |		// 頂点の順番を反転
		aiProcess_FlipUVs 					// UV反転
	);

	//if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
	//	// ここでエラーログを出して関数を抜けるべき
	//	return;
	//}

	assert(scene->HasMeshes());

	///2. 実際にファイルを読む、ModelDataを構築していく
	std::vector<ModelData>modelGroup;
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		ModelData modelData{};
		assert(mesh->HasNormals()); // 法線情報がないMeshは今回は非対応
		assert(mesh->HasTextureCoords(0)); // テクスチャ座標がないMeshは今回は非対応

		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {
			aiFace face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3); // 三角形以外は非対応
			// 面の頂点インデックスを逆順に格納して、左右手座標系変換を行う
			for (uint32_t element = 0; element < face.mNumIndices; element++) {
				uint32_t vertexIndex = face.mIndices[(face.mNumIndices - 1) - element];
				aiVector3D aiPosition = mesh->mVertices[vertexIndex];
				aiVector3D aiNormal = mesh->mNormals[vertexIndex];
				aiVector3D aiTexCoord = mesh->mTextureCoords[0][vertexIndex];
				VertexData vertex{};
				// 位置
				vertex.position = { aiPosition.x, aiPosition.y, aiPosition.z, 1.0f };
				// 法線
				vertex.normal = { aiNormal.x, aiNormal.y, aiNormal.z };
				// テクスチャ座標
				vertex.texcoord = { aiTexCoord.x, aiTexCoord.y };
				// aiProcess_MakeLeftHandedはz*=-1をしてくれるので、右手系から左手系への変換は頂点の順番を反転するだけで良い
				//vertex.position.z *= -1.0f;
				//vertex.normal.z *= -1.0f;
				vertex.texcoord.x *= -1.0f;
				modelData.vertices.push_back(vertex);
			}
		}
		// --- マテリアル関連 ---
		uint32_t materialIndex = mesh->mMaterialIndex;
		aiMaterial* material = scene->mMaterials[materialIndex];

		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			aiString textureFilePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
			modelData.material.textureFilePath = directoryPath + "/" + textureFilePath.C_Str();
		}

		modelData.rootNode = ReadNode(scene->mRootNode);

		modelGroup.push_back(modelData);

	}
	return modelGroup;														// 構築したModelDataを返す
}

ModelGroup::~ModelGroup() {
	if (!modelGroup_.empty()) 		{
		for (auto model : modelGroup_) {
			if(model){
				model->ClearAllResource();
			}
		}
	}
	modelGroup_.clear();
}

void ModelGroup::PushModel(std::shared_ptr<Model> model) {
	modelGroup_.push_back(model);
}

void ModelGroup::SetModelObj(std::string Path) {
	std::filesystem::path path(Path);
	directoryPath_ = path.parent_path().string();
	objName_ = path.filename().string();
}