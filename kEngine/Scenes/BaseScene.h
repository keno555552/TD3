#pragma once
#include "kEngine.h"
#include "SceneElement.h"



class SceneManager;
class BaseScene
{
public:

	virtual ~BaseScene() = default;

	ScenePhase phase_ = ScenePhase::PREPARE;

public:
	virtual void Update();
	virtual void Draw();

	ScenePhase GetScenePhase() const { return phase_; }
	void SetScenePhase(ScenePhase phase) { phase_ = phase; }
	
	SceneOutcome GetOutcome() const { return outcome_; }

public:

	bool operator == (const ScenePhase target) { return phase_ == target; }
	bool operator != (const ScenePhase target) { return phase_ != target; }

protected:

	void EndGame();

protected:
	kEngine* system_ = nullptr;

	SceneOutcome outcome_ = SceneOutcome::NONE;
	bool isSceneEnd_ = false;
};