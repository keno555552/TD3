#pragma once
#include <vector>

enum class LinkMode {
	PerBurst,
	PerParticle
};

enum class LinkFollow {
	Particle,
	Emitter
};

enum class EmitterTiming {
	SourceEmit,		// ソースエミット時
	TimeUp,			// ソースアップデート時
	SourceEnd 		// ソース終了時
};

struct EmitterLink {
	int sourceId{ -1 };											// A
	int targetId{ -1 };	
	int emitCount{ 1 };											// B
	float delayTime{};											// 遅延時間
	bool followPosition{ true };
	bool followRotation{ false };
	bool followScale{ false };

	LinkMode linkMode{ LinkMode::PerBurst };					// リンクの更新頻度
	LinkFollow linkFollow{ LinkFollow::Emitter };				// 追従対象
	EmitterTiming emitterTiming{ EmitterTiming::SourceEmit };	// 連動するタイミング
};