#ifndef __TEXTURE_STREAMING_FEEDBACK_HLSLI__
#define __TEXTURE_STREAMING_FEEDBACK_HLSLI__

#include "Common.hlsli"
#include "ShaderInterop.h"

inline void WriteMipmapFeedback(RWBuffer<uint> feedbackBuffer, in uint materialIndex, in float2 dFdx_uv, in float2 dFdy_uv) {
    const float lod = ComputeTextureLod(TextureStreamingFeedbackResolution, dFdx_uv, dFdy_uv);
    const uint clampedLod = (uint)clamp(lod, 0.0f, (float)TextureStreamingFeedbackMaxLOD);
    const uint resolution = TextureStreamingFeedbackResolution >> clampedLod;
    const uint resolutionMask = WaveActiveBitOr(resolution);

    if (WaveIsFirstLane()) {
        InterlockedOr(feedbackBuffer[materialIndex], resolutionMask);
    }
}

#endif // __TEXTURE_STREAMING_FEEDBACK_HLSLI__
