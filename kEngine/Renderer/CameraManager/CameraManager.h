#pragma once
#include "Camera/DebugCamera.h"

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


//	TODO: CameraManagerをアップグレードすること
// 
// 
//	【CameraManager 目標】
//
//	功能項目：管理 Camera 的生命週期
//	目前版本：沒有
//	問題說明：Camera 是由 Scene 刪除，不是 Manager；ownership 混亂
//	
//	------------------------------------------------------------
//	
//	功能項目：管理 activeCamera
//	目前版本：有
//	問題說明：activeCamera 可能指向已被刪除的 Camera（因為 Scene delete）
//	
//	------------------------------------------------------------
//	
//	功能項目：更新 Camera
//	目前版本：沒有
//	問題說明：Camera 的更新由 Scene 自己做，Manager 沒有參與
//	
//	------------------------------------------------------------
//	
//	功能項目：管理 Camera 清單
//	目前版本：沒有
//	問題說明：Manager 不知道有哪些 Camera，也無法統一管理
//	
//	------------------------------------------------------------
//	
//	功能項目：Camera 查找功能
//	目前版本：沒有
//	問題說明：無法依名稱或 ID 查找 Camera
//	
//	------------------------------------------------------------
//	
//	功能項目：defaultCamera 管理
//	目前版本：有
//	問題說明：功能弱，只是 fallback，不參與 Camera 管理