#include "LightingType.h"

std::string GetLightingTypeMacro(LightingType model) {
    switch (model) {
    case LightingType::DirectionalLight:           return "L_DIRECTIONAL_LIGHT";
    case LightingType::SpotLight:                  return "L_SPOT_LIGHT";
    case LightingType::PointLight:                 return "L_POINT_LIGHT";
    default:                                       return "L_DEFAULT";
    }
}