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
	static const int kDebugLineNumInstance_ = 5000;   /// DebugLineの最大インスタンス数
	static const int k2DTileNumInstance_ = 50000;     /// Tileの最大インスタンス数
	static const int k3DTileNumInstance_ = 10000;     /// Tileの最大インスタンス数
	static const int kParticleNumInstance_ = 100000;  /// Particleの最大インスタンス数

	/// 最大マテリアル数設定
	static const int kMaxMaterialNum_ = 10000;        /// 最大マテリアル数

	/// 最大SRV数(最大テキスチャ枚数)
	static const int kMaxSRVNum_ = 512;               /// 最大SRV数(最大テキスチャ枚数)

	/// 最大ライト数設定
	static const int kMaxLightNum_ = 50;              /// 最大ライト数

private:

	/// ============================= mesh関連 ============================= ///
	/// Sphereの分割数
	static const int kDefaultSphereDivideNum_ = 18;			/// Sphereの分割数



private:
	static std::string clientTitle_;
	static int clientWidth_;
	static int clientHeight_;

	static std::string psoCacheFolderPath_;           /// PSOキャッシュの保存先
	static std::string psoCacheFolderName_;           /// PSOキャッシュの保存先

	static bool isConnectKeyboard_;
	static bool isConnectMouse_;
	static bool isConnectGamePad_;
	static gamepadType gamePadType_;
	static int XInputDriverIP_;

public:
	static int GetDebugLineNumInstance() { return kDebugLineNumInstance_; }
	static int Get2DTileNumInstance() { return k2DTileNumInstance_; }
	static int Get3DTileNumInstance() { return k3DTileNumInstance_; }
	static int GetParticleNumInstance() { return kParticleNumInstance_; }
	static int GetMaxMaterialNum() { return kMaxMaterialNum_; }
	static int GetMaxSRVNum() { return kMaxSRVNum_; }
	static int GetMaxLightNum() { return kMaxLightNum_; }

	static int GetDefaultSphereDivideNum() { return kDefaultSphereDivideNum_; }

	static void SaveClientTitle(const std::string& clientTitle) { clientTitle_ = clientTitle; };
	static const std::string GetClientTitle() { return clientTitle_; }
	static void SaveClientWidth(int clientWidth) { clientWidth_ = clientWidth; }
	static int GetClientWidth() { return clientWidth_; }
	static void SaveClientHeight(int clientHeight) { clientHeight_ = clientHeight; }
	static int GetClientHeight() { return clientHeight_; }

	static void SavePsoCacheFolderPath(const std::string& path) { psoCacheFolderPath_ = path; }
	static std::string GetPsoCacheFolderPath() { return psoCacheFolderPath_; }
	static void SavePsoCacheFolderName(const std::string& name) { psoCacheFolderName_ = name; }
	static std::string GetPsoCacheFolderName() { return psoCacheFolderName_; }

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
	static int default_SkyCube_MeshBufferHandle_;
	static int default_Ring_MeshBufferHandle_;
	static int default_Cylinder_MeshBufferHandle_;

public:
	/// デフォルトPSoハンドル
	static int default_LightModel_;
	static int default_RenderModelTypes_;
	static int default_RasterizerMode_;
	static int default_BlendMode_;
	static int default_DepthStenctilState_;
	static int default_RenderTargetFormatType_;
};
