#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__

#define FLT_EPSILON     1.192092896e-07 // Smallest positive number, such that 1.0 + FLT_EPSILON != 1.0
#define FLT_MIN         1.175494351e-38 // Minimum representable positive floating-point number
#define FLT_MAX         3.402823466e+38 // Maximum representable floating-point number
#define HALF_MAX        65504.0 // (2 - 2^-10) * 2^15
#define HALF_MAX_MINUS1 65472.0 // (2 - 2^-9) * 2^15

#define HALF_PI         1.5707963267948966192313216916398
#define PI              3.1415926535897932384626433832795
#define TWO_PI          6.283185307179586476925286766559
#define FOUR_PI         12.566370614359172953850573533118
#define INV_HALF_PI     0.63661977236758134307553505349004
#define INV_PI          0.31830988618379067153776752674503
#define INV_TWO_PI      0.15915494309189533576888376337251
#define INV_FOUR_PI     0.07957747154594766788444188168626
#define SQRT2           1.41421356237309504880
#define GOLDEN_RATIO    1.6180339887

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

// [-1, 1] 범위의 단위 벡터를 RGB8 UNORM 으로 remap 해 패킹한다.
inline uint PackUnitVector3ToRGB8UNorm(float3 value) {
    const uint3 packedValue = uint3(floor(saturate(value * 0.5 + 0.5) * 255.0 + 0.5));
    return packedValue.x | (packedValue.y << 8u) | (packedValue.z << 16u);
}

// RGB8 UNORM 값을 [-1, 1] 범위의 float3 로 복원한다. 정규화된 방향으로 사용할 때는 normalize 가 필요하다.
inline float3 UnpackRGB8UNormToUnitVector3(uint value) {
    return float3(value & 0xFFu, (value >> 8u) & 0xFFu, (value >> 16u) & 0xFFu) / 255.0 * 2.0 - 1.0;
}

// [-1, 1] 범위의 4성분 단위 벡터를 RGBA8 UNORM 으로 remap 해 패킹한다.
inline uint PackUnitVector4ToRGBA8UNorm(float4 value) {
    const uint4 packedValue = uint4(floor(saturate(value * 0.5 + 0.5) * 255.0 + 0.5));
    return packedValue.x | (packedValue.y << 8u) | (packedValue.z << 16u) | (packedValue.w << 24u);
}

// RGBA8 UNORM 값을 [-1, 1] 범위의 float4 로 복원한다. 정규화된 벡터로 사용할 때는 normalize 가 필요하다.
inline float4 UnpackRGBA8UNormToUnitVector4(uint value) {
    return float4(value & 0xFFu, (value >> 8u) & 0xFFu, (value >> 16u) & 0xFFu, value >> 24u) / 255.0 * 2.0 - 1.0;
}

// [0, 1] 범위로 clamp 한 float4 를 반올림해 RGBA8 UNORM 으로 양자화한다.
inline uint PackFloat4ToRGBA8UNorm(float4 value) {
    const uint4 packedValue = uint4(floor(saturate(value) * 255.0 + 0.5));
    return packedValue.x | (packedValue.y << 8u) | (packedValue.z << 16u) | (packedValue.w << 24u);
}

// RGBA8 UNORM 값을 [0, 1] 범위의 float4 로 복원한다.
inline float4 UnpackRGBA8UNormToFloat4(uint value) {
    return float4(value & 0xFFu, (value >> 8u) & 0xFFu, (value >> 16u) & 0xFFu, value >> 24u) / 255.0;
}

// [0, 1] 범위로 clamp 한 float4 를 RGB 10-bit 와 alpha 2-bit 의 RGB10A2 UNORM 으로 양자화한다.
inline uint PackFloat4ToRGB10A2UNorm(float4 value) {
    const uint4 packedValue = uint4(floor(saturate(value) * float4(1023.0, 1023.0, 1023.0, 3.0) + 0.5));
    return packedValue.x | (packedValue.y << 10u) | (packedValue.z << 20u) | (packedValue.w << 30u);
}

// RGB10A2 UNORM 값을 [0, 1] 범위의 float4 로 복원한다. alpha 의 정밀도는 2-bit 이다.
inline float4 UnpackRGB10A2UNormToFloat4(uint value) {
    return float4(value & 0x3FFu, (value >> 10u) & 0x3FFu, (value >> 20u) & 0x3FFu, value >> 30u) / float4(1023.0, 1023.0, 1023.0, 3.0);
}

// float2 를 IEEE 754 half precision 두 개로 변환해 RG16F 로 패킹한다.
inline uint PackFloat2ToRG16F(float2 value) {
    return f32tof16(value.x) | (f32tof16(value.y) << 16u);
}

// RG16F 값을 IEEE 754 half precision 두 개로 해석해 float2 로 복원한다.
inline float2 UnpackRG16FToFloat2(uint value) {
    return float2(f16tof32(value & 0xFFFFu), f16tof32(value >> 16u));
}

// float3 를 IEEE 754 half precision 세 개로 변환해 RGB16F 로 패킹한다.
inline uint2 PackFloat3ToRGB16F(float3 value) {
    return uint2(PackFloat2ToRG16F(value.xy), f32tof16(value.z));
}

// RGB16F 값을 IEEE 754 half precision 세 개로 해석해 float3 로 복원한다.
inline float3 UnpackRGB16FToFloat3(uint2 value) {
    return float3(UnpackRG16FToFloat2(value.x), f16tof32(value.y & 0xFFFFu));
}

// float4 를 IEEE 754 half precision 네 개로 변환해 RGBA16F 로 패킹한다.
inline uint2 PackFloat4ToRGBA16F(float4 value) {
    return uint2(PackFloat2ToRG16F(value.xy), PackFloat2ToRG16F(value.zw));
}

// RGBA16F 값을 IEEE 754 half precision 네 개로 해석해 float4 로 복원한다.
inline float4 UnpackRGBA16FToFloat4(uint2 value) {
    return float4(UnpackRG16FToFloat2(value.x), UnpackRG16FToFloat2(value.y));
}

// uint2 각 성분의 하위 16-bit 를 RG16 UINT 형식으로 패킹한다.
inline uint PackUint2ToRG16Uint(uint2 value) {
    return (value.x & 0xFFFFu) | ((value.y & 0xFFFFu) << 16u);
}

// RG16 UINT 값을 두 개의 16-bit unsigned integer 로 복원한다.
inline uint2 UnpackRG16UintToUint2(uint value) {
    return uint2(value & 0xFFFFu, value >> 16u);
}

// [0, 1] 범위로 clamp 한 float2 를 반올림해 RG16 UNORM 으로 양자화한다.
inline uint PackFloat2ToRG16UNorm(float2 value) {
    const uint2 packedValue = uint2(floor(saturate(value) * 65535.0 + 0.5));
    return PackUint2ToRG16Uint(packedValue);
}

// RG16 UNORM 값을 [0, 1] 범위의 float2 로 복원한다.
inline float2 UnpackRG16UNormToFloat2(uint value) {
    return float2(value & 0xFFFFu, value >> 16u) / 65535.0;
}

// [0, 1] 범위로 clamp 한 float4 를 반올림해 RGBA16 UNORM 으로 양자화한다.
inline uint2 PackFloat4ToRGBA16UNorm(float4 value) {
    return uint2(PackFloat2ToRG16UNorm(value.xy), PackFloat2ToRG16UNorm(value.zw));
}

// RGBA16 UNORM 값을 [0, 1] 범위의 float4 로 복원한다.
inline float4 UnpackRGBA16UNormToFloat4(uint2 value) {
    return float4(UnpackRG16UNormToFloat2(value.x), UnpackRG16UNormToFloat2(value.y));
}

// value^4
inline float Pow4(float value) {
    const float valueSquared = value * value;
    return valueSquared * valueSquared;
}

// value^5
inline float Pow5(float value) {
    const float valueSquared = value * value;
    return valueSquared * valueSquared * value;
}

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

// Interleaved gradient function from Jimenez 2014
// http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
float InterleavedGradientNoise(uint2 pixelCoord) {
    float f = dot(float2(0.06711056, 0.00583715), pixelCoord);
    return frac(52.9829189 * frac(f));
}

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
inline uint BitfieldExtract(uint src, uint off, uint bits) { uint mask = (1u << bits) - 1u; return (src >> off) & mask; } // ABfe
inline uint BitfieldInsert(uint src, uint ins, uint bits) { uint mask = (1u << bits) - 1u; return (ins & mask) | (src & (~mask)); } // ABfiM
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

// 현재 픽셀의 표면을 이전 프레임 카메라로 reprojection 했을 때의 depth 를 구한다.
inline float ComputeReprojectedPreviousDepth(in float2 uv, in float currentDepth, in float4x4 invViewProjMatrix, in float4x4 prevViewProjMatrix) {
    const float2 ndcXY = uv * float2(2.0, -2.0) + float2(-1.0, 1.0);
    const float4 currentClipPosition = float4(ndcXY, currentDepth, 1.0);
    float4 worldPosition = mul(invViewProjMatrix, currentClipPosition);
    worldPosition /= worldPosition.w;

    const float4 previousClipPosition = mul(prevViewProjMatrix, worldPosition);
    return previousClipPosition.z / previousClipPosition.w;
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

#endif // __COMMON_HLSLI__
