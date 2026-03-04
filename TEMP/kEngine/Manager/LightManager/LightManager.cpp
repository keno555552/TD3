#include "LightManager.h"

void LightManager::Initialize() {
	// デフォルトライト設定
	LightGPU defaultLight{};
	defaultLight.type = (int)LightingType::DirectionalLight;
	defaultLight.color = Vector3(1.0f, 1.0f, 1.0f);
	defaultLight.direction = Vector3(-0.5f, -0.5f, 0.4f);
	defaultLight.intensity = 0.5f;
	for (int i = 0; i < config::GetMaxLightNum(); i++) {
		defaultLightGPUList_.push_back(defaultLight);
	}
}

void LightManager::Finalize() {
	lightList_.clear();
	lightGPUList_.clear();
	defaultLightGPUList_.clear();
}

void LightManager::AddLight(Light* light) {
	if (!light) {
		Logger::Log("[kError] LM:: null light!");
		return;
	}

	if (lightList_.size() < config::GetMaxLightNum()) {
		lightList_.push_back(light);
	} else {
		Logger::Log("[kError] LM:: Exceeded maximum number of lights!");
	}
}

void LightManager::ClearAllLights() {
	lightList_.clear();
}

void LightManager::RemoveLight(Light* light) {
	auto it = std::find(lightList_.begin(), lightList_.end(), light);
	if (it != lightList_.end()) {
		lightList_.erase(it);
	}
}

const std::vector<LightGPU>& LightManager::GetGPUBuffer() const {
	if (!lightGPUList_.empty()) {
		return lightGPUList_;
	}
	//Logger::Log("[kError] LM:: There is no lighting!");
	return defaultLightGPUList_;
}

void LightManager::TurnDataToGPUData() {
	// GPU listを
	lightGPUList_.clear();

	// nullptrを削除
	lightList_.erase(
		std::remove(lightList_.begin(), lightList_.end(), nullptr),
		lightList_.end()
	);

	// 光源がない場合は処理しない
	if (lightList_.empty()) return;

	// Light to LightGPU
	for (auto& light : lightList_) {

		//if (!light->ableLight)continue;

		if (light->lightingType == LightingType::DirectionalLight) {
			ConvertDirectionalLight(static_cast<DirectionalLight*>(light));
		} else if (light->lightingType == LightingType::PointLight) {
			ConvertPointLight(static_cast<PointLight*>(light));
		} else if (light->lightingType == LightingType::SpotLight) {
			ConvertSpotLight(static_cast<SpotLight*>(light));
		} else if (light->lightingType == LightingType::AreaLight) {
			ConvertAeraLight(static_cast<AreaLight*>(light));
		}
	}
}

int LightManager::GetLightCount() {
	if (lightGPUList_.empty()) {
		return 1;
	}
	return (int)lightGPUList_.size();
}

void LightManager::ConvertDirectionalLight(DirectionalLight* light) {
	LightGPU lightGPU{};
	lightGPU.type = (int)LightingType::DirectionalLight;
	lightGPU.color = light->color;
	lightGPU.direction = light->direction;
	lightGPU.intensity = light->ableLight ? light->intensity : 0;
	lightGPUList_.push_back(lightGPU);
}

void LightManager::ConvertPointLight(PointLight* light) {
	LightGPU lightGPU{};
	lightGPU.type = (int)LightingType::PointLight;
	lightGPU.position = light->position;
	lightGPU.range = light->range;
	lightGPU.color = light->color;
	lightGPU.intensity = light->ableLight ? light->intensity : 0;
	lightGPUList_.push_back(lightGPU);
}

void LightManager::ConvertSpotLight(SpotLight* light) {
	LightGPU lightGPU{};
	lightGPU.type = (int)LightingType::SpotLight;
	lightGPU.range = light->range;
	lightGPU.position = light->position;
	lightGPU.angle = light->angle;
	lightGPU.color = light->color;
	lightGPU.direction = light->direction;
	lightGPU.intensity = light->ableLight ? light->intensity : 0;
	lightGPUList_.push_back(lightGPU);
}


void LightManager::ConvertAeraLight(AreaLight* light) {

	// --- Step 1: Orthonormalize right / up ---
	Vector3 right = Normalize(light->right);

	// 去掉 up 與 right 的平行成分（Gram-Schmidt）
	Vector3 up = light->up - right * Dot(light->up, right);
	up = Normalize(up);

	// --- Step 2: Apply width / height ---
	Vector3 halfRight = right * (light->width * 0.5f);
	Vector3 halfUp = up * (light->height * 0.5f);

	// --- Step 3: Compute 4 sample points ---
	Vector3 p0 = light->position + halfRight + halfUp;
	Vector3 p1 = light->position - halfRight + halfUp;
	Vector3 p2 = light->position + halfRight - halfUp;
	Vector3 p3 = light->position - halfRight - halfUp;

	float perIntensity = light->intensity * 0.25f;

	PointLight temp{};
	temp.color = light->color;
	temp.range = light->range;
	temp.intensity = perIntensity;
	temp.ableLight = light->ableLight;

	temp.position = p0; ConvertPointLight(&temp);
	temp.position = p1; ConvertPointLight(&temp);
	temp.position = p2; ConvertPointLight(&temp);
	temp.position = p3; ConvertPointLight(&temp);
}
