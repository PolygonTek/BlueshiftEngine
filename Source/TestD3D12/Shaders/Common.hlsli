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
#define INV_SQRT2       0.70710678118654752440
#define GOLDEN_RATIO    1.6180339887

#ifndef half
#define half min16float
#define half2 min16float2
#define half3 min16float3
#define half4 min16float4
#define half3x3 min16float3x3
#define half3x4 min16float3x4
#define half4x4 min16float4x4
#endif

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

inline float2 ComplexMul(float2 a, float2 b) {
    return float2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

inline float2 ComplexConj(float2 a) {
    return float2(a.x, -a.y);
}

#define ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS "StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s101, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s102, addressU = TEXTURE_ADDRESS_MIRROR, addressV = TEXTURE_ADDRESS_MIRROR, addressW = TEXTURE_ADDRESS_MIRROR, filter = FILTER_MIN_MAG_MIP_LINEAR)," \
    "StaticSampler(s103, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_POINT)," \
    "StaticSampler(s104, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_MIN_MAG_MIP_POINT)," \
    "StaticSampler(s105, addressU = TEXTURE_ADDRESS_MIRROR, addressV = TEXTURE_ADDRESS_MIRROR, addressW = TEXTURE_ADDRESS_MIRROR, filter = FILTER_MIN_MAG_MIP_POINT)," \
    "StaticSampler(s106, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_ANISOTROPIC, maxAnisotropy = 16)," \
    "StaticSampler(s107, addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_WRAP, filter = FILTER_ANISOTROPIC, maxAnisotropy = 16)," \
    "StaticSampler(s108, addressU = TEXTURE_ADDRESS_MIRROR, addressV = TEXTURE_ADDRESS_MIRROR, addressW = TEXTURE_ADDRESS_MIRROR, filter = FILTER_ANISOTROPIC, maxAnisotropy = 16)," \
    "StaticSampler(s109, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, comparisonFunc = COMPARISON_LESS_EQUAL)"

#define DECLARE_COMMON_STATIC_SAMPLERS \
    SamplerState linearClampSampler : register(s100); \
    SamplerState linearWrapSampler : register(s101); \
    SamplerState linearMirrorSampler : register(s102); \
    SamplerState pointClampSampler : register(s103); \
    SamplerState pointWrapSampler : register(s104); \
    SamplerState pointMirrorSampler : register(s105); \
    SamplerState anisoClampSampler : register(s106); \
    SamplerState anisoWrapSampler : register(s107); \
    SamplerState anisoMirrorSampler : register(s108); \
    SamplerComparisonState shadowComparisonSampler : register(s109);

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

// 좌상단 원점의 UV [0, 1] 과 양의 선형 깊이로 뷰 위치를 복원한다.
// 오른손 뷰 공간의 카메라 전방은 음의 Z 축이므로 선형 깊이는 -view.z 이다.
// uvToView 는 깊이 생성에 사용한 투영과 일치해야 하며, 투영 지터를 포함한다.
inline float3 ViewPositionFromLinearDepth(float2 uv, float linearDepth, float4 uvToView, bool orthogonal) {
    const float2 viewXY = uv * uvToView.xy + uvToView.zw;
    return float3(viewXY * (orthogonal ? 1.0f : linearDepth), -linearDepth);
}

// 뷰 위치를 원근 나눗셈 전의 클립 위치로 변환한다.
// projectionXY 는 XY 투영 배율과 오프셋이다.
// clip.z = view.z * projectionZ.x + projectionZ.y
inline float4 ProjectViewPosition(float3 position, float4 projectionXY, float2 projectionZ, bool orthogonal) {
    const float clipW = orthogonal ? 1.0f : -position.z;
    return float4(position.xy * projectionXY.xy + clipW * projectionXY.zw, projectionZ.x * position.z + projectionZ.y, clipW);
}

// clip space position 을 좌상단 원점의 UV [0, 1] 와 device depth 로 변환한다.
inline float3 ClipPositionToScreenPosition(float4 clipPosition, float2 ndcDepthScaleBias) {
    const float3 ndcPosition = clipPosition.xyz / clipPosition.w;
    const float2 uv = ndcPosition.xy * float2(0.5f, -0.5f) + 0.5f;
    const float deviceDepth = (ndcPosition.z - ndcDepthScaleBias.y) / ndcDepthScaleBias.x;
    return float3(uv, deviceDepth);
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

// NDC Z 를 선형 Depth 로 변환한다.
// projDepthA: projection matrix [2][2]
// projDepthB: projection matrix [2][3]
inline float LinearizePerspectiveDepth(float ndcDepth, float projDepthA, float projDepthB) {
    // ndcZ = (A * viewZ + B) / (-viewZ)
    // viewZ = -B / (ndcZ + A)
    // 오른손 뷰 공간에서 카메라 전방은 음의 Z 축이므로, -viewZ 를 양의 선형 깊이로 사용한다.
    // viewDistance = -viewZ = B / (ndcZ + A)
    return projDepthB / (ndcDepth + projDepthA);
}

// NDC Z 를 선형 Depth 로 변환한다.
// projDepthA: projection matrix [2][2]
// projDepthB: projection matrix [2][3]
inline float LinearizeOrthogonalDepth(float ndcDepth, float projDepthA, float projDepthB) {
    // ndcZ = (A * viewZ + B) / 1
    // viewZ = (ndcZ - B) / A
    // 오른손 뷰 공간에서 카메라 전방은 음의 Z 축이므로, -viewZ 를 양의 선형 깊이로 사용한다.
    // viewDistance = -viewZ = (B - ndcZ) / A
    return (projDepthB - ndcDepth) / projDepthA;
}

// device depth 를 카메라 전방 축을 따르는 양의 linear view depth 로 변환한다.
// 작은 분모도 유효한 원거리 표면을 나타낼 수 있으므로 분모의 최솟값을 제한하지 않는다.
// 무한 원거리 투영의 배경은 무한 깊이가 될 수 있다. 호출부는 배경을 제외하거나,
// 위치 복원과 깊이 차이 계산 전에 무한 깊이를 별도로 처리해야 한다.
inline float ToLinearDepth(float deviceDepth, float2 ndcDepthScaleBias, float2 projectionZ, bool orthogonal) {
    const float ndcDepth = deviceDepth * ndcDepthScaleBias.x + ndcDepthScaleBias.y;
    return orthogonal ?
        LinearizeOrthogonalDepth(ndcDepth, projectionZ.x, projectionZ.y) :
        LinearizePerspectiveDepth(ndcDepth, projectionZ.x, projectionZ.y);
}

// Returns a uniformly sampled direction on the unit sphere.
// xi must be in [0, 1)^2. PDF with respect to solid angle is 1 / (4 * PI).
inline float3 SampleSphereUniform(float2 xi) {
    const float cosTheta = 1.0f - 2.0f * xi.x;
    const float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
    const float phi = TWO_PI * xi.y;

    float sinPhi;
    float cosPhi;
    sincos(phi, sinPhi, cosPhi);
    return float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
}

// Returns a uniformly sampled direction on the unit hemisphere about local +Z.
// xi must be in [0, 1)^2. PDF with respect to solid angle is 1 / (2 * PI).
inline float3 SampleHemisphereUniform(float2 xi) {
    const float cosTheta = 1.0f - xi.x;
    const float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
    const float phi = TWO_PI * xi.y;

    float sinPhi;
    float cosPhi;
    sincos(phi, sinPhi, cosPhi);
    return float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
}

// Returns a cosine-weighted direction on the unit hemisphere about local +Z.
// xi must be in [0, 1)^2. PDF with respect to solid angle is cosTheta / PI.
inline float3 SampleHemisphereCosine(float2 xi) {
    const float cosTheta = sqrt(1.0f - xi.x);
    const float sinTheta = sqrt(xi.x);
    const float phi = TWO_PI * xi.y;

    float sinPhi;
    float cosPhi;
    sincos(phi, sinPhi, cosPhi);
    return float3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
}

#endif // __COMMON_HLSLI__
