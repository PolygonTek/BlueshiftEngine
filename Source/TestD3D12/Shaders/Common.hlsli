#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

#include "ShaderInterop.h"

#ifndef half
#define half min16float
#define half2 min16float2
#define half3 min16float3
#define half4 min16float4
#define half3x3 min16float3x3
#define half3x4 min16float3x4
#define half4x4 min16float4x4
#endif

#define ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS "StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s101, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s102, addressU = TEXTURE_ADDRESS_MIRROR, addressV = TEXTURE_ADDRESS_MIRROR, addressW = TEXTURE_ADDRESS_MIRROR, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s103, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_POINT)," \
    "StaticSampler(s104, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_POINT)," \
    "StaticSampler(s105, addressU = TEXTURE_ADDRESS_MIRROR, addressV = TEXTURE_ADDRESS_MIRROR, addressW = TEXTURE_ADDRESS_MIRROR, filter = FILTER_MIN_MAG_MIP_POINT)," \
    "StaticSampler(s106, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_ANISOTROPIC, maxAnisotropy = 16)," \
    "StaticSampler(s107, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_ANISOTROPIC, maxAnisotropy = 16)," \
    "StaticSampler(s108, addressU = TEXTURE_ADDRESS_MIRROR, addressV = TEXTURE_ADDRESS_MIRROR, addressW = TEXTURE_ADDRESS_MIRROR, filter = FILTER_ANISOTROPIC, maxAnisotropy = 16)"

#define DECLARE_COMMON_STATIC_SAMPLERS \
    SamplerState linearClampSampler : register(s100); \
    SamplerState linearWrapSampler : register(s101); \
    SamplerState linearMirrorSampler : register(s102); \
    SamplerState pointClampSampler : register(s103); \
    SamplerState pointWrapSampler : register(s104); \
    SamplerState pointMirrorSampler : register(s105); \
    SamplerState anisoClampSampler : register(s106); \
    SamplerState anisoWrapSampler : register(s107); \
    SamplerState anisoMirrorSampler : register(s108);

// from https://github.com/GPUOpen-Effects/FidelityFX-Denoiser/blob/master/ffx-shadows-dnsr/ffx_denoiser_shadows_util.h
//  LANE TO 8x8 MAPPING
//  ===================
//  00 01 08 09 10 11 18 19 
//  02 03 0a 0b 12 13 1a 1b
//  04 05 0c 0d 14 15 1c 1d
//  06 07 0e 0f 16 17 1e 1f 
//  20 21 28 29 30 31 38 39 
//  22 23 2a 2b 32 33 3a 3b
//  24 25 2c 2d 34 35 3c 3d
//  26 27 2e 2f 36 37 3e 3f 
inline uint BitfieldExtract(uint src, uint off, uint bits) { uint mask = (1 << bits) - 1; return (src >> off) & mask; } // ABfe
inline uint BitfieldInsert(uint src, uint ins, uint bits) { uint mask = (1 << bits) - 1; return (ins & mask) | (src & (~mask)); } // ABfiM
inline uint2 RemapLane8x8(uint lane) {
    return uint2(BitfieldInsert(BitfieldExtract(lane, 2u, 3u), lane, 1u),
        BitfieldInsert(BitfieldExtract(lane, 3u, 3u),
        BitfieldExtract(lane, 1u, 2u), 2u));
}

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

inline void WriteMipmapFeedback(RWBuffer<uint> feedbackBuffer, in uint materialIndex, in float2 dFdx_uv, in float2 dFdy_uv) {
    const float lod = ComputeTextureLod(TextureStreamingFeedbackResolution, dFdx_uv, dFdy_uv);
    const uint clampedLod = (uint)clamp(lod, 0.0f, (float)TextureStreamingFeedbackMaxLOD);
    const uint resolution = TextureStreamingFeedbackResolution >> clampedLod;
    const uint resolutionMask = WaveActiveBitOr(resolution);

    if (WaveIsFirstLane()) {
        InterlockedOr(feedbackBuffer[materialIndex], resolutionMask);
    }
}

// Reconstructs world-space position from depth buffer
// uv    : screen space coordinate in [0, 1] range, origin at upper-left corner of the screen
// depth : depth value in NDC Z range [0, 1]
// invVP : Inverse view-projection matrix that was used to generate the depth value
inline float3 ReconstructPosition(in float2 uv, in float depth, in float4x4 invVP) {
    float ndcX = uv.x * 2.0f - 1.0f;
    float ndcY = (1.0f - uv.y) * 2.0f - 1.0f;
    float4 ndcPosition = float4(ndcX, ndcY, depth, 1.0f);
    float4 worldPosition = mul(invVP, ndcPosition);
    return worldPosition.xyz / worldPosition.w;
}

inline float SafeSignedDenominator(float value) {
    const float minAbsValue = 1.0e-6;
    if (abs(value) >= minAbsValue) {
        return value;
    }
    return value < 0.0 ? -minAbsValue : minAbsValue;
}

// projDepthA: projection matrix [2][2]
// projDepthB: projection matrix [2][3]
inline float LinearizePerspectiveDepth(float ndcDepth, float projDepthA, float projDepthB) {
    // ndcZ = (A * viewZ + B) / (-viewZ)
    // viewZ = -B / (ndcZ + A)
    // projection matrix 는 오른손 좌표계이므로 음수 방향이 카메라 앞쪽이다. 그러므로 preview 를 위해서 뒤집는다.
    // viewDistance = -viewZ = B / (ndcZ + A)
    return projDepthB / SafeSignedDenominator(ndcDepth + projDepthA);
}

// projDepthA: projection matrix [2][2]
// projDepthB: projection matrix [2][3]
inline float LinearizeOrthogonalDepth(float ndcDepth, float projDepthA, float projDepthB) {
    // ndcZ = (A * viewZ + B) / 1
    // viewZ = (ndcZ - B) / A
    // projection matrix 는 오른손 좌표계이므로 음수 방향이 카메라 앞쪽이다. 그러므로 preview 를 위해서 뒤집는다.
    // viewDistance = -viewZ = (B - ndcZ) / A
    return (projDepthB - ndcDepth) / SafeSignedDenominator(projDepthA);
}

#endif
