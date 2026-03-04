#pragma once
#include "kEngine.h"
#include "MathsIncluder.h"
#include "SceneManager.h"

class ResourceLoader
{
public:
	static ResourceLoader* GetInstance();

	void Initialize(kEngine* system);

	void Finalize();

	void LoadTextureByStage(int stage);

	void LoadTextureOnce();




public:
	static int textureHandle_;

private:
	static int loadingTexture_;

private:
	void LoadStageTestResource();
	void LoadStage1Resource();

	static void (ResourceLoader::*LoadStageTexture[])();




private:
	static ResourceLoader* instance_;
	static kEngine* system_;

	static int nowStage_;

};

