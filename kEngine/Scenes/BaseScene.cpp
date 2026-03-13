#include "BaseScene.h"


void BaseScene::Update() {
}

void BaseScene::Draw() {
}

void BaseScene::EndGame() {
	outcome_ = SceneOutcome::EXIT;
}
