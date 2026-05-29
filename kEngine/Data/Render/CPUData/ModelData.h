#pragma once
#include "Data/Geometry/ModelStructure/NodeData.h"
#include "Data/Render/CPUData/MaterialData.h"
#include "Data/Render/CPUData/MeshData.h"
#include "Data/Animation/Animation.h"
#include "Data/Geometry/ModelStructure/SkinData.h"
#include "Data/Animation/Skeleton.h"

struct ModelData
{
	/// 読み込んだモデルデータ、
	std::vector<MaterialData> materialList;
	std::vector<MeshData> meshDataList;		// MaterialDataがIndex参照
	std::vector<uint32_t> rootNodeList;		// root node の index
	std::vector<NodeData> nodeList;			// MeshDataがIndex参照

	/// アニメーションデータ
	std::vector<Animation> animationList;
	std::vector<SkinData> skinListList;
	Skeleton skeleton;

	/// 読み込みのpathを保存する
	std::string filePath = "resources/object/plane/plane.obj";

	/// 資料の内容を確認するためのフラグ
	bool haveSkinning = false;
	bool haveAnimation = false;
};

//struct ModelData
//{
//	std::vector<VertexData> vertices;
//	MaterialData material;
//	NodeData rootNode;
//};