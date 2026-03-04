#include "kEngine.h"

#pragma region システム管理

bool kEngine::isGameOn_ = true;

void kEngine::Initialize(const char* kClientTitle, int kClientWidth, int kClientHeight) {
	config::SaveClientTitle(kClientTitle);
	config::SaveClientWidth(kClientWidth);
	config::SaveClientHeight(kClientHeight);

	dxComm = std::make_unique<DirectXController>();
	dxComm->InitializeDrive(kClientTitle, kClientWidth, kClientHeight);

	srvManager = std::make_unique<SrvManager>();
	srvManager->Initialize(dxComm.get());

#ifdef USE_IMGUI
	ImGuiManager::Initialize(dxComm.get(), srvManager.get());
#endif

	//instanceManager = std::make_unique < InstanceManager>();
	TextureManager::GetInstance()->Initialize(dxComm.get(), srvManager.get());
	ResourceManager::GetInstance()->Initialize(dxComm.get());

	lightManager = std::make_unique<LightManager>();
	cameraManager = std::make_unique<CameraManager>();

	drawDataCollector = std::make_unique<DrawDataCollector>();
	drawDataCollector->Initialize(cameraManager.get(), lightManager.get());


	drawEngine = std::make_unique<DrawEngine>();
	drawEngine->Initialize(dxComm.get(), srvManager.get(), ResourceManager::GetInstance(), drawDataCollector.get());

	soundManager = std::make_unique<SoundManager>();
	soundManager->Initialize();

	timeManager = std::make_unique<TimeManager>();

	inputManager = std::make_unique<InputManager>();
	inputManager->Initialize(dxComm.get(), timeManager.get());
}

void kEngine::Finalize() {

#ifdef USE_IMGUI
	ImGuiManager::Shutdown();
#endif
	inputManager->Finalize();
	// delete timeManager;
	soundManager->Finalize();
	TextureManager::GetInstance()->Finalize();
	TextureManager::Destroy();
	ResourceManager::GetInstance()->Finalize();
	ResourceManager::Destroy();

	drawEngine->Finalize();
	drawDataCollector->Finalize();
	cameraManager->Finalize();
	lightManager->Finalize();
	srvManager->Finalize();
	dxComm->Finalize();
}

void kEngine::StartFrame() {
	dxComm->StartFrame();
	drawEngine->StartFrame();
	inputManager->KeysUpdata();
	timeManager->Update();
	drawDataCollector->PreCollect();
}

void kEngine::EndFrame() {
	drawDataCollector->EndCollect();
	drawEngine->PreDraw();
	drawEngine->CommitDraw();
	dxComm->EndFrame();
	drawEngine->EndDraw();
}

bool kEngine::ProcessMessage() {
	return dxComm->ProcessMessage();
}

#pragma endregion

#pragma region 描画システム

void kEngine::Draw2D(SpriteData* spriteData) {
	//resourceManager->Collet2D(spriteData);
	drawDataCollector->Collect2D(spriteData);
}

void kEngine::Draw3D(ObjectData* object) {
	//resourceManager->Collet3D(object);
	drawDataCollector->Collect3D(object);
}

int kEngine::GetModelTextureHandle(int modelHandle, int part) {
	return ResourceManager::GetInstance()->GetTextureHandleFromModelGroup(modelHandle, part);
}

int kEngine::GetMutiModelNum(int modelHandle) {
	return ResourceManager::GetInstance()->modelGroupList_[modelHandle]->GetModelNum();
}

int kEngine::SetModelObj(std::string path) {
	//return drawEngine->SetModel(path);
	return ResourceManager::GetInstance()->LoadModel(path);
}

void kEngine::AddLight(Light* light) {
	lightManager->AddLight(light);
}

void kEngine::RemoveLight(Light* light) {
	lightManager->RemoveLight(light);
}

DebugCamera* kEngine::CreateDebugCamera() {
	return cameraManager->CreateDebugCamera(this);
}

Camera* kEngine::CreateCamera() {
	return cameraManager->CreateCamera();
}

void kEngine::DestroyCamera(Camera* camera) {
	cameraManager->DestroyCamera(camera);
}

void kEngine::SetCamera(Camera* camera) {
	cameraManager->SetActiveCamera(camera);
}

void kEngine::ResetToDefaultCamera() {
	cameraManager->ResetActiveCamera();
}


int kEngine::commonTextureHandleReader(int handle) {
	return ResourceManager::GetInstance()->GetTextureHandleFromCommonList(handle);
};
int kEngine::commonModelHandleReader(int handle) {
	return ResourceManager::GetInstance()->ReadModelTextureHandle(handle);
};

int kEngine::LoadTexture(const std::string& filePath) {
	return ResourceManager::GetInstance()->LoadCommonTexture(filePath);
}

#pragma endregion

#pragma region 音関連

int kEngine::SoundLoadSE(const std::string filename) {
	return soundManager->SoundLoadFile(filename);
}

void kEngine::SoundPlaySE(int Handle, float volume) {
	soundManager->SoundPlaySE(Handle, volume);
}

void kEngine::SoundPlayBGM(int Handle, float volume) {
	soundManager->SoundPlayBGM(Handle, volume);
}

void kEngine::SoundSetVolume(int Handle, float volume) {
	soundManager->SoundSetVolume(Handle, volume);
}

void kEngine::SoundPause(int Handle) {
	soundManager->SoundPause(Handle);
}

void kEngine::SoundContinue(int Handle) {
	soundManager->SoundContinue(Handle);
}

void kEngine::SoundStop(int Handle) {
	soundManager->SoundStop(Handle);
}

void kEngine::SoundSetMasterVolume(float volume) {
	soundManager->SoundSetMasterVolume(volume);
}

void kEngine::SoundSetSEVolume(float volume) {
	soundManager->SoundSetMasterSEVolume(volume);
}

void kEngine::SoundSetBGMVolume(float volume) {
	soundManager->SoundSetMasterBGMVolume(volume);
}

bool kEngine::SoundIsPlaying(int Handle) {
	if (soundManager->isPlaying(Handle))return true;
	return false;
}

float kEngine::SoundGetVolume(int Handle) { return soundManager->SoundGetVolume(Handle); }
float kEngine::SoundGetMasterVolume() const { return soundManager->SoundGetMasterVolume(); }
float kEngine::SoundGetBGMVolume() const { return soundManager->SoundGetBGMVolume(); }
float kEngine::SoundGetSEVolume() const { return soundManager->SoundGetSEVolume(); }

void kEngine::SoundSetMute(int Handle, bool isMute) { soundManager->SoundSetMute(Handle, isMute); }
void kEngine::SoundSetMasterMute(bool isMute) { soundManager->SoundSetMasterMute(isMute); }
void kEngine::SoundSetBGMMute(bool isMute) { soundManager->SoundSetBGMMute(isMute); };
void kEngine::SoundSetSEMute(bool isMute) { soundManager->SoundSetSEMute(isMute); };

bool kEngine::SoundGetMute(int Handle) { return soundManager->SoundGetMute(Handle); }
bool kEngine::SoundGetMasterMute() const { return soundManager->SoundGetMasterMute(); }
bool kEngine::SoundGetBGMMute()const { return soundManager->SoundGetBGMMute(); }
bool kEngine::SoundGetSEMute()const { return soundManager->SoundGetSEMute(); }


#pragma endregion

#pragma region 入力関連

Vector2 kEngine::GetMousePosVector2() {
	return Vector2{ (float)inputManager->mousePosX(), (float)inputManager->mousePosY() };
}

int kEngine::GetMousePosX() {
	return inputManager->mousePosX();
}

int kEngine::GetMousePosY() {
	return inputManager->mousePosY();
}

int kEngine::GetMousePosXIns() {
	return inputManager->mousePosXIns();
}

int kEngine::GetMousePosYIns() {
	return inputManager->mousePosYIns();
}

bool kEngine::GetTriggerOn(int key) {
	if (inputManager->keyTriggerOn(key)) { return true; }
	return false;
}

bool kEngine::GetTriggerOff(int key) {
	if (inputManager->keyTriggerOff(key)) { return true; }
	return false;
}

bool kEngine::GetIsPush(int key) {
	if (inputManager->keyIsPush(key)) { return true; }
	return false;
}

bool kEngine::GetMouseTriggerOn(int key) {
	if (inputManager->mouseTriggerOn(key)) { return true; }
	return false;
}

bool kEngine::GetMouseTriggerOff(int key) {
	if (inputManager->mouseTriggerOff(key)) { return true; }
	return false;
}

bool kEngine::GetMouseIsPush(int key) {
	if (inputManager->mouseIsPush(key)) { return true; }
	return false;
}

int kEngine::GetMouseScroll() {
	return inputManager->mouseScrollSpeed();
}

int kEngine::GetMouseScrollOrigin() {
	return inputManager->mouseScrollSpeedOrigin();
}

bool kEngine::GetGamepadTriggerOn(int key) {
	return inputManager->gamepadTriggerOn(key);
}

bool kEngine::GetGamepadTriggerOff(int key) {
	return inputManager->gamepadTriggerOff(key);
}

bool kEngine::GetGamepadIsPush(int key) {
	return inputManager->gamepadIsPush(key);
}

int kEngine::GetGamepad4Direction() {
	return inputManager->gamepad4Direction();
}

int kEngine::GetGamepad4DirectionTriggerOn() {
	return inputManager->gamepad4DirectionTriggerOn();
}

int kEngine::GetGamepad4DirectionTriggerOff() {
	return inputManager->gamepad4DirectionTriggerOff();
}

int kEngine::GetGamepad8Direction() {
	return inputManager->gamepad8Direction();
}

int kEngine::GetGamepad8DirectionTriggerOn() {
	return inputManager->gamepad8DirectionTriggerOn();
}

int kEngine::GetGamepad8DirectionTriggerOff() {
	return inputManager->gamepad8DirectionTriggerOff();
}

int kEngine::GetGamepadDirectionFloat() {
	return inputManager->gamepadDirectionFloat();
}

float kEngine::GetGamepadLStick01X() {
	return inputManager->gamepadLStick01X();
}

float kEngine::GetGamepadLStick01Y() {
	return inputManager->gamepadLStick01Y();
}

int kEngine::GetGamepadLStickX() {
	return inputManager->gamepadLStickX();
}

int kEngine::GetGamepadLStickY() {
	return inputManager->gamepadLStickY();
}

float kEngine::GetGamepadRStick01X() {
	return inputManager->gamepadRStick01X();
}

float kEngine::GetGamepadRStick01Y() {
	return inputManager->gamepadRStick01Y();
}

int kEngine::GetGamepadRStickX() {
	return inputManager->gamepadRStickX();
}

int kEngine::GetGamepadRStickY() {
	return inputManager->gamepadRStickY();
}

float kEngine::GetGamepadL201() {
	return inputManager->gamepadL201();
}

float kEngine::GetGamepadR201() {
	return inputManager->gamepadR201();
}

int kEngine::GetGamepadL2() {
	return inputManager->gamepadL2();
}

int kEngine::GetGamepadR2() {
	return inputManager->gamepadR2();
}


#pragma endregion

#pragma region TimeManager

float kEngine::GetFPS() {
	return timeManager->getInstantFPS();
}

float kEngine::GetFPSPerSecond() {
	return timeManager->getFPSPerSecond();
}

float kEngine::GetDeltaTime() {
	return timeManager->getDeltaTime();
}

void kEngine::SetTimeScale(float timeScale) {
	timeManager->setTimeScale(timeScale);
}

float kEngine::GetTimeScale() const {
	return timeManager->getTimeScale();
}

float kEngine::GetScaledDeltaTime() const {
	return timeManager->getScaledDeltaTime();
}

void kEngine::SetTimerTimeScale(float timerTimeScale) {
	timeManager->setTimerTimeScale(timerTimeScale);
}

float kEngine::GetTimerTimeScale_() const {
	return timeManager->getTimerTimeScale();
}

float kEngine::GetTimerScaledDeltaTime_() const {
	return timeManager->getTimerScaledDeltaTime();
}

#pragma endregion

#pragma region システムインターフェース

DirectXController* kEngine::GetDirectXController() {
	return dxComm.get();
}

SrvManager* kEngine::GetSrvManager() {
	return srvManager.get();
}

ResourceManager* kEngine::GetResourceManager() const {
	return ResourceManager::GetInstance();
}

TextureManager* kEngine::GetTextureManager() const {
	return TextureManager::GetInstance();
}

LightManager* kEngine::GetLightManager() const {
	return lightManager.get();
}

CameraManager* kEngine::GetCameraManager() const {
	return cameraManager.get();
}

DrawDataCollector* kEngine::GetDrawDataCollector() const {
	return drawDataCollector.get();
}

DrawEngine* kEngine::GetDrawEngine() const {
	return drawEngine.get();
}

SoundManager* kEngine::GetSoundManager() const {
	return soundManager.get();
}

InputManager* kEngine::GetInputManager() const {
	return inputManager.get();
}

TimeManager* kEngine::GetTimeManager() const {
	return timeManager.get();
}

#pragma endregion

