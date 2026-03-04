#include "LightModelType.h"

std::string GetLightModelMacro(LightModelType model) {
    switch (model) {
    case LightModelType::Lambert:               return "LM_LAMBERT";
    case LightModelType::HalfLambert:           return "LM_HALF";
	case LightModelType::PhongReflection:       return "LM_Phong";
    case LightModelType::BlinnPhongReflection:  return "LM_BLINNPHONG";
    default:                                    return "LM_DEFAULT";
    }
}