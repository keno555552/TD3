#pragma once
#include "BaseScene.h"
#include <string>

/// <summary>
/// シーン工場（概念）,資料はAbstractSceneFactory名前に
/// </summary>
class BaseSceneFactory
{
public:
    /// 仮想デストラクタ
    virtual ~BaseSceneFactory() = default;

    /// シーン生成
    virtual BaseScene* CreateScene(const std::string& sceneName) = 0;
};

