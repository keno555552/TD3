#include "RenderTargetFormat.h"

DXGI_FORMAT GetDXGIFormat(RenderTargetFormatType format) {
    switch (format) {

    case RenderTargetFormatType::BackBuffer:
        return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

    case RenderTargetFormatType::RGBA8:
        return DXGI_FORMAT_R8G8B8A8_UNORM;

    case RenderTargetFormatType::RGBA16F:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;

    case RenderTargetFormatType::R32F:
        return DXGI_FORMAT_R32_FLOAT;

    case RenderTargetFormatType::Depth32F:
        return DXGI_FORMAT_D32_FLOAT;

    default:
        return DXGI_FORMAT_UNKNOWN;
    }
}
