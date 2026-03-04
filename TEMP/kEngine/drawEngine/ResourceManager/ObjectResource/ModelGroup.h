#pragma once
#include "Model.h"
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

std::vector<ModelData> LoadMuitObjFile(const std::string& directoryPath, const std::string& filename);
std::vector<ModelData> LoadAssimpFile(const std::string& directoryPath, const std::string& filename);

NodeData ReadNode(aiNode* node);

class ModelGroup
{
public:
	//~ModelGroup();

	Model* GetModel(int handle){return modelGroup_[handle];}
	void PushModel(Model* model);
	void SetModelObj(std::string Path);
	std::string GetDirectoryPath() { return directoryPath_; }
	std::string GetObjName_() { return objName_; }
	std::string GetFullPath_() { return (directoryPath_ + "/" + objName_); }
	void PushModelHandle(int handle) { modelHandle_.push_back(handle); }
	int GetModelHandle(int handle) { return modelHandle_[handle]; }
	int GetModelNum() { return (int)modelGroup_.size();}


private:
	std::string directoryPath_ = "resources/object/plane";
	std::string objName_ = "plane.obj";
	std::vector<Model*> modelGroup_;
	std::vector<int> modelHandle_;
};

