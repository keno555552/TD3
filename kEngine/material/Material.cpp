#include "Material.h"

bool Material::operator==(const Material target) {
	if (color != target.color)return false;
	if (color2 != target.color2)return false;
	if (color3 != target.color3)return false;

	if (enableLighting != target.enableLighting)return false;
	if (uvTransform != target.uvTransform)return false;
	if (shininess != target.shininess)return false;
	if (intensity != target.intensity)return false;
	if (heightScale != target.heightScale)return false;
	return true;
}
bool Material::operator!=(const Material target) {
	return !(*this == target);
}

void Material::inputMaterialConfig(MaterialConfig target) {
	color = target.textureColor;
	color2 = target.textureColor2;
	color3 = target.textureColor3;
	enableLighting = target.enableLighting;
	uvTransform = target.uvTransformMatrix;
	shininess = target.shininess;
	intensity = target.intensity;
	heightScale = target.heightScale;
}
