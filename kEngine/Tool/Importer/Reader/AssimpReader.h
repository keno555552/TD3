#pragma once
#include <string>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct ModelData;
struct NodeData;
struct Skeleton;
struct Joint;

ModelData ReadAssimp(const std::string& filePath);

uint32_t ReadNode(aiNode* node, ModelData& model,uint32_t parentIndex = -1);

void ComputeGlobalMatrix(ModelData& model, uint32_t nodeIndex);


Skeleton ReadSkeleton(const ModelData& model);

int32_t CreateJoint(
	uint32_t nodeIndex, 
	const std::optional<int32_t>& parent,
	const std::vector<NodeData>& nodeList,
	std::vector<Joint>& jointList);