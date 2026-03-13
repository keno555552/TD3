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
	~DefaultMenu();

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
	std::unique_ptr<SimpleSprite> sMenuBG_ = std::make_unique <SimpleSprite>();
	
	std::unique_ptr<SimpleSprite> sMenuPause = std::make_unique <SimpleSprite>();
	std::unique_ptr<SimpleSprite> sMenuBGM = std::make_unique <SimpleSprite>();
	std::unique_ptr<SimpleSprite> sMenuBGM_NL = std::make_unique <SimpleSprite>();
	std::unique_ptr<SimpleSprite> sMenuMASTER = std::make_unique <SimpleSprite>();
	std::unique_ptr<SimpleSprite> sMenuMASTER_NL = std::make_unique <SimpleSprite>();
	std::unique_ptr<SimpleSprite> sMenuSE = std::make_unique <SimpleSprite>();
	std::unique_ptr<SimpleSprite> sMenuSE_NL = std::make_unique <SimpleSprite>();
	
	std::unique_ptr<SimpleSprite> sMenuClose = std::make_unique <SimpleSprite>();
	std::unique_ptr<SimpleSprite> sMenuClose_NL = std::make_unique <SimpleSprite>();
	std::unique_ptr<SimpleSprite> sMenuBack = std::make_unique <SimpleSprite>();
	std::unique_ptr<SimpleSprite> sMenuBack_NL = std::make_unique <SimpleSprite>();
	std::unique_ptr<SimpleSprite> sMenuRetry = std::make_unique <SimpleSprite>();
	std::unique_ptr<SimpleSprite> sMenuRetry_NL = std::make_unique <SimpleSprite>();
	
	std::unique_ptr<SimpleSprite> sMenuButtonM = std::make_unique <SimpleSprite>();
	std::unique_ptr<SimpleSprite> sMenuButtonM_NL = std::make_unique <SimpleSprite>();
	std::unique_ptr<SimpleSprite> sMenuButtonB = std::make_unique <SimpleSprite>();
	std::unique_ptr<SimpleSprite> sMenuButtonB_NL = std::make_unique <SimpleSprite>();
	std::unique_ptr<SimpleSprite> sMenuButtonS = std::make_unique <SimpleSprite>();
	std::unique_ptr<SimpleSprite> sMenuButtonS_NL = std::make_unique <SimpleSprite>();
	std::unique_ptr<SimpleSprite> sMenuVolumeBarM = std::make_unique <SimpleSprite>();
	std::unique_ptr<SimpleSprite> sMenuVolumeBarB = std::make_unique <SimpleSprite>();
	std::unique_ptr<SimpleSprite> sMenuVolumeBarS = std::make_unique <SimpleSprite>();

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

	std::unique_ptr <Timer> animationTimer_ = nullptr;
	std::unique_ptr <Timer> buttonTimer_ = nullptr;

	Transform startTransform_{};

	int selectedMenuIndex_ = (int)ButtonIndex::Close;
	int lastSelectedMenuIndex_ = (int)ButtonIndex::Retry;

	Vector4* buttonsOnColor_[6] = {};						// 借り

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

