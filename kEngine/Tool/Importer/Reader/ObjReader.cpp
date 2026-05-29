#include "ObjReader.h"
#include "filesystem"
#include <algorithm>
#include "AssimpReader.h"
#include "Data/Render/CPUData/ModelData.h"

ModelData ReadObj(const std::string& filePath) {
	ModelData result = ReadAssimp(filePath);;
	for (auto& model : result.meshDataList) {
		for (auto& vertex : model.vertices) {
			vertex.position.z *= -1.0f;
			vertex.normal.z *= -1.0f;
			
		}
	}
	return result;
}
