#pragma once
#include <vector>
#include <string>

enum class ScenePhase {
	PREPARE,
	ENTRY,
	PROGRESS,
	TRANSITION,
};

enum class SceneOutcome {
  LOSE = -1,
  NONE,
  WIN,
  NEXT,
  RETRY,
  RETRY_MOD,
  RETURN,
  RETURN_PROMPT,
  EXIT,
};