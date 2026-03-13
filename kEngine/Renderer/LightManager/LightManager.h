#pragma once
#include <vector>

#include "config.h"
#include "Logger.h"
#include "Lighting/Light.h"
#include "GPUData/LightGPU.h"
#include "Data/Render/Types/LightingType.h"
#include "Lighting/DirectionalLight.h"
#include "Lighting/PointLight.h"
#include "Lighting/SpotLight.h"
#include "Lighting/AeraLight.h"


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

/// ここのListに収納するのは本体じゃなく、Lightのポインタ、だからそのままのLight*でいい


