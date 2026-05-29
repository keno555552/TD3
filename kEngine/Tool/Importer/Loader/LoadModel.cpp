#include "LoadModel.h"
#include "Logger.h"
#include "../Reader/GLTFReader.h"
#include "../Reader/ObjReader.h"
#include "filesystem"

ModelData LoadModelBranch(const std::string& filepath) {

	std::string ext = std::filesystem::path(filepath).extension().string();

	if (ext == ".obj") { 

		return ReadObj(filepath);

	} else if (ext == ".fbx") {

		Logger::Log("kEngine : LoadModel : FBX file is not supported yet.");

	} else if (ext == ".gltf" || ext == ".glb") {
		
		return ReadGltf(filepath);

	}
	return ModelData();
}