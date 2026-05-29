#include "ModelGroup.h"
#include "filesystem"

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

std::string ModelGroup::GetFilePath() {

	if (modelData_.lock() == nullptr) { 
		/// 元々はなければログ出るだけと、エンジンモデルの場所パースがないのてここはないのてやらないようにした
		//Logger::Log("kEngine:: ModelGroup :: GetDirectoryPath :: ModelData is expired.");
		return std::string(); 
	}
	return modelData_.lock()->filePath;

}

void ModelGroup::SetModelGroupData(std::shared_ptr<ModelData> modelData) {
	modelData_ = modelData;
}

void ModelGroup::SwapModel(int modelIndex, std::shared_ptr<Model> newModel, int newMeshHandle) {
	if (modelIndex >= 0 && modelIndex < static_cast<int>(modelGroup_.size())) {
		modelGroup_[modelIndex] = newModel;
		meshHandle_[modelIndex] = newMeshHandle;
	} else {
		Logger::Log("[kEngine]ModelGroup:SwapModel() Model index %d is out of range.", modelIndex);
	}
}

bool ModelGroup::HasSkinClusterData() {

	if (!modelData_.lock()) {
		Logger::Log("kEngine:: ModelGroup :: HasSkinClusterData :: ModelData is expired.");
		return false;
	}

	if (modelData_.lock()->skinListList.empty())return false;

	return true;
}
