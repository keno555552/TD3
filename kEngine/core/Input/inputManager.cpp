#include "inputManager.h"

void InputManager::Initialize(DirectXCore* directXDirver, TimeManager* timeManager) {
	driver_ = directXDirver;
	timer->Init0(3, timeManager);
}

void InputManager::Finalize() {
}

void InputManager::KeysUpdata()   {
	/// keyboard
	if (config::GetKeyboardState()) {
		memcpy(preKeys_, keys_, sizeof keys_);
		driver_->GetDirectInputKeyboard()->Acquire();
		driver_->GetDirectInputKeyboard()->GetDeviceState(sizeof(keys_), keys_);
	}

	/// mouse
	if (config::GetMouseState()) {
		preMouse_ = mouse_;
		driver_->GetDirectInputMouse()->Acquire();
		driver_->GetDirectInputMouse()->GetDeviceState(sizeof(DIMOUSESTATE), &mouse_);
		GetCursorPos(&mousePos_);
		ScreenToClient(driver_->GetHWND(), &mousePos_);
	}

	/// Gamepad
	if (config::GetGamePadState()) {
		if (config::GetGamePadType() == DIRECTINPUT) {
			prePadData_ = padData_;
			driver_->GetDirectInputGamepad()->Acquire();
			driver_->GetDirectInputGamepad()->Poll();
			driver_->GetDirectInputGamepad()->GetDeviceState(sizeof(DIJOYSTATE), &padData_);
		}
		memcpy(&XprePadData_, &XpadData_, sizeof(XINPUT_STATE));
		XINPUT_STATE currentState;
		XInputGetState(config::GetXInputIp(), &currentState);
		if (XInputGetState(config::GetXInputIp(), &currentState) == ERROR_SUCCESS) {
			if (currentState.dwPacketNumber != XpadData_.dwPacketNumber) {
				memcpy(&XpadData_, &currentState, sizeof(XINPUT_STATE));
			}
		}
	}

	Keys3sUpdata();
}

void InputManager::Keys3sUpdata() {
	///今はいいが,後で3秒更新するにした方がいい
	if (timer->parameter_ == 0) {
		CheckIsKeyboardConnet();
		CheckIsMouseConnet();
		CheckIsGamepadConnet();
	}
	timer->foreverUp();
}

#pragma region keyboard関連

bool InputManager::keyTriggerOn(int key) {
	if (!preKeys_[key] && keys_[key]) { return true; }
	return false;
}

bool InputManager::keyTriggerOff(int key) {
	if (preKeys_[key] && !keys_[key]) { return true; }
	return false;
}

bool InputManager::keyIsPush(int key) {
	if (keys_[key]) { return true; }
	return false;
}

void InputManager::CheckIsKeyboardConnet() {
	bool flag1 = config::GetKeyboardState();
	bool flag2 = SUCCEEDED(driver_->GetDirectInput()->GetDeviceStatus(GUID_SysKeyboard));

	if (flag1 && !flag2) {
		driver_->ReleaseDirectXInputKeyBoard();
	}

	if (!flag1 && flag2) {
		driver_->SetDirectXInputKeyBoard();
	}
}

#pragma endregion

#pragma region mouse関連
int InputManager::mousePosX() {
	return mousePos_.x;
}

int InputManager::mousePosY() {
	return mousePos_.y;
}

int InputManager::mousePosXIns() {
	return mouse_.lX;;
}

int InputManager::mousePosYIns() {
	return mouse_.lY;;
}

bool InputManager::mouseTriggerOn(int key) {
	if (!(preMouse_.rgbButtons[key] & 0x80) && (mouse_.rgbButtons[key] & 0x80)) { return true; }
	return false;
}

bool InputManager::mouseTriggerOff(int key) {
	if ((preMouse_.rgbButtons[key] & 0x80) && !(mouse_.rgbButtons[key] & 0x80)) { return true; }
	return false;
}

bool InputManager::mouseIsPush(int key) {
	if (mouse_.rgbButtons[key] & 0x80) { return true; }
	return false;
}

int InputManager::mouseScrollSpeed() {
	if (mouse_.lZ > 0) { return  1; }
	if (mouse_.lZ < 0) { return -1; }
	return 0;
}

int InputManager::mouseScrollSpeedOrigin() {
	return mouse_.lZ;
}

void InputManager::CheckIsMouseConnet() {
	bool flag1 = config::GetMouseState();
	bool flag2 = SUCCEEDED(driver_->GetDirectInput()->GetDeviceStatus(GUID_SysMouse));

	if (flag1 && !flag2) {
		driver_->ReleaseDirectXInputMouse();
	}

	if (!flag1 && flag2) {
		driver_->SetDirectXInputMouse();
	}
}


#pragma endregion

#pragma region GamePad関連

bool InputManager::gamepadTriggerOn(int key) {
	if (config::GetGamePadType() == XINPUT) { return gamepadXITriggerOn(XIKeyChange(key)); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDITriggerOn(DIKeyChange(key)); }
	return 0;
}

bool InputManager::gamepadTriggerOff(int key) {
	if (config::GetGamePadType() == XINPUT) { return gamepadXITriggerOff(XIKeyChange(key)); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDITriggerOff(DIKeyChange(key)); }
	return 0;
}

bool InputManager::gamepadIsPush(int key) {
	if (config::GetGamePadType() == XINPUT) { return gamepadXIIsPush(XIKeyChange(key)); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDIIsPush(DIKeyChange(key)); }
	return 0;
}

int InputManager::gamepad4Direction() {
	if (config::GetGamePadType() == XINPUT) { return gamepadXI4Direction(); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDI4Direction(); }
	return 0;
}

int InputManager::gamepad4DirectionTriggerOn() {
	if (config::GetGamePadType() == XINPUT) { return gamepadXI4DirectionTriggerOn(); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDI4DirectionTriggerOn(); }
	return 0;
}

int InputManager::gamepad4DirectionTriggerOff() {
	if (config::GetGamePadType() == XINPUT) { return gamepadXI4DirectionTriggerOff(); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDI4DirectionTriggerOff(); }
	return 0;
}

int InputManager::gamepad8Direction() {
	if (config::GetGamePadType() == XINPUT) { return gamepadXI8Direction(); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDI8Direction(); }
	return 0;
};


int InputManager::gamepad8DirectionTriggerOn() {
	if (config::GetGamePadType() == XINPUT) { return gamepadXI8DirectionTriggerOn(); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDI8DirectionTriggerOn(); }
	return 0;
}

int InputManager::gamepad8DirectionTriggerOff() {
	if (config::GetGamePadType() == XINPUT) { return gamepadXI8DirectionTriggerOff(); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDI8DirectionTriggerOff(); }
	return 0;
}

int InputManager::gamepadDirectionFloat() {
	if (config::GetGamePadType() == XINPUT) { return gamepadXIDirectionFloat(); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDIDirectionFloat(); }
	return 0;
}

float InputManager::gamepadLStick01X() {
	if (config::GetGamePadType() == XINPUT) { return gamepadXILStick01X(); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDILStick01X(); }
	return 0;
}

float InputManager::gamepadLStick01Y() {
	if (config::GetGamePadType() == XINPUT) { return gamepadXILStick01Y(); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDILStick01Y(); }
	return 0;
}

int InputManager::gamepadLStickX() {
	if (config::GetGamePadType() == XINPUT) { return gamepadXILStickX(); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDILStickX(); }
	return 0;
}

int InputManager::gamepadLStickY() {
	if (config::GetGamePadType() == XINPUT) { return gamepadXILStickY(); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDILStickY(); }
	return 0;
}

float InputManager::gamepadRStick01X() {
	if (config::GetGamePadType() == XINPUT) { return gamepadXIRStick01X(); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDIRStick01X(); }
	return 0;
}

float InputManager::gamepadRStick01Y() {
	if (config::GetGamePadType() == XINPUT) { return gamepadXIRStick01Y(); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDIRStick01Y(); }
	return 0;
}

int InputManager::gamepadRStickX() {
	if (config::GetGamePadType() == XINPUT) { return gamepadXIRStickX(); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDIRStickX(); }
	return 0;
}

int InputManager::gamepadRStickY() {
	if (config::GetGamePadType() == XINPUT) { return gamepadXIRStickY(); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDIRStickY(); }
	return 0;
}

float InputManager::gamepadL201() {
	if (config::GetGamePadType() == XINPUT) { return gamepadXIL201(); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDIL201(); }
	return 0;
}

float InputManager::gamepadR201() {
	if (config::GetGamePadType() == XINPUT) { return gamepadXIR201(); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDIR201(); }
	return 0;
}

int InputManager::gamepadL2() {
	if (config::GetGamePadType() == XINPUT) { return gamepadXIL2(); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDIL2(); }
	return 0;
}

int InputManager::gamepadR2() {
	if (config::GetGamePadType() == XINPUT) { return gamepadXIR2(); }
	if (config::GetGamePadType() == DIRECTINPUT) { return gamepadDIR2(); }
	return 0;
}

#pragma region XInput関連

bool InputManager::gamepadXITriggerOn(int key) {
	if (key == VK_PAD_LTRIGGER) { 				/// Button[06] = Direct(PS4): L2
		if (!(XprePadData_.Gamepad.bLeftTrigger > 10) && (XpadData_.Gamepad.bLeftTrigger > 10)) { return true; } else { return false; }
	}
	if (key == VK_PAD_RTRIGGER) { 				/// Button[07] = Direct(PS4): R2
		if (!(XprePadData_.Gamepad.bRightTrigger > 10) && (XpadData_.Gamepad.bRightTrigger > 10)) { return true; } else { return false; }
	}
	return (!(XprePadData_.Gamepad.wButtons & key) && (XpadData_.Gamepad.wButtons & key));
}

bool InputManager::gamepadXITriggerOff(int key) {
	if (key == VK_PAD_LTRIGGER) { 				/// Button[06] = Direct(PS4): L2
		if ((XprePadData_.Gamepad.bLeftTrigger > 10) && !(XpadData_.Gamepad.bLeftTrigger > 10)) { return true; }
		else { return false; }
	}
	if (key == VK_PAD_RTRIGGER) { 				/// Button[07] = Direct(PS4): R2
		if ((XprePadData_.Gamepad.bRightTrigger > 10) && !(XpadData_.Gamepad.bRightTrigger > 10)) { return true; }
		else { return false; }
	}
	return ((XprePadData_.Gamepad.wButtons & key) && !(XpadData_.Gamepad.wButtons & key));
}

bool InputManager::gamepadXIIsPush(int key) {
	if (key == VK_PAD_LTRIGGER) { 				/// Button[06] = Direct(PS4): L2
		if (XpadData_.Gamepad.bLeftTrigger > 10) { return true; }
		else { return false; }
	}
	if (key == VK_PAD_RTRIGGER) { 				/// Button[07] = Direct(PS4): R2
		if (XpadData_.Gamepad.bRightTrigger > 10) { return true; }
		else { return false; }
	}
	return (XpadData_.Gamepad.wButtons & key);
}

int InputManager::gamepadXI4Direction() {
	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) { return 1; }
	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) { return 2; };
	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) { return 3; };
	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) { return 4; };
	return 0;
}

int InputManager::gamepadXI4DirectionTriggerOn() {
	int preFlag = -1;
	int flag = -1;

	if (XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) { preFlag = 1; }
	if (XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) { preFlag = 2; }
	if (XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) { preFlag = 3; }
	if (XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) { preFlag = 4; }

	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) { flag = 1; }
	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) { flag = 2; }
	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) { flag = 3; }
	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) { flag = 4; }

	if (preFlag != 1 && flag == 1) { return 1; }
	if (preFlag != 2 && flag == 2) { return 2; }
	if (preFlag != 3 && flag == 3) { return 3; }
	if (preFlag != 4 && flag == 4) { return 4; }
	return 0;
}

int InputManager::gamepadXI4DirectionTriggerOff() {
	int preFlag = -1;
	int flag = -1;

	if (XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) { preFlag = 1; }
	if (XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) { preFlag = 2; }
	if (XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) { preFlag = 3; }
	if (XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) { preFlag = 4; }

	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) { flag = 1; }
	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) { flag = 2; }
	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) { flag = 3; }
	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) { flag = 4; }

	if (preFlag == 1 && flag != 1) { return 1; }
	if (preFlag == 2 && flag != 2) { return 2; }
	if (preFlag == 3 && flag != 3) { return 3; }
	if (preFlag == 4 && flag != 4) { return 4; }
	return 0;
}

int InputManager::gamepadXI8Direction() {
	if ((XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) &&
		(XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)) {
		return 2;
	};
	if ((XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) &&
		(XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)) {
		return 4;
	};
	if ((XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) &&
		(XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)) {
		return 6;
	};
	if ((XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) &&
		(XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)) {
		return 8;
	};
	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) { return 1; }
	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) { return 3; };
	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) { return 5; }
	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) { return 7; };
	return 0;
}

int InputManager::gamepadXI8DirectionTriggerOn() {
	int preFlag = -1;
	int flag = -1;

	if (XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) { preFlag = 1; }
	if (XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) { preFlag = 3; };
	if (XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) { preFlag = 5; }
	if (XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) { preFlag = 7; };
	if ((XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) &&
		(XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)) {
		preFlag = 2;
	};
	if ((XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) &&
		(XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)) {
		preFlag = 4;
	};
	if ((XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) &&
		(XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)) {
		preFlag = 6;
	};
	if ((XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) &&
		(XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)) {
		preFlag = 8;
	};


	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) { flag = 1; }
	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) { flag = 3; };
	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) { flag = 5; }
	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) { flag = 7; };
	if ((XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) &&
		(XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)) {
		flag = 2;
	};
	if ((XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) &&
		(XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)) {
		flag = 4;
	};
	if ((XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) &&
		(XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)) {
		flag = 6;
	};
	if ((XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) &&
		(XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)) {
		flag = 8;
	};

	if (preFlag != 1 && flag == 1) { return 1; }
	if (preFlag != 2 && flag == 2) { return 2; }
	if (preFlag != 3 && flag == 3) { return 3; }
	if (preFlag != 4 && flag == 4) { return 4; }
	if (preFlag != 5 && flag == 5) { return 5; }
	if (preFlag != 6 && flag == 6) { return 6; }
	if (preFlag != 7 && flag == 7) { return 7; }
	if (preFlag != 8 && flag == 8) { return 8; }
	return 0;
}

int InputManager::gamepadXI8DirectionTriggerOff() {
	int preFlag = -1;
	int flag = -1;

	if (XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) { preFlag = 1; }
	if (XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) { preFlag = 3; };
	if (XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) { preFlag = 5; }
	if (XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) { preFlag = 7; };
	if ((XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) &&
		(XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)) {
		preFlag = 2;
	};
	if ((XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) &&
		(XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)) {
		preFlag = 4;
	};
	if ((XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) &&
		(XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)) {
		preFlag = 6;
	};
	if ((XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) &&
		(XprePadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)) {
		preFlag = 8;
	};


	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) { flag = 1; }
	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) { flag = 3; };
	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) { flag = 5; }
	if (XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) { flag = 7; };
	if ((XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) &&
		(XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)) {
		flag = 2;
	};
	if ((XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) &&
		(XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)) {
		flag = 4;
	};
	if ((XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) &&
		(XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)) {
		flag = 6;
	};
	if ((XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) &&
		(XpadData_.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)) {
		flag = 8;
	};

	if (preFlag == 1 && flag != 1) { return 1; }
	if (preFlag == 2 && flag != 2) { return 2; }
	if (preFlag == 3 && flag != 3) { return 3; }
	if (preFlag == 4 && flag != 4) { return 4; }
	if (preFlag == 5 && flag != 5) { return 5; }
	if (preFlag == 6 && flag != 6) { return 6; }
	if (preFlag == 7 && flag != 7) { return 7; }
	if (preFlag == 8 && flag != 8) { return 8; }
	return 0;
}

int InputManager::gamepadXIDirectionFloat() {
	return 0;
}

float InputManager::gamepadXILStick01X() {
	float result = ((float)XpadData_.Gamepad.sThumbLX);
	if (result > -7000.0f && result < 7000.0f) {
		return 0.0f;
	} else {
		result /= 32767.0f;
		return result;
	}
	return -1.0f;
}

float InputManager::gamepadXILStick01Y() {
	float result = ((float)XpadData_.Gamepad.sThumbLY);
	if (result > -7000.0f && result < 7000.0f) {
		return 0.0f;
	} else {
		result /= 32767.0f;
		return result;
	}
	return -1.0f;
}

int InputManager::gamepadXILStickX() {
	return XpadData_.Gamepad.sThumbLX;
}

int InputManager::gamepadXILStickY() {
	return XpadData_.Gamepad.sThumbLY;
}

float InputManager::gamepadXIRStick01X() {
	float result = ((float)XpadData_.Gamepad.sThumbRX);
	if (result > -7000.0f && result < 7000.0f) {
		return 0.0f;
	} else {
		result /= 32767.0f;
		return result;
	}
	return -1.0f;
}

float InputManager::gamepadXIRStick01Y() {
	float result = ((float)XpadData_.Gamepad.sThumbRY);
	if (result > -7000.0f && result < 7000.0f) {
		return 0.0f;
	} else {
		result /= 32767.0f;
		return result;
	}
	return -1.0f;
}

int InputManager::gamepadXIRStickX() {
	return XpadData_.Gamepad.sThumbRX;
}

int InputManager::gamepadXIRStickY() {
	return XpadData_.Gamepad.sThumbRY;
}

float InputManager::gamepadXIL201() {
	float result = ((float)XpadData_.Gamepad.bLeftTrigger);
	result /= 255.0f;
	return result;
}

float InputManager::gamepadXIR201() {
	float result = ((float)XpadData_.Gamepad.bRightTrigger);
	result /= 255.0f;
	return result;
}

int InputManager::gamepadXIL2() {
	float result = ((float)XpadData_.Gamepad.bLeftTrigger);
	result = result / 255.0f * 65535.0f;
	return (int)result;
}

int InputManager::gamepadXIR2() {
	float result = ((float)XpadData_.Gamepad.bRightTrigger);
	result = result / 255.0f * 65535.0f;
	return (int)result;
}

#pragma endregion

#pragma region DirectInput関連

/// 通常配置
/// Button[00] = Direct(PS4): []
/// Button[01] = Direct(PS4): X
/// Button[02] = Direct(PS4): 0
/// Button[03] = Direct(PS4): A
/// Button[04] = Direct(PS4): L1
/// Button[05] = Direct(PS4): R1
/// Button[06] = Direct(PS4): L2
/// Button[07] = Direct(PS4): R2
/// Button[08] = Direct(PS4): Share
/// Button[09] = Direct(PS4): Start
/// Button[10] = Direct(PS4): 左ステック押し
/// Button[11] = Direct(PS4): 右ステック押し
/// Button[12] = Direct(PS4): PSButton
/// Button[13] = Direct(PS4): 中央パッド


bool InputManager::gamepadDITriggerOn(int key) {
	if (!(prePadData_.rgbButtons[key] & 0x80) && (padData_.rgbButtons[key] & 0x80)) { return true; }
	return false;
}

bool InputManager::gamepadDITriggerOff(int key) {
	if ((prePadData_.rgbButtons[key] & 0x80) && !(padData_.rgbButtons[key] & 0x80)) { return true; }
	return false;
}

bool InputManager::gamepadDIIsPush(int key) {
	if (padData_.rgbButtons[key] & 0x80) { return true; }
	return false;
}

int InputManager::gamepadDI4Direction() {
	if (padData_.rgdwPOV[0] == -1) { return 0; }
	if (padData_.rgdwPOV[0] > 31500 || padData_.rgdwPOV[0] <= 4500) { return 1; }// 0
	if (padData_.rgdwPOV[0] > 4500 && padData_.rgdwPOV[0] <= 13500) { return 2; }// 9000
	if (padData_.rgdwPOV[0] > 13500 && padData_.rgdwPOV[0] <= 22500) { return 3; }// 18000
	if (padData_.rgdwPOV[0] > 22500 && padData_.rgdwPOV[0] <= 31500) { return 4; }// 27000
	return -1;
}

int InputManager::gamepadDI4DirectionTriggerOn() {
	int preFlag = -1;
	int flag = -1;

	if (prePadData_.rgdwPOV[0] == -1) {
		preFlag = 0;
	} else {
		if (prePadData_.rgdwPOV[0] > 31500 || prePadData_.rgdwPOV[0] <= 4500) { preFlag = 1; }// 0
		if (prePadData_.rgdwPOV[0] > 4500 && prePadData_.rgdwPOV[0] <= 13500) { preFlag = 2; }// 9000
		if (prePadData_.rgdwPOV[0] > 13500 && prePadData_.rgdwPOV[0] <= 22500) { preFlag = 3; }// 18000
		if (prePadData_.rgdwPOV[0] > 22500 && prePadData_.rgdwPOV[0] <= 31500) { preFlag = 4; }// 27000
	}

	if (padData_.rgdwPOV[0] == -1) {
		flag = 0;
	} else {
		if (padData_.rgdwPOV[0] > 31500 || padData_.rgdwPOV[0] <= 4500) { flag = 1; }// 0
		if (padData_.rgdwPOV[0] > 4500 && padData_.rgdwPOV[0] <= 13500) { flag = 2; }// 9000
		if (padData_.rgdwPOV[0] > 13500 && padData_.rgdwPOV[0] <= 22500) { flag = 3; }// 18000
		if (padData_.rgdwPOV[0] > 22500 && padData_.rgdwPOV[0] <= 31500) { flag = 4; }// 27000
	}

	if (preFlag != 1 && flag == 1) { return 1; }
	if (preFlag != 2 && flag == 2) { return 2; }
	if (preFlag != 3 && flag == 3) { return 3; }
	if (preFlag != 4 && flag == 4) { return 4; }
	return 0;
}

int InputManager::gamepadDI4DirectionTriggerOff() {
	int preFlag = -1;
	int flag = -1;

	if (prePadData_.rgdwPOV[0] == -1) {
		preFlag = 0;
	} else {
		if (prePadData_.rgdwPOV[0] > 31500 || prePadData_.rgdwPOV[0] <= 4500) { preFlag = 1; }// 0
		if (prePadData_.rgdwPOV[0] > 4500 && prePadData_.rgdwPOV[0] <= 13500) { preFlag = 2; }// 9000
		if (prePadData_.rgdwPOV[0] > 13500 && prePadData_.rgdwPOV[0] <= 22500) { preFlag = 3; }// 18000
		if (prePadData_.rgdwPOV[0] > 22500 && prePadData_.rgdwPOV[0] <= 31500) { preFlag = 4; }// 27000
	}
	if (padData_.rgdwPOV[0] == -1) {
		flag = 0;
	} else {
		if (padData_.rgdwPOV[0] > 31500 || padData_.rgdwPOV[0] <= 4500) { flag = 1; }// 0
		if (padData_.rgdwPOV[0] > 4500 && padData_.rgdwPOV[0] <= 13500) { flag = 2; }// 9000
		if (padData_.rgdwPOV[0] > 13500 && padData_.rgdwPOV[0] <= 22500) { flag = 3; }// 18000
		if (padData_.rgdwPOV[0] > 22500 && padData_.rgdwPOV[0] <= 31500) { flag = 4; }// 27000
	}

	if (preFlag == 1 && flag != 1) { return 1; }
	if (preFlag == 2 && flag != 2) { return 2; }
	if (preFlag == 3 && flag != 3) { return 3; }
	if (preFlag == 4 && flag != 4) { return 4; }
	return 0;
}

int InputManager::gamepadDI8Direction() {
	if (padData_.rgdwPOV[0] == -1) { return 0; }
	if (padData_.rgdwPOV[0] > 33750 || padData_.rgdwPOV[0] <= 2250) { return 1; }// 0
	if (padData_.rgdwPOV[0] > 2250 && padData_.rgdwPOV[0] <= 6750) { return 2; }// 4500
	if (padData_.rgdwPOV[0] > 6750 && padData_.rgdwPOV[0] <= 11250) { return 3; }// 9000
	if (padData_.rgdwPOV[0] > 11250 && padData_.rgdwPOV[0] <= 15750) { return 4; }// 13500
	if (padData_.rgdwPOV[0] > 15750 && padData_.rgdwPOV[0] <= 20250) { return 5; }// 18000
	if (padData_.rgdwPOV[0] > 20250 && padData_.rgdwPOV[0] <= 24750) { return 6; }// 22500
	if (padData_.rgdwPOV[0] > 24750 && padData_.rgdwPOV[0] <= 29250) { return 7; }// 27000
	if (padData_.rgdwPOV[0] > 29250 && padData_.rgdwPOV[0] <= 33750) { return 8; }// 31500
	return -1;
}


int InputManager::gamepadDI8DirectionTriggerOn() {
	int preFlag = -1;
	int flag = -1;

	if (prePadData_.rgdwPOV[0] == -1) {
		preFlag = 0;
	} else {
		if (prePadData_.rgdwPOV[0] > 33750 || prePadData_.rgdwPOV[0] <= 2250) { preFlag = 1; }// 0
		if (prePadData_.rgdwPOV[0] > 2250 && prePadData_.rgdwPOV[0] <= 6750) { preFlag = 2; }// 4500
		if (prePadData_.rgdwPOV[0] > 6750 && prePadData_.rgdwPOV[0] <= 11250) { preFlag = 3; }// 9000
		if (prePadData_.rgdwPOV[0] > 11250 && prePadData_.rgdwPOV[0] <= 15750) { preFlag = 4; }// 13500
		if (prePadData_.rgdwPOV[0] > 15750 && prePadData_.rgdwPOV[0] <= 20250) { preFlag = 5; }// 18000
		if (prePadData_.rgdwPOV[0] > 20250 && prePadData_.rgdwPOV[0] <= 24750) { preFlag = 6; }// 22500
		if (prePadData_.rgdwPOV[0] > 24750 && prePadData_.rgdwPOV[0] <= 29250) { preFlag = 7; }// 27000
		if (prePadData_.rgdwPOV[0] > 29250 && prePadData_.rgdwPOV[0] <= 33750) { preFlag = 8; }// 31500
	}

	if (padData_.rgdwPOV[0] == -1) {
		flag = 0;
	} else {
		if (padData_.rgdwPOV[0] > 33750 || padData_.rgdwPOV[0] <= 2250) { flag = 1; }// 0
		if (padData_.rgdwPOV[0] > 2250 && padData_.rgdwPOV[0] <= 6750) { flag = 2; }// 4500
		if (padData_.rgdwPOV[0] > 6750 && padData_.rgdwPOV[0] <= 11250) { flag = 3; }// 9000
		if (padData_.rgdwPOV[0] > 11250 && padData_.rgdwPOV[0] <= 15750) { flag = 4; }// 13500
		if (padData_.rgdwPOV[0] > 15750 && padData_.rgdwPOV[0] <= 20250) { flag = 5; }// 18000
		if (padData_.rgdwPOV[0] > 20250 && padData_.rgdwPOV[0] <= 24750) { flag = 6; }// 22500
		if (padData_.rgdwPOV[0] > 24750 && padData_.rgdwPOV[0] <= 29250) { flag = 7; }// 27000
		if (padData_.rgdwPOV[0] > 29250 && padData_.rgdwPOV[0] <= 33750) { flag = 8; }// 31500
	}

	if (preFlag != 1 && flag == 1) { return 1; }
	if (preFlag != 2 && flag == 2) { return 2; }
	if (preFlag != 3 && flag == 3) { return 3; }
	if (preFlag != 4 && flag == 4) { return 4; }
	if (preFlag != 5 && flag == 5) { return 5; }
	if (preFlag != 6 && flag == 6) { return 6; }
	if (preFlag != 7 && flag == 7) { return 7; }
	if (preFlag != 8 && flag == 8) { return 8; }
	return 0;
}

int InputManager::gamepadDI8DirectionTriggerOff() {
	int preFlag = -1;
	int flag = -1;

	if (prePadData_.rgdwPOV[0] == -1) {
		preFlag = 0;
	} else {
		if (prePadData_.rgdwPOV[0] > 33750 || prePadData_.rgdwPOV[0] <= 2250) { preFlag = 1; }// 0
		if (prePadData_.rgdwPOV[0] > 2250 && prePadData_.rgdwPOV[0] <= 6750) { preFlag = 2; }// 4500
		if (prePadData_.rgdwPOV[0] > 6750 && prePadData_.rgdwPOV[0] <= 11250) { preFlag = 3; }// 9000
		if (prePadData_.rgdwPOV[0] > 11250 && prePadData_.rgdwPOV[0] <= 15750) { preFlag = 4; }// 13500
		if (prePadData_.rgdwPOV[0] > 15750 && prePadData_.rgdwPOV[0] <= 20250) { preFlag = 5; }// 18000
		if (prePadData_.rgdwPOV[0] > 20250 && prePadData_.rgdwPOV[0] <= 24750) { preFlag = 6; }// 22500
		if (prePadData_.rgdwPOV[0] > 24750 && prePadData_.rgdwPOV[0] <= 29250) { preFlag = 7; }// 27000
		if (prePadData_.rgdwPOV[0] > 29250 && prePadData_.rgdwPOV[0] <= 33750) { preFlag = 8; }// 31500
	}

	if (padData_.rgdwPOV[0] == -1) {
		flag = 0;
	} else {
		if (padData_.rgdwPOV[0] > 33750 || padData_.rgdwPOV[0] <= 2250) { flag = 1; }// 0
		if (padData_.rgdwPOV[0] > 2250 && padData_.rgdwPOV[0] <= 6750) { flag = 2; }// 4500
		if (padData_.rgdwPOV[0] > 6750 && padData_.rgdwPOV[0] <= 11250) { flag = 3; }// 9000
		if (padData_.rgdwPOV[0] > 11250 && padData_.rgdwPOV[0] <= 15750) { flag = 4; }// 13500
		if (padData_.rgdwPOV[0] > 15750 && padData_.rgdwPOV[0] <= 20250) { flag = 5; }// 18000
		if (padData_.rgdwPOV[0] > 20250 && padData_.rgdwPOV[0] <= 24750) { flag = 6; }// 22500
		if (padData_.rgdwPOV[0] > 24750 && padData_.rgdwPOV[0] <= 29250) { flag = 7; }// 27000
		if (padData_.rgdwPOV[0] > 29250 && padData_.rgdwPOV[0] <= 33750) { flag = 8; }// 31500
	}

	if (preFlag == 1 && flag != 1) { return 1; }
	if (preFlag == 2 && flag != 2) { return 2; }
	if (preFlag == 3 && flag != 3) { return 3; }
	if (preFlag == 4 && flag != 4) { return 4; }
	if (preFlag == 5 && flag != 5) { return 5; }
	if (preFlag == 6 && flag != 6) { return 6; }
	if (preFlag == 7 && flag != 7) { return 7; }
	if (preFlag == 8 && flag != 8) { return 8; }
	return 0;
}

int InputManager::gamepadDIDirectionFloat() {
	return padData_.rgdwPOV[0];
}

float InputManager::gamepadDILStick01X() {
	float result = ((float)padData_.lX - 32767.0f);
	if (result > -7000.0f && result < 7000.0f) {
		return 0.0f;
	} else {
		result /= 32767.0f;
		return result;
	}
	return -1.0f;
}

float InputManager::gamepadDILStick01Y() {
	float result = -((float)padData_.lY - 32767.0f);
	if (result > -7000.0f && result < 7000.0f) {
		return 0.0f;
	} else {
		result /= 32767.0f;
		return result;
	}
	return -1.0f;
}

int InputManager::gamepadDILStickX() {
	return int(padData_.lX - 32767.0f);
}

int InputManager::gamepadDILStickY() {
	return int(padData_.lY - 32767.0f);
}

float InputManager::gamepadDIRStick01X() {
	float result = ((float)padData_.lZ - 32767.0f);
	if (result > -7000.0f && result < 7000.0f) {
		return 0.0f;
	} else {
		result /= 32767.0f;
		return result;
	}
	return -1.0f;
}

float InputManager::gamepadDIRStick01Y() {
	float result = -((float)padData_.lRz - 32767.0f);
	if (result > -7000.0f && result < 7000.0f) {
		return 0.0f;
	} else {
		result /= 32767.0f;
		return result;
	}
	return -1.0f;
}

int InputManager::gamepadDIRStickX() {
	return int(padData_.lRz - 32767.0f);
}

int InputManager::gamepadDIRStickY() {
	return int(padData_.lZ - 32767.0f);
}


float InputManager::gamepadDIL201() {
	float result = ((float)padData_.lRx);
	if (result > -7000.0f && result < 7000.0f) {
		return 0.0f;
	} else {
		result /= 65535.0f;
		return result;
	}
	return -1.0f;
}

float InputManager::gamepadDIR201() {
	float result = ((float)padData_.lRy);
	if (result > -7000.0f && result < 7000.0f) {
		return 0.0f;
	} else {
		result /= 65535.0f;
		return result;
	}
	return -1.0f;
}

int InputManager::gamepadDIL2() {
	return padData_.lRx;
}

int InputManager::gamepadDIR2() {
	return padData_.lRy;
}

#pragma endregion

bool InputManager::CheckIsGamepadConnet() {
	bool hasState = config::GetGamePadState();
	bool isXInput = CheckXInputDeviceConnected();
	bool isDInput = SUCCEEDED(driver_->GetDirectInput()->GetDeviceStatus(GUID_Joystick));

	if (hasState && !isDInput) {
		driver_->ReleaseDirectXInputGamepad();
		config::SetGamePadType(NODEVICE);
	}

	if (!isXInput) {
		if (!hasState && isDInput) {
			driver_->SetDirectXInputGamepad();
			config::SetGamePadType(DIRECTINPUT);
		}
	} else {
		config::SetGamePadType(XINPUT);
	}

	if (!isXInput && !isDInput) {
		bool boool = false;
		config::SetConnectGamePad(&boool);
		return false;
	}
	return true;
}

int InputManager::DIKeyChange(int input) {
	if (input == VK_PAD_A) { return 1; }	/// Button[00] = Direct(PS4): []
	if (input == VK_PAD_B) { return 2; }	/// Button[01] = Direct(PS4): X
	if (input == VK_PAD_X) { return 0; }	/// Button[02] = Direct(PS4): 0
	if (input == VK_PAD_Y) { return 3; }	/// Button[03] = Direct(PS4): A
	if (input == VK_PAD_RSHOULDER) { return 5; }	/// Button[04] = Direct(PS4): L1
	if (input == VK_PAD_LSHOULDER) { return 4; }	/// Button[05] = Direct(PS4): R1
	if (input == VK_PAD_LTRIGGER) { return 6; }	/// Button[06] = Direct(PS4): L2
	if (input == VK_PAD_RTRIGGER) { return 7; }	/// Button[07] = Direct(PS4): R2
	//if (input == ) { handle = VK_PAD_DPAD_UP; }		
	//if (input == ) { handle = VK_PAD_DPAD_DOWN; }	
	//if (input == ) { handle = VK_PAD_DPAD_LEFT; }	
	//if (input == ) { handle = VK_PAD_DPAD_RIGHT; }	
	if (input == VK_PAD_START) { return 9; }	/// Button[08] = Direct(PS4): Share
	if (input == VK_PAD_BACK) { return 8; }	/// Button[09] = Direct(PS4): Start
	if (input == VK_PAD_LTHUMB_PRESS) { return 10; }	/// Button[10] = Direct(PS4): 左ステック押し
	if (input == VK_PAD_RTHUMB_PRESS) { return 11; }	/// Button[11] = Direct(PS4): 右ステック押し
	if (input > 11) { return input; }
	return input;
}


int InputManager::XIKeyChange(int input) {
	if (input == VK_PAD_A) { return XINPUT_GAMEPAD_A; }							/// Button[00] = Direct(PS4): []
	if (input == VK_PAD_B) { return XINPUT_GAMEPAD_B; }							/// Button[01] = Direct(PS4): X
	if (input == VK_PAD_X) { return XINPUT_GAMEPAD_X; }							/// Button[02] = Direct(PS4): 0
	if (input == VK_PAD_Y) { return XINPUT_GAMEPAD_Y; }							/// Button[03] = Direct(PS4): A
	if (input == VK_PAD_RSHOULDER) { return XINPUT_GAMEPAD_RIGHT_SHOULDER; }	/// Button[04] = Direct(PS4): L1
	if (input == VK_PAD_LSHOULDER) { return XINPUT_GAMEPAD_LEFT_SHOULDER; }		/// Button[05] = Direct(PS4): R1
	//if (input == ) { return 4; }												/// Button[06] = Direct(PS4): L2
	//if (input == ) { return 5; }												/// Button[07] = Direct(PS4): R2
	if (input == VK_PAD_DPAD_UP) { return XINPUT_GAMEPAD_DPAD_UP; }
	if (input == VK_PAD_DPAD_DOWN) { return XINPUT_GAMEPAD_DPAD_DOWN; }
	if (input == VK_PAD_DPAD_LEFT) { return XINPUT_GAMEPAD_DPAD_LEFT; }
	if (input == VK_PAD_DPAD_RIGHT) { return XINPUT_GAMEPAD_DPAD_RIGHT; }
	if (input == VK_PAD_START) { return XINPUT_GAMEPAD_START; }	/// Button[08] = Direct(PS4): Share
	if (input == VK_PAD_BACK) { return XINPUT_GAMEPAD_BACK; }	/// Button[09] = Direct(PS4): Start
	if (input == VK_PAD_LTHUMB_PRESS) { return XINPUT_GAMEPAD_LEFT_THUMB; }	/// Button[10] = Direct(PS4): 左ステック押し
	if (input == VK_PAD_RTHUMB_PRESS) { return XINPUT_GAMEPAD_RIGHT_THUMB; }	/// Button[11] = Direct(PS4): 右ステック押し
	if (input > 11) { return input; }
	return input;
}

#pragma endregion

