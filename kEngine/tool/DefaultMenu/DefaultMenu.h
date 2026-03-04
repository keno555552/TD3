#pragma once
#include "kEngine.h"
#include "Object/Sprite.h"
#include "config.h"

enum class MenuPhase {
	ENTRY,
	PROGRESS,
	TRANSITION,
	EXIT,
};

enum class ButtonIndex {
	Close,
	Retry,
	Back,
	VolumeSE,
	VolumeBGM,
	VolumeMASTER,
};

class DefaultMenu
{
public:
	DefaultMenu(kEngine* system);

	void Updata();
	void Draw();

	bool GetIsPause() const { return isPause_; }

	void SetCanOpen(bool state) { canOpen_ = state; }
	bool GetCanOpen() const { return canOpen_; }
	int GetSelectedIndex() const { return selectedMenuIndex_; }
	bool isClicked() const { return isConfirm_; }

	bool IsRetry();
	bool IsBack();

private:
	/// ============ メニューConfig ============///

	float animationDefaultTime_ = 0.5f;
	float buttonAnimationDefaultTime_ = 0.2f;
	float menuWidth_ = 800.0f;
	float menuHeight_ = 500.0f;
	Vector2 menuStartPos_ = { ((float)config::GetClientWidth() - menuWidth_) / 2.0f,
								-550.0f };

	Vector2 menuNormalPos_ = { ((float)config::GetClientWidth() - menuWidth_) / 2.0f,
							   ((float)config::GetClientHeight() - menuHeight_) / 2.0f };

	float volumeButton0x_ = 195.0f;
	float volumeButton1x_ = 725.0f;

private:
	/// ========= テキスチャーハンドル =========///
	kEngine* system_ = nullptr;				// 借り


	/// =========== リソースハンドル ============///
	int TH_menuBG_ = 0;

	int	TH_menuPause = 0;
	int	TH_menuBGM = 0;
	int	TH_menuBGM_NL = 0;
	int	TH_menuMASTER = 0;
	int	TH_menuMASTER_NL = 0;
	int	TH_menuSE = 0;
	int	TH_menuSE_NL = 0;

	int	TH_menuClose = 0;
	int	TH_menuClose_NL = 0;
	int	TH_menuBack = 0;
	int	TH_menuBack_NL = 0;
	int	TH_menuRetry = 0;
	int	TH_menuRetry_NL = 0;

	int	TH_menuButton = 0;
	int	TH_menuButton_NL = 0;
	int	TH_menuVolumeBar = 0;

	// テスト用サウンドハンドル
	//int SH_menuSE_ = 0;
	//int SH_menuBGM_ = 0;

	/// ============ オブジェクト ============///
	/// 背景スプライト+mainPosition
	SimpleSprite* sMenuBG_ = new SimpleSprite;

	SimpleSprite* sMenuPause = new SimpleSprite;
	SimpleSprite* sMenuBGM = new SimpleSprite;
	SimpleSprite* sMenuBGM_NL = new SimpleSprite;
	SimpleSprite* sMenuMASTER = new SimpleSprite;
	SimpleSprite* sMenuMASTER_NL = new SimpleSprite;
	SimpleSprite* sMenuSE = new SimpleSprite;
	SimpleSprite* sMenuSE_NL = new SimpleSprite;

	SimpleSprite* sMenuClose = new SimpleSprite;
	SimpleSprite* sMenuClose_NL = new SimpleSprite;
	SimpleSprite* sMenuBack = new SimpleSprite;
	SimpleSprite* sMenuBack_NL = new SimpleSprite;
	SimpleSprite* sMenuRetry = new SimpleSprite;
	SimpleSprite* sMenuRetry_NL = new SimpleSprite;

	SimpleSprite* sMenuButtonM = new SimpleSprite;
	SimpleSprite* sMenuButtonM_NL = new SimpleSprite;
	SimpleSprite* sMenuButtonB = new SimpleSprite;
	SimpleSprite* sMenuButtonB_NL = new SimpleSprite;
	SimpleSprite* sMenuButtonS = new SimpleSprite;
	SimpleSprite* sMenuButtonS_NL = new SimpleSprite;
	SimpleSprite* sMenuVolumeBarM = new SimpleSprite;
	SimpleSprite* sMenuVolumeBarB = new SimpleSprite;
	SimpleSprite* sMenuVolumeBarS = new SimpleSprite;

	///	============= サント関連 ==============///
	float masterVolume_ = 1.0f;
	float BGMVolume_ = 1.0f;
	float SEVolume_ = 1.0f;

	/// =============== フラグ ===============///
	bool canOpen_ = true;
	bool isOpened_ = false;
	bool isPause_ = false;
	bool isConfirm_ = false;

	bool clickLeft_ = false;
	bool clickRight_ = false;
	bool clickUp_ = false;
	bool clickDown_ = false;
	bool clickDecide_ = false;
	
	MenuPhase phase_ = MenuPhase::EXIT;

	Timer* animationTimer_ = nullptr;
	Timer* buttonTimer_ = nullptr;

	Transform startTransform_{};

	int selectedMenuIndex_ = (int)ButtonIndex::Close;
	int lastSelectedMenuIndex_ = (int)ButtonIndex::Retry;

	Vector4* buttonsOnColor_[6] = {};

	/// ============== 内部処理 ==============///

	/// セレクト関係
	void ChangeSelect();
	void CheckClick();
	void WorkChange();
	void ChangeButtonLight();

	/// オブジェクト関係
	void MenuObjectUpdate();

	/// ============ Test =============///



#ifdef USE_IMGUI
	void ImguiPart();
#endif
};

