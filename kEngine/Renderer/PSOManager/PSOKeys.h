#pragma once
#include <cstdint>
#include <tuple>
#include <functional>
#include <type_traits>

#include "Data/Render/Types/PSOType/LightModelType.h"
#include "Data/Render/Types/PSOType/RenderModelType.h"
#include "Data/Render/Types/PSOType/RasterizerMode.h"
#include "Data/Render/Types/PSOType/BlendModeType.h"
#include "Data/Render/Types/PSOType/DepthStencilType.h"
#include "Data/Render/Types/PSOType/PrimitiveType.h"

#include "Data/Render/Types/PSOType/RenderTargetFormat.h"

#include "config.h"

enum FeatureFlags : uint64_t {
	None = 0,
	EnvReflection = 1 << 0,
	Shadow = 1 << 1,
	NormalMap = 1 << 2,
	Instancing = 1 << 3,
	IBL = 1 << 4,
	// ...
};

struct PSOKey {

	// Shader Feature
	LightModelType lightModelType = (LightModelType)config::default_LightModel_;

	// Render Model
	RenderModelType renderModelType = (RenderModelType)config::default_RenderModelTypes_;

	// Pipeline State
	BlendModeType blendModeType = (BlendModeType)config::default_BlendMode_;
	RasterizerMode rasterizerMode = (RasterizerMode)config::default_RasterizerMode_;
	DepthStencilType depthStencilType = (DepthStencilType)config::default_DepthStenctilState_;
	PrimitiveType primitiveType = PrimitiveType::TRIANGLE;

	// Render Target
	RenderTargetFormatType renderTargetFormatType = (RenderTargetFormatType)config::default_RenderTargetFormatType_;

	// FeatureFlags
	uint64_t featureMask{
		FeatureFlags::EnvReflection
	};

};

// 比較
inline bool operator==(const PSOKey& a, const PSOKey& b) {
	return

		a.lightModelType == b.lightModelType

		&& a.renderModelType == b.renderModelType

		&& a.blendModeType == b.blendModeType
		&& a.rasterizerMode == b.rasterizerMode
		&& a.depthStencilType == b.depthStencilType
		&& a.primitiveType == b.primitiveType

		&& a.renderTargetFormatType == b.renderTargetFormatType

		&& a.featureMask == b.featureMask;
}

// 比較
inline bool operator!=(const PSOKey& a, const PSOKey& b) {
	return !(a == b);
}

// 順序比較（std::map用）
inline bool operator<(const PSOKey& a, const PSOKey& b) {
	return std::tie(
		a.renderModelType,
		a.blendModeType,
		a.rasterizerMode,
		a.depthStencilType,
		a.primitiveType,
		a.featureMask,
		a.lightModelType,
		a.renderTargetFormatType
	) < std::tie(
		b.renderModelType,
		b.blendModeType,
		b.rasterizerMode,
		b.depthStencilType,
		b.primitiveType,
		b.featureMask,
		b.lightModelType,
		b.renderTargetFormatType
	);
}


// hash
template<>
struct std::hash<PSOKey> {
	size_t operator()(const PSOKey& key) const noexcept {
		size_t h = 0;
		auto hash_combine = [&h](auto v) {
			using T = decltype(v);
			if constexpr (std::is_enum_v<T>) {
				h ^= std::hash<std::underlying_type_t<T>>{}(
					static_cast<std::underlying_type_t<T>>(v)
					) + 0x9e3779b97f4a7c15ULL + (h << 7) + (h >> 2);
			} else {
				h ^= std::hash<T>{}(v)
					+0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
			}
			};
		hash_combine(key.lightModelType);

		hash_combine(key.renderModelType);

		hash_combine(key.blendModeType);
		hash_combine(key.rasterizerMode);
		hash_combine(key.depthStencilType);
		hash_combine(key.primitiveType);

		hash_combine(key.featureMask);
		hash_combine(key.renderTargetFormatType);
		return h;
	}
};

inline PSOKey CreateDefaultPSOKey() {
	PSOKey defaultKey;
	defaultKey.lightModelType = (LightModelType)config::default_LightModel_;
	defaultKey.renderModelType = (RenderModelType)config::default_RenderModelTypes_;
	defaultKey.blendModeType = (BlendModeType)config::default_BlendMode_;
	defaultKey.rasterizerMode = (RasterizerMode)config::default_RasterizerMode_;
	defaultKey.depthStencilType = (DepthStencilType)config::default_DepthStenctilState_;
	defaultKey.renderTargetFormatType = (RenderTargetFormatType)config::default_RenderTargetFormatType_;
	defaultKey.primitiveType = PrimitiveType::TRIANGLE;
	return defaultKey;
}

inline PSOKey CreateDebugLinePSOKey() {
	PSOKey debugLineKey;
	debugLineKey.lightModelType = LightModelType::DebugLine;
	debugLineKey.renderModelType = RenderModelType::DebugLine;
	debugLineKey.blendModeType = BlendModeType::AlphaBlend;
	debugLineKey.rasterizerMode = RasterizerMode::Wireframe;
	debugLineKey.depthStencilType = DepthStencilType::Disable;
	debugLineKey.renderTargetFormatType = (RenderTargetFormatType)config::default_RenderTargetFormatType_;
	debugLineKey.primitiveType = PrimitiveType::LINE;
	return debugLineKey;
}



/// ========================================= Offscreen用PSOKey ========================================== ///

inline PSOKey CreateFullscreenPSOKey() {
	PSOKey fullscreenKey;
	fullscreenKey.lightModelType = LightModelType::NONE;
	fullscreenKey.renderModelType = RenderModelType::FullscreenQuad;
	fullscreenKey.blendModeType = BlendModeType::Opaque;
	fullscreenKey.rasterizerMode = RasterizerMode::CullNone;
	fullscreenKey.depthStencilType = DepthStencilType::Disable;
	fullscreenKey.renderTargetFormatType = RenderTargetFormatType::BackBuffer;
	fullscreenKey.primitiveType = PrimitiveType::TRIANGLE;
	return fullscreenKey;
}

inline PSOKey CreateColorGradingPSOKey() {
	PSOKey colorGradingKey = CreateFullscreenPSOKey();
	colorGradingKey.renderModelType = RenderModelType::ColorGradient;
	return colorGradingKey;
}

inline PSOKey CreateVignettePSOKey() {
	PSOKey vignetteKey = CreateFullscreenPSOKey();
	vignetteKey.renderModelType = RenderModelType::Vignette;
	return vignetteKey;
}

inline PSOKey CreateBlurPSOKey() {
	PSOKey blurKey = CreateFullscreenPSOKey();
	blurKey.renderModelType = RenderModelType::Blur;
	return blurKey;
}