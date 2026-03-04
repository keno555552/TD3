#include "ResourceLoader.h"

/// ================ ResourceLoader system関連の定義 =================///
kEngine* ResourceLoader::system_ = nullptr;
ResourceLoader* ResourceLoader::instance_ = nullptr;
int ResourceLoader::nowStage_ = 0;

/// ================ systemの予備テキスチャの定義 =================///



/// ================ ステージ1のテキスチャの定義 =================///
int ResourceLoader::textureHandle_ = -1;


ResourceLoader* ResourceLoader::GetInstance() {
	if (instance_ == nullptr) {
		instance_ = new ResourceLoader();
	}
	return instance_;
}

void ResourceLoader::Initialize(kEngine* system) {
	system_ = system;
}

void ResourceLoader::Finalize() {
	delete instance_;
}

void ResourceLoader::LoadTextureByStage(int stage) {
	nowStage_ = stage;
	LoadStageTexture[nowStage_];
}


void ResourceLoader::LoadStageTestResource() {
	/// 例: loadingTexture_ = TextureManager::GetInstance()->LoadCommonTexture("Resources/Texture/Stage/Stage1/Stage1_Floor.png");
}

void ResourceLoader::LoadStage1Resource() {
}

void (ResourceLoader::*ResourceLoader::LoadStageTexture[])() = {
	&ResourceLoader::LoadStageTestResource,
	&ResourceLoader::LoadStage1Resource
};

void ResourceLoader::LoadTextureOnce() {
	for (auto& func : LoadStageTexture) {
		(this->*func)();
	}
}
