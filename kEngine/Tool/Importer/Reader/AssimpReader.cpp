#include "AssimpReader.h"
#include "filesystem"
#include "Data/Geometry/ModelStructure/NodeData.h"
#include "Data/Render/CpuData/ModelData.h"

ModelData ReadAssimp(const std::string& filePath) {

	std::string directoryPath = std::filesystem::path(filePath).parent_path().string();
	std::string filename = std::filesystem::path(filePath).filename().string();

	/// ===============================  中で必要となる変数の宣言 ================================ ///
	Assimp::Importer importer;
	//std::string filePath = filePath;
	const aiScene* scene = importer.ReadFile(
		filePath,
		aiProcess_FlipWindingOrder |		// 頂点の順番を反転
		aiProcess_FlipUVs 					// UV反転
	);
	assert(scene->HasMeshes());

	/// 読み込んだシーンからModelDataを構築する
	ModelData result{};

	/// 読み込んだファイルのディレクトリパスを保存
	result.filePath = filePath;

	/// ================  1. まずはMeshを読み込む、MeshごとにModelDataを構築していく ================ ///

	/// --- マテリアル関連 ---
	if (scene->mNumMaterials > 0) {
		result.materialList.reserve(scene->mNumMaterials);

		for (uint32_t i = 0; i < scene->mNumMaterials; i++) {
			aiMaterial* material = scene->mMaterials[i];

			MaterialData materialData{};

			aiString name;
			if (material->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
				materialData.mtlName_ = name.C_Str();
			}

			if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
				aiString textureFilePath;
				material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
				materialData.textureFilePath = directoryPath + "/" + textureFilePath.C_Str();

			}

			result.materialList.push_back(materialData);
		}
	} else {
		MaterialData defaultMat;
		defaultMat.mtlName_ = "DefaultMaterial";
		result.materialList.push_back(defaultMat);
	}

	/// --- メッシュ関連 ---
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
		aiMesh* mesh = scene->mMeshes[meshIndex];

		assert(mesh->HasNormals()); // 法線情報がないMeshは今回は非対応
		assert(mesh->HasTextureCoords(0)); // テクスチャ座標がないMeshは今回は非対応

		MeshData meshData{};

		/// Meshの名前を保存
		meshData.name = mesh->mName.C_Str();

		/// Meshの頂点データを保存
		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {
			aiFace face = mesh->mFaces[faceIndex];
			//assert(face.mNumIndices == 3); // 三角形以外は非対応
			// 面の頂点インデックスを逆順に格納して、左右手座標系変換を行う
			for (uint32_t element = 0; element < face.mNumIndices; element++) {
				uint32_t vertexIndex = face.mIndices[element];
				//uint32_t vertexIndex = face.mIndices[element];
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

				// push index
				meshData.indices.push_back((uint32_t)meshData.vertices.size());
				//meshData.indices.push_back((uint32_t)vertexIndex);

				// push vertex
				meshData.vertices.push_back(vertex);
			}
		}

		/// マテリアルのインデックスを格納
		meshData.materialIndex = min(
			mesh->mMaterialIndex,
			(uint32_t)result.materialList.size() - 1
		);

		meshData.nodeIndex = meshIndex; // meshとnodeは1対1対応していると仮定する

		/// MeshDataをModelDataに追加、次
		result.meshDataList.push_back(meshData);
	}

	/// --- ノード関連 ---
	uint32_t rootIndex = ReadNode(scene->mRootNode, result, (uint32_t)-1);
	result.rootNodeList.push_back(rootIndex);
	ComputeGlobalMatrix(result, rootIndex);

	///=========================== 2. アニメーション  =========================== ///

	if (scene->mNumAnimations > 0) {
		/// 受け皿を作る
		Animation animation;
		/// 最初のアニメーションを読み込む
		aiAnimation* animationAssimp = scene->mAnimations[0];

		/// アニメーションの長さを秒数で計算して保存
		animation.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond);

		/// assimpで一つずつのNodeのAnimationをchannelと呼んでいるのてchannelを回してNodeAnimationの情報をお取ってくる
		for (int channelIndex = 0; channelIndex < (int)animationAssimp->mNumChannels; channelIndex++) {
			AnimationNodeData nodeData{};
			aiNodeAnim* nodeAnim = animationAssimp->mChannels[channelIndex];
			nodeData.name = nodeAnim->mNodeName.C_Str();
			nodeData.animationNodeID = channelIndex;
			/// スケールのKeyFrameを保存
			for (int keyIndex = 0; keyIndex < (int)nodeAnim->mNumScalingKeys; keyIndex++) {
				KeyFrameVector3 keyFrame;
				keyFrame.time_ = float(nodeAnim->mScalingKeys[keyIndex].mTime / animationAssimp->mTicksPerSecond);
				keyFrame.value_ = { nodeAnim->mScalingKeys[keyIndex].mValue.x,
									nodeAnim->mScalingKeys[keyIndex].mValue.y,
									nodeAnim->mScalingKeys[keyIndex].mValue.z, };
				keyFrame.animationType_ = AnimationType::LINEARITY;
				nodeData.scaleList.push_back(keyFrame);
			}
			/// 回転のKeyFrameを保存
			for (int keyIndex = 0; keyIndex < (int)nodeAnim->mNumRotationKeys; keyIndex++) {
				KeyFrameQuaternion keyFrame;
				keyFrame.time_ = float(nodeAnim->mRotationKeys[keyIndex].mTime / animationAssimp->mTicksPerSecond);
				Quaternion quatValue = { nodeAnim->mRotationKeys[keyIndex].mValue.x,
										 nodeAnim->mRotationKeys[keyIndex].mValue.y,
										 nodeAnim->mRotationKeys[keyIndex].mValue.z,
										 nodeAnim->mRotationKeys[keyIndex].mValue.w, };
				keyFrame.value_ = quatValue;
				keyFrame.animationType_ = AnimationType::LINEARITY;
				nodeData.rotateList.push_back(keyFrame);
			};
			/// 平行移動のKeyFrameを保存
			for (int keyIndex = 0; keyIndex < (int)nodeAnim->mNumPositionKeys; keyIndex++) {
				KeyFrameVector3 keyFrame;
				keyFrame.time_ = float(nodeAnim->mPositionKeys[keyIndex].mTime / animationAssimp->mTicksPerSecond);
				keyFrame.value_ = { nodeAnim->mPositionKeys[keyIndex].mValue.x,
									nodeAnim->mPositionKeys[keyIndex].mValue.y,
									nodeAnim->mPositionKeys[keyIndex].mValue.z, };
				keyFrame.animationType_ = AnimationType::LINEARITY;
				nodeData.translationList.push_back(keyFrame);
			}
			animation.nodeList.push_back(nodeData);
		}
		/// AnimationをModelDataに追加
		result.animationList.push_back(animation);
		/// アニメーションがあるとフラグを立てる
		result.haveAnimation = true;
	}

	/// =========================== 3. skeleton  =========================== ///

	result.skeleton = ReadSkeleton(result);

	/// =========================== 4. skining  =========================== ///

	result.skinListList.resize(scene->mNumMeshes);

	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {

		aiMesh* mesh = scene->mMeshes[meshIndex];
		SkinData& skin = result.skinListList[meshIndex];

		/// もしskinningがないMeshなら、次のMeshに行く
		if (mesh->mNumBones == 0) {
			continue;
		}

		/// Jointの数を取ってくる
		int jointCount = (int)result.skeleton.jointList.size();

		/// 初始化SkinDataの数
		skin.allVertexNum = mesh->mNumVertices;
		skin.jointIndices.resize(jointCount, -1);
		skin.jointNames.resize(jointCount);
		skin.skinClusterData.resize(jointCount);
		skin.inverseBindPoseMatrices.resize(jointCount);

		for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {

			/// boneからjointのindexを取ってきて、jointWeightDataにウェイト情報を保存していく
			aiBone* bone = mesh->mBones[boneIndex];
			std::string jointName = bone->mName.C_Str();

			/// joint名からjointIndexを取ってくる
			int jointIndex = result.skeleton.jointMap[jointName];
			skin.jointNames[boneIndex] = jointName;

			/// まずはbindPoseMatrixをAssimpから取ってきて、行列の転置と左右手座標系変換を行う
			aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse();
			aiVector3D scale, translate;
			aiQuaternion rotate;
			bindPoseMatrixAssimp.Decompose(scale, rotate, translate);

			/// Assimpは右手系で、kEngineは左手系なので、左右手座標系変換を行う
			Matrix4x4 bindPoseMatrix = MakeAffineMatrix(
				{ scale.x, scale.y, scale.z },
				{ rotate.x, -rotate.y, -rotate.z, rotate.w },
				{ -translate.x, translate.y, translate.z }
			);

			/// skinDataの資料を保存していく
			// Indices
			skin.jointIndices[jointIndex] = jointIndex;
			// inverseBindPoseMatrix
			skin.inverseBindPoseMatrices[jointIndex] = Inverse(bindPoseMatrix);

			// まずはvertexWeightsの位置をとる
			JointWeightData& jointWeightData = skin.skinClusterData[jointIndex];
			// データを保存していく
			for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
				jointWeightData.vertexWeights.push_back({
					bone->mWeights[weightIndex].mWeight,
					bone->mWeights[weightIndex].mVertexId
					});
			}
		}

		/// skinningの資料があるMeshが一つでもあれば、ModelData全体のフラグを立てる
		if (!result.haveSkinning)result.haveSkinning = true;
	}

	return result;
}

uint32_t ReadNode(aiNode* node, ModelData& model, uint32_t parentIndex) {

	NodeData result{};

	/// 1.nodeの名前を取得
	result.name = node->mName.C_Str();

	/// 2.nodeのlocalMatrixを取得、行列の転置も行う
	aiMatrix4x4 aiLocalMatrix = node->mTransformation; // nodeのlocalMatrixを取得
	aiLocalMatrix.Transpose(); // 行列を転置

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result.localMatrix.m[i][j] = aiLocalMatrix[i][j];
		}
	}

	/// 3.nodeのlocalMatrixからスケール、回転、平行移動を分解して保存

	aiVector3D scaling;
	aiQuaternion rotation;
	aiVector3D position;

	node->mTransformation.Decompose(scaling, rotation, position);

	result.scale = { scaling.x, scaling.y, scaling.z };
	result.rotation = { rotation.x, rotation.y, rotation.z, rotation.w };
	result.translation = { position.x, position.y, position.z };

	/// 4.nodeが綁定しているMeshのindexを格納
	if (node->mNumMeshes > 0) {
		result.meshIndex = node->mMeshes[0];  // glTF 通常只有一個

	}

	/// 5.子ノードの数だけchildrenを確保、再帰的に子ノードを読み込む
	// node push 進 nodeList
	uint32_t thisIndex = (uint32_t)model.nodeList.size();
	result.parentIndex = parentIndex;
	model.nodeList.push_back(result);

	/// mesh を node に関連付け
	int meshIndex = (int)result.meshIndex;
	if (meshIndex >= 0) {
		model.meshDataList[meshIndex].nodeIndex = thisIndex;
	}

	for (uint32_t i = 0; i < node->mNumChildren; i++) {
		uint32_t childIndex = ReadNode(node->mChildren[i], model, thisIndex);
		model.nodeList[thisIndex].children.push_back(childIndex);
	}

	return thisIndex;
}

void ComputeGlobalMatrix(ModelData& model, uint32_t nodeIndex) {
	NodeData& node = model.nodeList[nodeIndex];

	if (node.parentIndex == -1)
		node.globalMatrix = node.localMatrix;
	else
		node.globalMatrix = model.nodeList[node.parentIndex].globalMatrix * node.localMatrix;

	for (uint32_t child : node.children)
		ComputeGlobalMatrix(model, child);
}

Skeleton ReadSkeleton(const ModelData& model) {

	Skeleton skeleton{};

	/// RootTreeを再帰的にたどってJointを作成していく
	skeleton.rootID = CreateJoint(
		model.rootNodeList[0],
		std::nullopt,
		model.nodeList,
		skeleton.jointList
	);

	/// Joint名とIndexとの辞書を作る
	for (const Joint& joint : skeleton.jointList) {
		skeleton.jointMap[joint.name] = joint.ID;
	}

	return skeleton;
}


int32_t CreateJoint(
	uint32_t nodeIndex,
	const std::optional<int32_t>& parent,
	const std::vector<NodeData>& nodeList,
	std::vector<Joint>& jointList) {
	const NodeData& nodeData = nodeList[nodeIndex];

	Joint joint{};
	joint.ID = int32_t(jointList.size());
	joint.name = nodeData.name;
	joint.localMatrix = nodeData.localMatrix;
	joint.skeletonSpaceMatrix = Identity();
	joint.scale = nodeData.scale;
	joint.rotate = nodeData.rotation;
	joint.translation = nodeData.translation;
	joint.parentID = parent;
	jointList.push_back(joint);
	for (uint32_t childIndex : nodeData.children) {
		int32_t childID = CreateJoint(childIndex, joint.ID, nodeList, jointList);
		jointList[joint.ID].childrenIDList.push_back(childID);
	}

	return joint.ID;
}
