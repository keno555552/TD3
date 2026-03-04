#pragma once
#include "externals/nlohmann/json.hpp"
#include "../StageManager/BaseScene.h"
#include "config.h"
#include "Object/Sprite.h"
#include "Object/Object.h"
#include "tool/crashDecision/crashDecision.h"
#include "DebugCamera.h"
#include "AnimationSystem.h"
#include "AnimationUnit.h"

class AnimationEditor : public BaseScene
{
public:
	AnimationEditor(kEngine* system);
	~AnimationEditor();

	void Update()override;
	void Draw()override;



private:
	/// ============ Editorセッティング ===========///
	const float startMinTime_ = 0.0f;
	const float startMaxTime_ = 10.0f;


private:
	kEngine* system_ = nullptr;				// 借り


	/// ========== UIテキスチャーハンドル ==========///
	int mainNeedleTH_{};
	int mainTimeBarTH_{};
	int markerTH_{};
	int pingTH_{};
	int pingFirstTH_{};
	int pingLastTH_	{};

	/// ============= UIオブジェクト =============///
	/// スカイドームモデルハンドル
	int Skydome_modelHandle_{};

	/// スカイドームオブジェクト
	Object* skydome_{};	

	/// Time Bar関連スプライト
	SimpleSprite* mainNeedle_{ new SimpleSprite };
	SimpleSprite* mainTimeBar_{ new SimpleSprite };
	SimpleSprite* markerStartEnd_{ new SimpleSprite };
	SimpleSprite* marker10_{ new SimpleSprite };
	SimpleSprite* marker02_{ new SimpleSprite };
	SimpleSprite* ping_{ new SimpleSprite };

	/// ============= 選んでるモデル =============///
	Object* instanceModel_{};	/// インスタンスモデル
	Object* keyFrameModel_{};	/// キーフレームモデル // 借り
	Object* choosingModel_{};	/// 選んでるモデル　　 // 借り

	/// ============== カメラ関連 ==============///
	Camera* camera_{};

	/// ============= バー用定数 ===============///

	Vector2 startPos_{ 50.0f,(float)config::GetClientHeight() - 50.f };
	Vector2 EndPos_{ (float)config::GetClientWidth() - 50.0f,(float)config::GetClientHeight() - 50.f };
	Vector2 NowPos_{};

	/// マーカー用色
	Vector4 markerColor_ { 0.0f,0.0f,0.0f,1.0f };
	Vector4 markerColor2_{ 0.2f,0.2f,0.2f,1.0f };

	Vector4 saveMarkerColor_{ 0.0f,0.0f,0.0f,1.0f };
	Vector4 saveMarkerColor2_{ 0.2f,0.2f,0.2f,1.0f };
	/// ============== タイマー ===============///
	Timer* mainTimer_{};
	float saveMaxTime_{};

	/// =============== フラグ ===============///
	bool isPlay_{ false };
	bool isLoop_{ false };
	bool isPickKeyFrame_{ false };
	bool isChooseKeyFrame_{ false };

	/// =========== ヒットボックス ============///
	HitBox mainTimeBarHitBox_{};

	/// ======== アニメーション内部用 ==========///
	/// キーフレームリスト
	std::vector<AnimationObjectData> animationList_{};
	std::vector<KeyFrame> keyFrameList_{};

	/// 選んでるキーフレーム(by IP)
	int pickedKeyFrame_{-1};
	int selectedKeyFrame_{ -1 };

	/// ============== セーブ用 ==============///

	std::string saveFilePath_{ "resources/animationData/NewData.json" };
	std::string loadFilePath_{ "resources/animationData/NewData.json" };

	///////////////// 使うどころ /////////////////
	/// ============ モデルハンドル ===========///
	/// 3Dオブジェク
	int targetModelHandle_{};

	/// =============== モデル ==============///

	/// 3Dオブジェク
	Object* targetModel_{};

	/// ============ アニメーションユニット ==========///
	AnimationUnit* animationUnit_{};


	///////////////// 使うどころ /////////////////


	/// ============== テスト用 ==============///

private:
	/// ============== 内部処理 ==============///
	/// AnimationEditorの初期化
	void SetupAnimationEditor();
	void SetupAnimationEditorEnd();
	void SetChooseModel(Object* model);

	/// 集合命令
	void UIUpdate();
	void UIDraw();

	/// メイン処理(UIUpdateの内部関数)
	void TimerPart();
	void NeedlePart();
	void MarkerPart();
	void PingPart();

	/// UIUpdate内部のmouse調整関数
	void mouseMovement();

	/// markerUpdate内部のmarker調整関数
	void AdjustMarker();
	void KeyFrameTurnning();

	/// キーフレーム関連関数
	void CreateKeyFrame(float time_ = -1);
	void DeleteKeyFrame(KeyFrame* keyFrame = nullptr);
	void SortKeyFrame();
	void AdjuctKeyFrameTexture();

	/// モデル関連関数
	void SetUsingModel(Object* model);

	/// セーブ/ロード関連関数
	void SaveAnimationData(const AnimationObjectData& animationList, const std::string& filePath);
	void LoadAnimationData(AnimationObjectData* animationList, const std::string& filePath);

#ifdef USE_IMGUI
	void ImguiPart();
#endif

};