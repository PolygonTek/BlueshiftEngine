#ifndef _COMMON_HLSLI_
#define _COMMON_HLSLI_

#include "ShaderInterop.h"

inline float ComputeTextureLod(in uint2 textureSize, in float2 dFdx_uv, in float2 dFdy_uv, uint anisotropy = 0) {
    // Isotropic:
    if (anisotropy == 0) {
        return log2(max(length(dFdx_uv * textureSize), length(dFdy_uv * textureSize)));
    }

    // Anisotropic:
    dFdx_uv *= textureSize;
    dFdy_uv *= textureSize;
    float squaredLengthX = dot(dFdx_uv, dFdx_uv);
    float squaredLengthY = dot(dFdy_uv, dFdy_uv);
    float determinant = abs(dFdx_uv.x * dFdy_uv.y - dFdx_uv.y * dFdy_uv.x);
    float squaredLengthMajor = squaredLengthX > squaredLengthY ? squaredLengthX : squaredLengthY;
    float lengthMajor = sqrt(squaredLengthMajor);
    float ratioOfAnisotropy = squaredLengthMajor / determinant;
    float lengthMinor = 0;
    if (ratioOfAnisotropy > anisotropy) {
        // ratio is clamped - LOD is based on ratio (preserves area)
        lengthMinor = lengthMajor / anisotropy;
    } else {
        // ratio not clamped - LOD is based on area
        lengthMinor = determinant / lengthMajor;
    }
    return log2(lengthMinor);
}

inline void WriteMipmapFeedback(RWBuffer<uint> feedbackBuffer, in uint textureIndex, in float2 dFdx_uv, in float2 dFdy_uv) {
    const float lod = ComputeTextureLod(TEXTURE_STREAMING_FEEDBACK_RESOLUTION, dFdx_uv, dFdy_uv);
    const uint clampedLod = (uint)clamp(lod, 0.0f, (float)TEXTURE_STREAMING_FEEDBACK_MAX_LOD);
    const uint resolution = TEXTURE_STREAMING_FEEDBACK_RESOLUTION >> clampedLod;
    const uint resolutionMask = WaveActiveBitOr(resolution);
    
    if (WaveIsFirstLane()) {
        InterlockedOr(feedbackBuffer[textureIndex], resolutionMask);
    }
}

#endif
