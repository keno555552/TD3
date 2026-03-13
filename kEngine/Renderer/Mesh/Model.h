#pragma once
#include <cassert>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include "Data/Render/CPUData/VertexData.h"
#include "Transform.h"
#include "TransformationMatrix.h"
#include "VertexResource.h"
#include "Config.h"

struct MaterialData
{
	std::string textureFilePath;
	std::string mtlName_;
};

struct NodeData
{
	Matrix4x4 localMatrix = Identity();
	std::string name;
	std::vector<NodeData> children;
};

struct ModelData
{
	std::vector<VertexData> vertices;
	MaterialData material;
	NodeData rootNode;
};

ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

std::vector<ModelData> LoadFileTop(const std::string& filepath);
MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
MaterialData LoadTargetMaterialTemplateFile(const std::string& directoryPath, const std::string& filename, const std::string& target);


class Model :public MeshBuffer
{
public:
	ModelData GetModelData() { return modelData_; }
	void SetModelData(ModelData modeldata);
	ID3D12Resource* CreateVertexResource_(ID3D12Device* device)override;
	ID3D12Resource* CreateVertexResourceG_(ID3D12Device* device);
	std::string GetTexturePatch() { return texturePath_; }
	void SetTextureHandle(int textureHandle) {textureHandle_ = textureHandle;}
	UINT GetTextureHandle() { return textureHandle_; }
	UINT GetVertexNum() { return VertexNum_; }
	void SetModelObj(std::string Path);
	void SetVertexNum(int num);

private:
	ModelData modelData_ = {};
	std::string directoryPath_ = "resources/object/plane";
	std::string objName_ = "plane.obj";
	std::string texturePath_;
	std::string mtlName_;
	int textureHandle_ = -1;
	int modelResourceHandle_ = -1;
	int VertexNum_ = 0;
};


