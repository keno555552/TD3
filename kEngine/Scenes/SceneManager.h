#pragma once
#include "Data/Render/CPUData/MaterialConfig.h"
#include "SceneFactory.h"
#include "BaseScene.h"
#include "AnimationSystem/AnimationEditor.h"
#include "DefaultMenu/DefaultMenu.h"
#include "CG4_HK_1/Effect2.h"
#include "CG3_HK_2/SceneCGHK2.h"

class SceneManager {
public:

	static SceneManager& GetInstance();

	class ConstructorKey {
	private:
		/// からのみ生成・破棄可能
		friend class SceneManager;
		friend class kEnigne;
		ConstructorKey() {}
	};

	explicit SceneManager(ConstructorKey) {};

	void Initialize(kEngine* system);
	void Finalize();

	/// 複製禁止
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;
	SceneManager(SceneManager&&) = delete;
	SceneManager& operator=(SceneManager&&) = delete;

	void Update();
	void Render();
	;

public:
	std::string sceneUsingNameHandle_ = "NONE";


	bool IsPause() const { return defaultMenu_ && defaultMenu_->GetIsPause(); }

	void SceneChanger();

private:

	/// シーンの遷移の流れを管理するマップ
	std::map<std::string, std::map<SceneOutcome, std::string>> sceneFlow_ =
	{
		{"TITLE",
			{{ SceneOutcome::NEXT, "PROMPT" }}
		},
		{"PROMPT",
			{
				{ SceneOutcome::NEXT, "MOD" }
			}
		},
		{"MOD", 
			{ 
				{ SceneOutcome::NEXT, "TRAVEL" },
				{ SceneOutcome::RETURN_PROMPT, "PROMPT" },
			}
		},
		{"TRAVEL", 
			{ 
				{ SceneOutcome::NEXT,				"CONTEST" },
				{ SceneOutcome::RETURN_PROMPT,		"PROMPT" },
				{ SceneOutcome::RETRY_MOD,			"MOD" },
			}
		},
		{"CONTEST", 
			{ 
				{ SceneOutcome::NEXT, "PROMPT" } 
			}
		},
	};

private:
	friend struct std::default_delete<SceneManager>;
	~SceneManager() = default;

private:
	static std::unique_ptr <SceneManager> sceneManager_;


private:
	std::unique_ptr <BaseScene> sceneUsing_ = nullptr;
	std::unique_ptr <BaseScene> sceneOld_ = nullptr;

private:
	kEngine* system_ = nullptr; // 借り

	std::unique_ptr <SceneFactory> sceneFactory_ = nullptr;

	std::unique_ptr <DefaultMenu> defaultMenu_ = nullptr;

	/// ========= リソースハンドル ========= ///

  // int helperTextureHandle_ = 0;
	int pauseTextureHandle_ = 0;

	/// ============ オブジェクト =========== ///

  // SimpleSprite *helperSprite_ = nullptr;
	std::unique_ptr<SimpleSprite> pauseSprite_ = nullptr;
	// Vector2 helperSpriteScale_ = {1.0f, 1.0f};
	// Vector2 helperSpritePos_ = {50.0f, 50.0f};

	  /// =============== ステージ管理 ===============///

	bool stage[10]{};

	bool isFirst_ = true;

	bool isFromTitle_ = true;

	bool isReset_ = false;

	bool isEnd_ = false;

private:
	/// ============= ステージマネージメント ============///
	void ClearStage();

#ifdef USE_IMGUI
	void ImGuiPart();
#endif
};
