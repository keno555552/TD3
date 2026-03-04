#pragma once
#include "BaseScene.h"

class Menu :public BaseScene
{
	public:
	Menu(kEngine* system);

private:
	kEngine* system_ = nullptr;				// 借り

};

