#include "kEngine.h"			//kEngine
#include "Transform.h"			//Transform.h
#include <cstdint>
#include <windows.h>
#include "GPUData/DirectionalLightGPU.h"
#include <string>
#include <vector>
#include "SceneManager.h"

const char kWindowTitle[] = "進化しろ";

const int32_t kWindowWidth = 1280;
const int32_t kWindowHeight = 720;

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	CoInitializeEx(0, COINIT_MULTITHREADED);
	std::unique_ptr<kEngine> system;
	system = std::make_unique<kEngine>();
	system->Initialize(kWindowTitle, kWindowWidth, kWindowHeight);

	SceneManager::GetInstance().Initialize(system.get());

	///========================Main処理=====================///
	while (system->ProcessMessage() && kEngine::GameOn()) {

		///====================ゲーム処理====================///
		SceneManager::GetInstance().Update();

		///=====================描画処理=====================///
		system->StartFrame();

		SceneManager::GetInstance().Render();

		system->EndFrame();

	}
	SceneManager::GetInstance().Finalize();
	system->Finalize();
	return 0;
}
