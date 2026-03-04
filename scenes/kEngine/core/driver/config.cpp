#include "config.h"

/// インスタンス設定
std::string config::clientTitle_{};
int config::clientWidth_ = 0;
int config::clientHeight_ = 0;

bool config::isConnectKeyboard_ = false;
bool config::isConnectMouse_ = false;
bool config::isConnectGamePad_ = false;
gamepadType config::gamePadType_ = NODEVICE;
int config::XInputDriverIP_ = -1;

int config::default_Plane_MeshBufferHandle_     = 0;
int config::default_Triangle_MeshBufferHandle_	= 0;
int config::default_Sprite2D_MeshBufferHandle_	= 0;
int config::default_Cube_MeshBufferHandle_		= 0;
int config::default_Sphere_MeshBufferHandle_	= 0;

void config::SaveClientTitle(const std::string& clientTitle) {

    ////// 旧資料を消す
    ///if (!clientTitle_.empty()) {
    ///    clientTitle_.clear();
    ///}

	/// 文字列コピー
	clientTitle_ = clientTitle;
}

