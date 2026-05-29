#include "filesystem"
#include "GLTFReader.h"
#include "AssimpReader.h"
#include "Data/Render/CPUData/ModelData.h"

ModelData ReadGltf(const std::string& filePath) {
	ModelData result = ReadAssimp(filePath);;
	for (auto& model : result.meshDataList) {
		for(auto& vertex : model.vertices) {
			vertex.position.x *= -1.0f;
			vertex.normal.x *= -1.0f;
		}
	}
	return result;
}
