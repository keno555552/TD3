#pragma once
#include <memory>
#include <string>
#include "BaseScene.h"

/// <summary>
/// シーン工場（概念）,資料はAbstractSceneFactory名前に
/// </summary>
class BaseSceneFactory
{
public:
    /// 仮想デストラクタ
    virtual ~BaseSceneFactory() = default;

    /// シーン生成
    virtual std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) = 0;
};

