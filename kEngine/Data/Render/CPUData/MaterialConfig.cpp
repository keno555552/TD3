#include "MaterialConfig.h"


bool MaterialConfig::operator==(const MaterialConfig& target)const {
	if (useModelTexture != target.useModelTexture)return false;
	if (textureHandle != target.textureHandle)return false;

	if (uvTranslate != target.uvTranslate)return false;
	if (uvScale != target.uvScale)return false;
	if (uvRotate != target.uvRotate)return false;
	if (textureColor != target.textureColor)return false;

	if (lightModelType != target.lightModelType)return false;
	if (renderModelType != target.renderModelType)return false;
	if (blendModeType != target.blendModeType)return false;
	if (rasterizerMode != target.rasterizerMode)return false;
	if (depthStencilType != target.depthStencilType)return false;

	if (enableLighting != target.enableLighting)return false;
	if (reflectiveStrength != target.reflectiveStrength)return false;

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

	LightModelType lightModelType = (LightModelType)config::default_LightModel_;
	RenderModelType renderModelType = (RenderModelType)config::default_RenderModelTypes_;
	BlendModeType blendModeType = (BlendModeType)config::default_BlendMode_;
	RasterizerMode rasterizerMode = (RasterizerMode)config::default_RasterizerMode_;
	DepthStencilType depthStencilType = (DepthStencilType)config::default_DepthStenctilState_;

	materialConfig->reflectiveStrength = 0.0f;

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


/// ========================= 便利PSO関連生成関数 ========================= ///

void MaterialConfig::MakePSOParticle() {

	lightModelType = LightModelType::BlinnPhongReflection;
	renderModelType = RenderModelType::Static;
	blendModeType = BlendModeType::AddBlend;
	rasterizerMode = RasterizerMode::CullBack;
	depthStencilType = DepthStencilType::ReadOnly;

}

void MaterialConfig::MakePSOEnvironment() {

	lightModelType = LightModelType::Environment;
	renderModelType = RenderModelType::Environment;
	blendModeType = BlendModeType::AlphaBlend;
	rasterizerMode = RasterizerMode::CullBack;
	depthStencilType = DepthStencilType::ReadOnly;

}

void MaterialConfig::MakePSODebugLine() {

	lightModelType = LightModelType::DebugLine;
	renderModelType = RenderModelType::DebugLine;
	blendModeType = BlendModeType::AlphaBlend;
	rasterizerMode = RasterizerMode::CullBack;
	depthStencilType = DepthStencilType::Disable;

}
