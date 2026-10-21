#ifndef __SSR_RADIANCE_HLSLI__
#define __SSR_RADIANCE_HLSLI__

#include "../Colors.hlsli"

float3 CompressSSRRadiance(float3 color) {
    return color * rcp(1.0f + GetLuma(color));
}

float3 DecompressSSRRadiance(float3 color) {
    return color * rcp(max(1.0f - GetLuma(color), 1.0e-4f));
}

#endif // __SSR_RADIANCE_HLSLI__
