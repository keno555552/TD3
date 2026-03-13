#pragma once
#include <string>

enum gamepadType {
	NODEVICE,
	DIRECTINPUT,
	XINPUT
};

class config
{
private:
	/// 描画設定
	static const int k2DTileNumInstance_ = 50000;     /// Tileの最大インスタンス数
	static const int k3DTileNumInstance_ = 10000;     /// Tileの最大インスタンス数
	static const int kParticleNumInstance_ = 100000;  /// Particleの最大インスタンス数

	/// 最大マテリアル数設定
	static const int kMaxMaterialNum_ = 10000;          /// 最大マテリアル数

	/// 最大SRV数(最大テキスチャ枚数)
	static const int kMaxSRVNum_ = 512;              /// 最大SRV数(最大テキスチャ枚数)

	/// 最大ライト数設定
	static const int kMaxLightNum_ = 50;              /// 最大ライト数

private:
	static std::string clientTitle_;
	static int clientWidth_;
	static int clientHeight_;

	static bool isConnectKeyboard_;
	static bool isConnectMouse_;
	static bool isConnectGamePad_;
	static gamepadType gamePadType_;
	static int XInputDriverIP_;

public:
	static int Get2DTileNumInstance() { return k2DTileNumInstance_; }
	static int Get3DTileNumInstance() { return k3DTileNumInstance_; }
	static int GetMaxMaterialNum() { return kMaxMaterialNum_; }
	static int GetMaxSRVNum() { return kMaxSRVNum_; }
	static int GetMaxLightNum() { return kMaxLightNum_; }

	static void SaveClientTitle(const std::string& clientTitle);
	static const std::string GetClientTitle() { return clientTitle_; }
	static void SaveClientWidth(int clientWidth) { clientWidth_ = clientWidth; }
	static int GetClientWidth() { return clientWidth_; }
	static void SaveClientHeight(int clientHeight) { clientHeight_ = clientHeight; }
	static int GetClientHeight() { return clientHeight_; }


	static void SetConnectKeyboard(bool* state) { isConnectKeyboard_ = *state; }
	static bool GetKeyboardState() { return isConnectKeyboard_; }
	static void SetConnectMouse(bool* state) { isConnectMouse_ = *state; }
	static bool GetMouseState() { return isConnectMouse_; }
	static void SetConnectGamePad(bool* state) { isConnectGamePad_ = *state; }
	static bool GetGamePadState() { return isConnectGamePad_; }
	static void SetGamePadType(gamepadType Type) { gamePadType_ = Type; }
	static gamepadType GetGamePadType() { return gamePadType_; }
	static void SetXInputIp(int IP) { XInputDriverIP_ = IP; }
	static int GetXInputIp() { return XInputDriverIP_; }

public:
	/// デフォルトメッシュバッファハンドル
	static int default_Plane_MeshBufferHandle_;
	static int default_Triangle_MeshBufferHandle_;
	static int default_Sprite2D_MeshBufferHandle_;
	static int default_Cube_MeshBufferHandle_;
	static int default_Sphere_MeshBufferHandle_;
};
