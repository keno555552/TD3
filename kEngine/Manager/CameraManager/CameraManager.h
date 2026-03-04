#pragma once
#include "./kEngine/tool/Camera/DebugCamera.h"

class CameraManager {
public:

	void Initialize();
	void Finalize();

	/// カメラの生成
    Camera* CreateCamera();

	/// デバッグカメラの生成
    DebugCamera* CreateDebugCamera(kEngine* kEngine);

	/// カメラの破棄
    void DestroyCamera(Camera* cam);

	/// アクティブカメラの設定
    void SetActiveCamera(Camera* cam = nullptr);

	/// リセットアクティブカメラ
	void ResetActiveCamera();

	/// アクティブカメラの取得
    Camera* GetActiveCamera() const;

private:
    Camera* defaultCamera_ = nullptr;
    Camera* activeCamera_ = nullptr;
};

