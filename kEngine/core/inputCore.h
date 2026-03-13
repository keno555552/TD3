#pragma once
#include "DirectXCore.h"
#include "tool/TimeManager/TimeManager.h"
#include "config.h"
#include <cassert>

class InputCore
{
public:
	void Initialize(DirectXCore* directXDirver, TimeManager* timeManager);
	void Finalize();

	void KeysUpdata();
	void Keys3sUpdata();

	/// keyboard関連
	bool keyTriggerOn(int key);
	bool keyTriggerOff(int key);
	bool keyIsPush(int key);
	void CheckIsKeyboardConnet();

	/// mouse関連
	int mousePosX();
	int mousePosY();
	int mousePosXIns();
	int mousePosYIns();
	bool mouseTriggerOn(int key);
	bool mouseTriggerOff(int key);
	bool mouseIsPush(int key);
	int mouseScrollSpeed();
	int mouseScrollSpeedOrigin();
	void CheckIsMouseConnet();

	/// GamePad関連
	bool gamepadTriggerOn(int key);
	bool gamepadTriggerOff(int key);
	bool gamepadIsPush(int key);
	// 出力1-4、順序は上右下左,0は押されてない
	int gamepad4Direction();
	int gamepad4DirectionTriggerOn();
	int gamepad4DirectionTriggerOff();
	int gamepad8Direction();
	int gamepad8DirectionTriggerOn();
	int gamepad8DirectionTriggerOff();
	int gamepadDirectionFloat();
	float gamepadLStick01X();
	float gamepadLStick01Y();
	int gamepadLStickX();
	int gamepadLStickY();
	float gamepadRStick01X();
	float gamepadRStick01Y();
	int gamepadRStickX();
	int gamepadRStickY();
	float gamepadL201();
	float gamepadR201();
	int gamepadL2();
	int gamepadR2();

	bool CheckIsGamepadConnet();
	int DIKeyChange(int input);
	int XIKeyChange(int input);

private:
	DirectXCore* driver_ = nullptr;    // 借り
	BYTE keys_[256] = { 0 };
	BYTE preKeys_[256] = { 0 };
	DIMOUSESTATE mouse_ = { 0 };
	DIMOUSESTATE preMouse_ = { 0 };
	POINT mousePos_ = { 0 };
	DIJOYSTATE padData_;
	DIJOYSTATE prePadData_ = { 0 };
	WORD XpadKeysData_ = { 0 };
	WORD XprePadKeysData_ = { 0 };
	XINPUT_STATE XpadData_ = { 0 };
	XINPUT_STATE XprePadData_ = { 0 };

	std::unique_ptr<Timer> timer{};

private:

	/// XInput
	bool gamepadXITriggerOn(int key);
	bool gamepadXITriggerOff(int key);
	bool gamepadXIIsPush(int key);
	// 出力0-3、順序は上右下左
	int gamepadXI4Direction();
	int gamepadXI4DirectionTriggerOn();
	int gamepadXI4DirectionTriggerOff();
	int gamepadXI8Direction();
	int gamepadXI8DirectionTriggerOn();
	int gamepadXI8DirectionTriggerOff();
	int gamepadXIDirectionFloat();
	float gamepadXILStick01X();
	float gamepadXILStick01Y();
	int gamepadXILStickX();
	int gamepadXILStickY();
	float gamepadXIRStick01X();
	float gamepadXIRStick01Y();
	int gamepadXIRStickX();
	int gamepadXIRStickY();
	float gamepadXIL201();
	float gamepadXIR201();
	int gamepadXIL2();
	int gamepadXIR2();

	/// DirectInput
	bool gamepadDITriggerOn(int key);
	bool gamepadDITriggerOff(int key);
	bool gamepadDIIsPush(int key);
	// 出力0-3、順序は上右下左
	int gamepadDI4Direction();
	int gamepadDI4DirectionTriggerOn();
	int gamepadDI4DirectionTriggerOff();
	int gamepadDI8Direction();
	int gamepadDI8DirectionTriggerOn();
	int gamepadDI8DirectionTriggerOff();
	int gamepadDIDirectionFloat();
	float gamepadDILStick01X();
	float gamepadDILStick01Y();
	int gamepadDILStickX();
	int gamepadDILStickY();
	float gamepadDIRStick01X();
	float gamepadDIRStick01Y();
	int gamepadDIRStickX();
	int gamepadDIRStickY();
	float gamepadDIL201();
	float gamepadDIR201();
	int gamepadDIL2();
	int gamepadDIR2();
};