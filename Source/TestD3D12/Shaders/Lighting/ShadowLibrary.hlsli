#ifndef __SHADOW_LIBRARY_HLSLI__
#define __SHADOW_LIBRARY_HLSLI__

#include "../ShaderInterop.h"

#ifndef SHADOW_MAP_QUALITY
#define SHADOW_MAP_QUALITY 2
#endif

// r_shadowMapRotationUseTexture 의 global define. 0 = 고정 seed 해시, 1 = STBN 방향 텍스처.
#ifndef USE_SHADOW_ROTATION_TEXTURE
#define USE_SHADOW_ROTATION_TEXTURE 0
#endif

// 0: Off, 1: Visibility, 2: Coverage.
#ifndef DEBUG_CASCADED_SHADOW_MAP
#define DEBUG_CASCADED_SHADOW_MAP 0
#endif

// 한 픽셀에서 필요한 Q3 회전을 최초 샘플링 때 계산하고 모든 shadow light 에 재사용한다.
struct ShadowSamplingContext {
    uint2 pixelCoord;
#if SHADOW_MAP_QUALITY == 3
    float2 rotationCosSin;
    bool hasRotation;
#endif
};

ShadowSamplingContext InitShadowSamplingContext(uint2 pixelCoord) {
    ShadowSamplingContext context = (ShadowSamplingContext)0;
    context.pixelCoord = pixelCoord;
    return context;
}

#if SHADOW_MAP_QUALITY == 3
#include "../Random.hlsli"
#if USE_SHADOW_ROTATION_TEXTURE
#include "../STBN.hlsli"
#endif

static const float4 poissonDisk16[8] = {
    float4(-0.763426298f, -0.323406895f, +0.766319366f, -0.623135770f),
    float4(-0.076328428f, -0.753192720f, +0.279561064f, +0.238163504f),
    float4(-0.742249744f, +0.370939623f, -0.660848598f, -0.712457854f),
    float4(-0.310207847f, +0.224297952f, +0.790030467f, +0.613067582f),
    float4(+0.359203912f, -0.790250544f, +0.435542438f, -0.383922411f),
    float4(-0.214735562f, -0.339508324f, +0.641830388f, +0.154710194f),
    float4(-0.196030554f, +0.808038824f, -0.659760394f, +0.741026087f),
    float4(+0.161954823f, +0.637323276f, +0.116563631f, -0.114275248f)
};

float4 GetShadowRotationMatrix(uint2 pixelCoord) {
#if USE_SHADOW_ROTATION_TEXTURE
    [branch]
    if (frameConstants.stbnUnitVec2TextureIndex != 0u) {
        // TAA 로 누적할 때만 시간 슬라이스를 바꾼다. TAA 를 사용하지 않는 카메라는 패턴을 고정한다.
        const uint noiseFrameIndex = (cameraConstants.flags & ShaderCameraFlagUseTAA) != 0u ? frameConstants.frameCount : 0u;
        const float2 cs = ReadSTBNUnitVec2(GetTexture2DArray(frameConstants.stbnUnitVec2TextureIndex), pixelCoord, noiseFrameIndex);
        return float4(cs.x, -cs.y, cs.y, cs.x); // column-major rotation matrix
    }
#endif
    // STBN 미사용/로드 실패 시 전체 픽셀 좌표와 고정 seed 로 타일 반복 및 프레임마다 바뀌는 노이즈를 피한다.
    const uint h = Hash(pixelCoord.x ^ Hash(pixelCoord.y ^ 23119u));
    const float angle = TWO_PI * UintToUniform01(h);
    float s, c;
    sincos(angle, s, c);
    return float4(c, -s, s, c); // column-major rotation matrix
}

float4 GetShadowRotationMatrix(inout ShadowSamplingContext context) {
    [branch]
    if (!context.hasRotation) {
        context.rotationCosSin = GetShadowRotationMatrix(context.pixelCoord).xz;
        context.hasRotation = true;
    }
    const float2 cs = context.rotationCosSin;
    return float4(cs.x, -cs.y, cs.y, cs.x); // column-major rotation matrix
}
#endif

float SampleShadowMap_Q0(Texture2D<float> shadowMap, float3 shadowCoord) {
    return shadowMap.SampleCmpLevelZero(shadowComparisonGSampler, shadowCoord.xy, shadowCoord.z);
}

float SampleShadowMap_Q1(Texture2D<float> shadowMap, float3 shadowCoord, float2 filterOffset) {
    float shadowSum = shadowMap.SampleCmpLevelZero(shadowComparisonGSampler, shadowCoord.xy, shadowCoord.z);
    shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonGSampler, shadowCoord.xy + float2(0.0f, filterOffset.y), shadowCoord.z);
    shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonGSampler, shadowCoord.xy + float2(0.0f, -filterOffset.y), shadowCoord.z);
    shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonGSampler, shadowCoord.xy + float2(filterOffset.x, 0.0f), shadowCoord.z);
    shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonGSampler, shadowCoord.xy + float2(-filterOffset.x, 0.0f), shadowCoord.z);
    return shadowSum * 0.2f;
}

float SampleShadowMap_Q2(Texture2D<float> shadowMap, float3 shadowCoord, float2 filterOffset) {
    float shadowSum = 0.0f;
    [unroll]
    for (int y = -1; y <= 1; ++y) {
        [unroll]
        for (int x = -1; x <= 1; ++x) {
            shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonGSampler, shadowCoord.xy + float2(x, y) * filterOffset, shadowCoord.z);
        }
    }
    return shadowSum * (1.0f / 9.0f);
}

#if SHADOW_MAP_QUALITY == 3
float SampleShadowMap_Q3(Texture2D<float> shadowMap, float3 shadowCoord, float2 filterOffset, float4 rotMat) {
    float shadowSum = 0.0f;
    [unroll]
    for (int i = 0; i < 8; ++i) {
        const float4 kernel = poissonDisk16[i];
        const float4 rotated = rotMat.xyzw * kernel.xxww + rotMat.zwxy * kernel.yyzz;
        shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonGSampler, shadowCoord.xy + filterOffset * rotated.xy, shadowCoord.z);
        shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonGSampler, shadowCoord.xy + filterOffset * rotated.zw, shadowCoord.z);
    }
    return shadowSum * 0.0625f;
}
#endif

#if SHADOW_MAP_QUALITY == 3
float SampleShadowMap(Texture2D<float> shadowMap, float3 shadowCoord, float2 texelSize, float filterRadius, float4 rotMat) {
    return SampleShadowMap_Q3(shadowMap, shadowCoord, texelSize * filterRadius, rotMat);
}
#else
float SampleShadowMap(Texture2D<float> shadowMap, float3 shadowCoord, float2 texelSize) {
#if SHADOW_MAP_QUALITY == 2
    return SampleShadowMap_Q2(shadowMap, shadowCoord, texelSize);
#elif SHADOW_MAP_QUALITY == 1
    return SampleShadowMap_Q1(shadowMap, shadowCoord, texelSize);
#else
    return SampleShadowMap_Q0(shadowMap, shadowCoord);
#endif
}
#endif

float EvaluateSpotShadowLighting(uint shadowIndex, inout ShadowSamplingContext shadowSampling, float3 worldPosition) {
    const ShaderShadowAtlasData atlas = cameraConstants.shadowAtlas;
    // Invalid index 와 비활성 atlas 모두 테이블/texture 에 접근하기 전에 걸러낸다.
    [branch]
    if (shadowIndex >= atlas.numShadows) {
        return 1.0f;
    }

    const ShaderLocalShadow shadow = GetShaderLocalShadowTable(atlas.shadowTableIndex)[shadowIndex];
    [branch]
    if (shadow.numShadowViews != 1 || shadow.firstShadowView >= atlas.numShadowViews) {
        return 1.0f;
    }

    const ShaderLocalShadowView view = GetShaderLocalShadowViewTable(atlas.shadowViewTableIndex)[shadow.firstShadowView];
    const float4 projectedPosition = mul(view.worldToShadowCoordMatrix, float4(worldPosition, 1.0f));
    if (projectedPosition.w <= 0.0f) {
        return 1.0f;
    }

    const float3 shadowCoord = projectedPosition.xyz / projectedPosition.w;
    // border 는 필터를 위한 caster 영역이다. receiver 는 원래 조명 영역과 reverse-Z depth [0, 1] 안에 있어야 한다.
    if (any(shadowCoord.xy < shadow.normalizedBorder) || any(shadowCoord.xy > 1.0f - shadow.normalizedBorder) || shadowCoord.z < 0.0f || shadowCoord.z > 1.0f) {
        return 1.0f;
    }

    const float3 atlasCoord = float3(shadowCoord.xy * view.atlasScaleBias.xy + view.atlasScaleBias.zw, shadowCoord.z);
    Texture2D<float> shadowMap = GetTexture2DFloat(atlas.textureIndex);
#if SHADOW_MAP_QUALITY == 3
    const float4 rotMat = GetShadowRotationMatrix(shadowSampling);
    return SampleShadowMap(shadowMap, atlasCoord, atlas.texelSize, shadow.filterRadius, rotMat);
#else
    return SampleShadowMap(shadowMap, atlasCoord, atlas.texelSize);
#endif
}

// 입력은 worldPosition - lightOrigin 이다. Normalize 나 광원 회전을 적용하지 않는다.
// 면 경계에서 절댓값이 같으면 X, Y, Z 순서로 선택한다. 광원 원점에서는 유효한 면이 없다.
uint SelectPointShadowFace(float3 lightToReceiver) {
    const float3 magnitude = abs(lightToReceiver);
    if (max(max(magnitude.x, magnitude.y), magnitude.z) == 0.0f) {
        return ShaderPointShadowFaceInvalid;
    }
    if (magnitude.x >= magnitude.y && magnitude.x >= magnitude.z) {
        return lightToReceiver.x >= 0.0f ? ShaderPointShadowFacePositiveX : ShaderPointShadowFaceNegativeX;
    }
    if (magnitude.y >= magnitude.z) {
        return lightToReceiver.y >= 0.0f ? ShaderPointShadowFacePositiveY : ShaderPointShadowFaceNegativeY;
    }
    return lightToReceiver.z >= 0.0f ? ShaderPointShadowFacePositiveZ : ShaderPointShadowFaceNegativeZ;
}

// CubeMapFaceToViewAxis + reverse-Z 90도 투영 + tile UV 변환과 같은 좌표를 만든다.
// Atlas 는 최종 Y 반전 blit 없이 기록하므로 Cubemap.hlsli 의 cubemap UV 와 세로 방향이 반대다.
// facePosition 은 (right, down, forward). Depth 는 radial distance 가 아닌 forward distance 로 비교한다.
// normalizedBorder = borderTexels / tileResolution.
// depthScaleBias = (A, B), A = -near / (far - near), B = -far * A.
// 0 < near < far 인 유한한 원근 투영이며 near 에서 depth 1, far 에서 0 이다.
// 유효 90도 영역은 [border, 1-border] 이며 렌더링 FOV 는 border 만큼 확장한다.
// 반환값은 (tileU, tileV, depth). 잘못된 면/뒤쪽 점은 depth -1 로 반환하며 near/far 밖 depth 를 clamp 하지 않는다.
float3 CalcPointShadowFaceCoord(uint faceIndex, float3 lightToReceiver, float normalizedBorder, float2 depthScaleBias) {
    float3 facePosition;
    switch (faceIndex) {
    case ShaderPointShadowFacePositiveX:
        facePosition = float3(-lightToReceiver.z, lightToReceiver.y, lightToReceiver.x);
        break;
    case ShaderPointShadowFaceNegativeX:
        facePosition = float3(lightToReceiver.z, lightToReceiver.y, -lightToReceiver.x);
        break;
    case ShaderPointShadowFacePositiveY:
        facePosition = float3(lightToReceiver.x, -lightToReceiver.z, lightToReceiver.y);
        break;
    case ShaderPointShadowFaceNegativeY:
        facePosition = float3(lightToReceiver.x, lightToReceiver.z, -lightToReceiver.y);
        break;
    case ShaderPointShadowFacePositiveZ:
        facePosition = float3(lightToReceiver.x, lightToReceiver.y, lightToReceiver.z);
        break;
    case ShaderPointShadowFaceNegativeZ:
        facePosition = float3(-lightToReceiver.x, lightToReceiver.y, -lightToReceiver.z);
        break;
    default:
        return float3(0.5f, 0.5f, -1.0f);
    }

    if (facePosition.z <= 0.0f) {
        return float3(0.5f, 0.5f, -1.0f);
    }
    const float invFaceDistance = rcp(facePosition.z);
    const float2 tileUV = facePosition.xy * ((0.5f - normalizedBorder) * invFaceDistance) + 0.5f;
    const float depth = depthScaleBias.x + depthScaleBias.y * invFaceDistance;
    return float3(tileUV, depth);
}

float EvaluatePointShadowLighting(uint shadowIndex, inout ShadowSamplingContext shadowSampling, float3 worldPosition) {
    const ShaderShadowAtlasData atlas = cameraConstants.shadowAtlas;
    [branch]
    if (shadowIndex >= atlas.numShadows) {
        return 1.0f;
    }

    const ShaderLocalShadow shadow = GetShaderLocalShadowTable(atlas.shadowTableIndex)[shadowIndex];
    // 6면 전체가 등록된 경우에만 view 테이블에 접근한다.
    [branch]
    if (shadow.numShadowViews != 6 || shadow.firstShadowView >= atlas.numShadowViews || atlas.numShadowViews - shadow.firstShadowView < 6) {
        return 1.0f;
    }

    // 조명은 현재 광원 위치를 사용하지만, 유지 중인 depth 는 기록 당시의 위치와 투영 계수로 읽는다.
    const float3 lightToReceiver = worldPosition - shadow.pointOrigin;
    const uint faceIndex = SelectPointShadowFace(lightToReceiver);
    if (faceIndex == ShaderPointShadowFaceInvalid) {
        return 1.0f;
    }

    // 선택한 면의 UV 는 원래 90도 영역인 [border, 1 - border] 안에 들어간다.
    // 방사 거리가 아닌 면의 전방 거리로 depth 를 비교하며 near/far 밖 receiver 는 제외한다.
    const float3 shadowCoord = CalcPointShadowFaceCoord(faceIndex, lightToReceiver, shadow.normalizedBorder, shadow.pointDepthScaleBias);
    if (shadowCoord.z < 0.0f || shadowCoord.z > 1.0f) {
        return 1.0f;
    }

    // Point 는 월드 축 좌표에서 UV/depth 를 직접 계산하므로 view 행렬은 읽지 않는다.
    const float4 atlasScaleBias = GetShaderLocalShadowViewTable(atlas.shadowViewTableIndex)[shadow.firstShadowView + faceIndex].atlasScaleBias;
    const float3 atlasCoord = float3(shadowCoord.xy * atlasScaleBias.xy + atlasScaleBias.zw, shadowCoord.z);
    Texture2D<float> shadowMap = GetTexture2DFloat(atlas.textureIndex);
#if SHADOW_MAP_QUALITY == 3
    const float4 rotMat = GetShadowRotationMatrix(shadowSampling);
    return SampleShadowMap(shadowMap, atlasCoord, atlas.texelSize, shadow.filterRadius, rotMat);
#else
    return SampleShadowMap(shadowMap, atlasCoord, atlas.texelSize);
#endif
}

float SampleShadowMapArray_Q0(Texture2DArray<float> shadowMap, float3 shadowCoord, uint textureSlice) {
    return shadowMap.SampleCmpLevelZero(shadowComparisonGSampler, float3(shadowCoord.xy, textureSlice), shadowCoord.z);
}

float SampleShadowMapArray_Q1(Texture2DArray<float> shadowMap, float3 shadowCoord, uint textureSlice, float2 filterOffset) {
    float shadowSum = shadowMap.SampleCmpLevelZero(shadowComparisonGSampler, float3(shadowCoord.xy, textureSlice), shadowCoord.z);
    shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonGSampler, float3(shadowCoord.xy + float2(0.0f, filterOffset.y), textureSlice), shadowCoord.z);
    shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonGSampler, float3(shadowCoord.xy + float2(0.0f, -filterOffset.y), textureSlice), shadowCoord.z);
    shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonGSampler, float3(shadowCoord.xy + float2(filterOffset.x, 0.0f), textureSlice), shadowCoord.z);
    shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonGSampler, float3(shadowCoord.xy + float2(-filterOffset.x, 0.0f), textureSlice), shadowCoord.z);
    return shadowSum * 0.2f;
}

float SampleShadowMapArray_Q2(Texture2DArray<float> shadowMap, float3 shadowCoord, uint textureSlice, float2 filterOffset) {
    float shadowSum = 0.0f;
    [unroll]
    for (int y = -1; y <= 1; ++y) {
        [unroll]
        for (int x = -1; x <= 1; ++x) {
            shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonGSampler, float3(shadowCoord.xy + float2(x, y) * filterOffset, textureSlice), shadowCoord.z);
        }
    }
    return shadowSum * (1.0f / 9.0f);
}

#if SHADOW_MAP_QUALITY == 3
float SampleShadowMapArray_Q3(Texture2DArray<float> shadowMap, float3 shadowCoord, uint textureSlice, float2 filterOffset, float4 rotMat) {
    float shadowSum = 0.0f;
    [unroll]
    for (int i = 0; i < 8; ++i) {
        const float4 kernel = poissonDisk16[i];
        const float4 rotated = rotMat.xyzw * kernel.xxww + rotMat.zwxy * kernel.yyzz;
        shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonGSampler, float3(shadowCoord.xy + filterOffset * rotated.xy, textureSlice), shadowCoord.z);
        shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonGSampler, float3(shadowCoord.xy + filterOffset * rotated.zw, textureSlice), shadowCoord.z);
    }
    return shadowSum * 0.0625f;
}
#endif

#if SHADOW_MAP_QUALITY == 3
float SampleShadowMapArray(Texture2DArray<float> shadowMap, float3 shadowCoord, uint textureSlice, float2 texelSize, float filterRadius, float4 rotMat) {
    return SampleShadowMapArray_Q3(shadowMap, shadowCoord, textureSlice, texelSize * filterRadius, rotMat);
}
#else
float SampleShadowMapArray(Texture2DArray<float> shadowMap, float3 shadowCoord, uint textureSlice, float2 texelSize) {
#if SHADOW_MAP_QUALITY == 2
    return SampleShadowMapArray_Q2(shadowMap, shadowCoord, textureSlice, texelSize);
#elif SHADOW_MAP_QUALITY == 1
    return SampleShadowMapArray_Q1(shadowMap, shadowCoord, textureSlice, texelSize);
#else
    return SampleShadowMapArray_Q0(shadowMap, shadowCoord, textureSlice);
#endif
}
#endif

// 유효한 shadow-map XYZ 영역의 가장 가까운 경계까지 거리를 구한다. 음수면 영역 밖이다.
float CalcShadowMapCoverageMargin(float normalizedBorder, float3 shadowCoord) {
    float3 margins = min(shadowCoord, 1.0f - shadowCoord);
    margins.xy -= normalizedBorder;
    margins.z *= 1.0f - 2.0f * normalizedBorder; // Z 경계에도 XY 와 같은 비율의 normalizedBorder 를 적용한다.
    return min(min(margins.x, margins.y), margins.z);
}

// firstCascadeIndex 부터 worldPosition 이 유효 XYZ 영역에 들어가는 첫 cascade 와 shadow 좌표를 얻는다.
bool FindShadowMapCascade(ShaderCSMData csm, float3 worldPosition, uint firstCascadeIndex, out uint outCascadeIndex, out float3 outShadowCoord) {
    outCascadeIndex = 0;
    outShadowCoord = 0.0f;

    [unroll(ShaderCSMCascadeMaxCount)]
    for (uint candidateIndex = 0; candidateIndex < csm.cascadeCount; candidateIndex++) {
        if (candidateIndex < firstCascadeIndex) {
            continue;
        }

        const float3 candidateCoord = mul(csm.worldToShadowCoordMatrices[candidateIndex], float4(worldPosition, 1.0f));
        if (CalcShadowMapCoverageMargin(csm.normalizedBorder, candidateCoord) < 0.0f) {
            continue;
        }

        outCascadeIndex = candidateIndex;
        outShadowCoord = candidateCoord;
        return true;
    }

    return false;
}

#if DEBUG_CASCADED_SHADOW_MAP == 2
typedef float3 ShadowLighting;
#else
typedef float ShadowLighting;
#endif

ShadowLighting SampleCascadedShadowMap(ShaderCSMData csm, inout ShadowSamplingContext shadowSampling, float3 worldPosition) {
#if DEBUG_CASCADED_SHADOW_MAP == 2
    static const float3 cascadeDebugColors[ShaderCSMCascadeMaxCount] = {
        float3(0.02f, 0.35f, 1.00f),
        float3(0.02f, 1.00f, 0.08f),
        float3(1.00f, 0.80f, 0.02f),
        float3(0.55f, 0.03f, 1.00f)
    };
#endif

    uint cascadeIndex;
    float3 shadowCoord;
    if (!FindShadowMapCascade(csm, worldPosition, 0, cascadeIndex, shadowCoord)) {
        return 1.0f;
    }

    Texture2DArray<float> shadowMapArray = GetTexture2DArrayFloat(csm.textureIndex);

#if SHADOW_MAP_QUALITY == 3
    const float4 rotMat = GetShadowRotationMatrix(shadowSampling);
    const float shadowFactor = SampleShadowMapArray(shadowMapArray, shadowCoord, cascadeIndex, csm.texelSize, csm.cascadeFilterRadii[cascadeIndex], rotMat);
#else
    const float shadowFactor = SampleShadowMapArray(shadowMapArray, shadowCoord, cascadeIndex, csm.texelSize);
#endif

    ShadowLighting shadowLighting = shadowFactor;
#if DEBUG_CASCADED_SHADOW_MAP == 2
    shadowLighting *= cascadeDebugColors[cascadeIndex];
#endif

    const float coverageMargin = CalcShadowMapCoverageMargin(csm.normalizedBorder, shadowCoord);

    // XYZ 경계 안쪽에서 blending 한다. coverageMargin >= blendWidth 면 가중치 0, 경계에서는 1 이다.
    const float blendBandWeight = csm.invBlendWidth > 0.0f ? saturate(1.0f - coverageMargin * csm.invBlendWidth) : 0.0f;

    [branch]
    if (cascadeIndex < csm.cascadeCount - 1 && blendBandWeight > 0.0f) {
        uint cascadeIndex2;
        float3 shadowCoord2;

        if (FindShadowMapCascade(csm, worldPosition, cascadeIndex + 1, cascadeIndex2, shadowCoord2)) {
#if SHADOW_MAP_QUALITY == 3
            const float shadowFactor2 = SampleShadowMapArray(shadowMapArray, shadowCoord2, cascadeIndex2, csm.texelSize, csm.cascadeFilterRadii[cascadeIndex2], rotMat);
#else
            const float shadowFactor2 = SampleShadowMapArray(shadowMapArray, shadowCoord2, cascadeIndex2, csm.texelSize);
#endif
            ShadowLighting shadowLighting2 = shadowFactor2;

#if DEBUG_CASCADED_SHADOW_MAP == 2
            shadowLighting2 *= cascadeDebugColors[cascadeIndex2];
#endif
            shadowLighting = lerp(shadowLighting, shadowLighting2, blendBandWeight);
        }
    }

    return shadowLighting;
}

ShadowLighting EvaluateDirectionalShadowLighting(inout ShadowSamplingContext shadowSampling, float3 worldPosition) {
    const ShaderCSMData csm = cameraConstants.csm;
    if (csm.enabled == 0) {
        return 1.0f;
    }

    const float viewDistance = dot(worldPosition - cameraConstants.cameraOrigin, cameraConstants.cameraForward);
    const float farFade = saturate((viewDistance - csm.farFadeStart) * csm.invFarFadeRange);
    if (farFade >= 1.0f) {
        return 1.0f;
    }

    const ShadowLighting shadowLighting = SampleCascadedShadowMap(csm, shadowSampling, worldPosition);
    return lerp(shadowLighting, 1.0f, farFade);
}

ShadowLighting EvaluateDirectionalShadowLighting(uint2 pixelCoord, float3 worldPosition) {
    ShadowSamplingContext shadowSampling = InitShadowSamplingContext(pixelCoord);
    return EvaluateDirectionalShadowLighting(shadowSampling, worldPosition);
}

#endif // __SHADOW_LIBRARY_HLSLI__
