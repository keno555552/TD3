#pragma once
#include <vector>

#include "config.h"
#include "Logger.h"
#include "Light.h"
#include "Data/LightGPU.h"
#include "LightingType.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "AeraLight.h"


class LightManager {
public:
    static constexpr int MaxLights = 8;

	void Initialize();
	void Finalize();

    void AddLight(Light* light);

    void ClearAllLights();

	void RemoveLight(Light* light);

    const std::vector<LightGPU>& GetGPUBuffer() const;

    void TurnDataToGPUData();

    int GetLightCount();

private:
    std::vector<Light*> lightList_;
    std::vector<LightGPU> lightGPUList_;
	std::vector<LightGPU> defaultLightGPUList_;

private:

	/// 各ライトタイプ変換関数
    void ConvertDirectionalLight(DirectionalLight* light);
	void ConvertPointLight(PointLight* light);
	void ConvertSpotLight(SpotLight* light);
	void ConvertAeraLight(AreaLight* light);
};

/// 問題
/// GetGPUBuffer() 回傳 vector「複製」而不是 reference
/// 問題 B：TurnDataToGPUData() 在 lightList_ 空時直接 return


