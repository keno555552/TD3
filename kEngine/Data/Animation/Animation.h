#pragma once
#include <vector>
#include <string>
#include "Data/Animation/AnimationNodeData.h"


struct Animation {

	/// アニメーションのID
	int animationID{};

	/// アニメーションの名前
	std::string name;

	/// アニメーションの開始時間(秒)
	float startTime{};

	/// アニメーションの長さ(秒)
	float duration{};

	/// アニメーションのノードリスト(メーン資料)
	std::vector<AnimationNodeData> nodeList;

};
