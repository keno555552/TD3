#include "MaterialConfig.h"


bool MaterialConfig::operator==(const MaterialConfig& target)const {
	if (useModelTexture != target.useModelTexture)return false;
	if (textureHandle != target.textureHandle)return false;
	if (uvTranslate != target.uvTranslate)return false;
	if (uvScale != target.uvScale)return false;
	if (uvRotate != target.uvRotate)return false;
	if (textureColor != target.textureColor)return false;
	if (lightModelType != target.lightModelType)return false;
	if (enableLighting != target.enableLighting)return false;
	if (textureColor2 != target.textureColor2)return false;
	if (textureColor3 != target.textureColor3)return false;
	if (shininess != target.shininess)return false;
	if (intensity != target.intensity)return false;
	if (heightScale != target.heightScale)return false;
	return true;
}

bool MaterialConfig::operator!=(const MaterialConfig& target) const {
	return !(*this == target);
}

void MaterialConfig::MakeUVMatrix() {
	uvTransformMatrix = MakeAffineMatrix(uvScale, uvRotate, uvTranslate);
}

void InitMaterialConfig(MaterialConfig* materialConfig) {
	materialConfig->useModelTexture = true;
	materialConfig->textureHandle = 0;
	materialConfig->uvTranslate = { 0,0,0 };
	materialConfig->uvScale = { 1,1,1 };
	materialConfig->uvRotate = { 0,0,0 };
	materialConfig->uvTransformMatrix = {};
	materialConfig->textureColor = { 1.0f,1.0f,1.0f,1.0f };
	materialConfig->lightModelType = LightModelType::HalfLambert;
	materialConfig->enableLighting = true;
	materialConfig->materialResourceHandle = -1;
	materialConfig->drawState = 0;
	materialConfig->textureColor2 = { 1.0f,1.0f,1.0f,1.0f };
	materialConfig->textureColor3 = { 1.0f,1.0f,1.0f,1.0f };
	materialConfig->shininess = 32.0f;
	materialConfig->intensity = 1.0f;
	materialConfig->heightScale = 2.0f;
}

MaterialConfig InitMaterialConfig() {
	MaterialConfig newMaterialConfig;
	InitMaterialConfig(&newMaterialConfig);
	return newMaterialConfig;
}
