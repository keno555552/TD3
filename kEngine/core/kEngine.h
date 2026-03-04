#pragma once
#include "DirectXController.h"
#include "inputManager.h"
#include "soundManager.h"
#include "SrvManager.h"
#include "drawEngine.h"
#include "TransformationMatrix.h"
#include "Vector2.h"
#include "materialconfig.h"
#include "tool/TimeManager/TimeManager.h"
#include "DrawDataCollector.h"
#include "CameraManager/CameraManager.h"

#ifdef USE_IMGUI
#include "ImGuiManager.h"
#endif // USE_IMGUI
#include <string>
#include "DrawData/ObjectData.h"
#include "DrawData/SpriteData.h"
#include "InstanceManager.h"
#include "ResourceManager/ResourceManager.h"
#include "TextureManager/TextureManager.h"
#include "LightManager/LightManager.h"
#include "Data/DirectionalLightGPU.h"
#include "Camera.h"


class kEngine
{
public:

#pragma region システム管理


	/// System関連

	void Initialize(const char* kClientTitle, int kClientWidth, int kClientHeight);

	void Finalize();

	void StartFrame();

	void EndFrame();

	bool ProcessMessage();

	static bool GameOn() { return isGameOn_; }

	static void EndGame() { isGameOn_ = false; }

#pragma endregion

#pragma region 描画システム

	/// VVVV====== SpriteData/ObjectData描画,今上使えない ======VVVV///
	void Draw2D(SpriteData* spriteData);
	void Draw3D(ObjectData* objectData);

	int GetModelTextureHandle(int modelHandle, int part);

	int GetMutiModelNum(int modelHandle);
	int SetModelObj(std::string path);

	void AddLight(Light* light);
	void RemoveLight(Light* light);


	DebugCamera* CreateDebugCamera();
	Camera* CreateCamera();
	void DestroyCamera(Camera* camera);
	void SetCamera(Camera* camera);
	void ResetToDefaultCamera();

	int commonTextureHandleReader(int handle);
	int commonModelHandleReader(int handle);

	int LoadTexture(const std::string& filePath);

#pragma endregion

#pragma region 音関連

	/// 音関連

	int SoundLoadSE(const std::string filename);
	void SoundPlaySE(int Handle, float volume = 1.0f);
	void SoundPlayBGM(int Handle, float volume = 1.0f);

	void SoundSetVolume(int Handle, float volume = 1.0f);
	void SoundPause(int Handle);
	void SoundContinue(int Handle);
	void SoundStop(int Handle);

	void SoundSetMasterVolume(float volume);
	void SoundSetSEVolume(float volume);
	void SoundSetBGMVolume(float volume);

	bool SoundIsPlaying(int Handle);
	float SoundGetVolume(int Handle);
	float SoundGetMasterVolume() const;
	float SoundGetBGMVolume() const;
	float SoundGetSEVolume() const;

	void SoundSetMute(int Handle, bool isMute);
	void SoundSetMasterMute(bool isMute);
	void SoundSetBGMMute(bool isMute);
	void SoundSetSEMute(bool isMute);

	bool SoundGetMute(int Handle);
	bool SoundGetMasterMute() const;
	bool SoundGetBGMMute()const;
	bool SoundGetSEMute()const;

#pragma endregion

#pragma region 入力関連

	/// 入力関連

	Vector2 GetMousePosVector2();
	int GetMousePosX();
	int GetMousePosY();
	int GetMousePosXIns();
	int GetMousePosYIns();
	bool GetTriggerOn(int key);
	bool GetTriggerOff(int key);
	bool GetIsPush(int key);

	bool GetMouseTriggerOn(int key);
	bool GetMouseTriggerOff(int key);
	bool GetMouseIsPush(int key);
	int GetMouseScroll();
	int GetMouseScrollOrigin();

	bool GetGamepadTriggerOn(int key);
	bool GetGamepadTriggerOff(int key);
	bool GetGamepadIsPush(int key);
	int GetGamepad4Direction();
	int GetGamepad4DirectionTriggerOn();
	int GetGamepad4DirectionTriggerOff();
	int GetGamepad8Direction();
	int GetGamepad8DirectionTriggerOn();
	int GetGamepad8DirectionTriggerOff();
	int GetGamepadDirectionFloat();
	float GetGamepadLStick01X();
	float GetGamepadLStick01Y();
	int GetGamepadLStickX();
	int GetGamepadLStickY();
	float GetGamepadRStick01X();
	float GetGamepadRStick01Y();
	int GetGamepadRStickX();
	int GetGamepadRStickY();
	float GetGamepadL201();
	float GetGamepadR201();
	int GetGamepadL2();
	int GetGamepadR2();

#pragma endregion

#pragma region タイム関連

	/// Timer
	float GetFPS();
	float GetFPSPerSecond();
	float GetDeltaTime();

	/// 倍率付きの時間関数
	void SetTimeScale(float timeScale);
	float GetTimeScale() const;
	float GetScaledDeltaTime() const;

	/// Timer用の倍率付きの時間関数
	void SetTimerTimeScale(float timerTimeScale);
	float GetTimerTimeScale_() const;
	float GetTimerScaledDeltaTime_() const;


#pragma endregion

#pragma region システムインターフェース

	DirectXController* GetDirectXController();
	SrvManager* GetSrvManager();
	ResourceManager* GetResourceManager() const;
	TextureManager* GetTextureManager() const;
	LightManager* GetLightManager() const;
	CameraManager* GetCameraManager() const;
	DrawDataCollector* GetDrawDataCollector() const;
	DrawEngine* GetDrawEngine() const;
	SoundManager* GetSoundManager() const;
	InputManager* GetInputManager() const;
	TimeManager* GetTimeManager()const;

#pragma endregion

private:
	/// ============ コアシステム ============///
	std::unique_ptr <DirectXController> dxComm = nullptr;


	/// ============ 描画関連 ============///

	/// 資源管理
	std::unique_ptr <SrvManager> srvManager{};
	///ResourceManager* resourceManager{};					/// シングルトン
	///TextureManager* textureManager{};					/// シングルトン

	/// インスタンス管理
	//std::unique_ptr <InstanceManager> instanceManager{};	/// まだ使えない!
	std::unique_ptr <DrawDataCollector> drawDataCollector{};

	/// ライティング管理
	std::unique_ptr <LightManager> lightManager{};

	/// カメラ管理
	std::unique_ptr <CameraManager> cameraManager{};

	/// 描画ロジック
	std::unique_ptr <DrawEngine> drawEngine{};

	/// ============ 入力関連 ============///
	std::unique_ptr <InputManager> inputManager{};

	/// ============ 音関連 ============///
	std::unique_ptr <SoundManager> soundManager{};

	/// ============ 時間関連 ============///
	std::unique_ptr <TimeManager> timeManager{};

	/// =========== ゲーム継続関連 ===========///
	static bool isGameOn_;
};