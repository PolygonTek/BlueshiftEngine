#ifndef __SHADOW_LIBRARY_HLSLI__
#define __SHADOW_LIBRARY_HLSLI__

#include "../ShaderInterop.h"

#ifndef SHADOW_MAP_QUALITY
#define SHADOW_MAP_QUALITY 2
#endif

// 0: Off, 1: Visibility, 2: Coverage.
#ifndef DEBUG_CASCADED_SHADOW_MAP
#define DEBUG_CASCADED_SHADOW_MAP 0
#endif

#if SHADOW_MAP_QUALITY == 3
static const float4 poissonDisk16[8] = {
    float4(-0.94201624f, -0.39906216f, +0.94558609f, -0.76890725f),
    float4(-0.09418410f, -0.92938870f, +0.34495938f, +0.29387760f),
    float4(-0.91588581f, +0.45771432f, -0.81544232f, -0.87912464f),
    float4(-0.38277543f, +0.27676845f, +0.97484398f, +0.75648379f),
    float4(+0.44323325f, -0.97511554f, +0.53742981f, -0.47373420f),
    float4(-0.26496911f, -0.41893023f, +0.79197514f, +0.19090188f),
    float4(-0.24188840f, +0.99706507f, -0.81409955f, +0.91437590f),
    float4(+0.19984126f, +0.78641367f, +0.14383161f, -0.14100790f)
};

float4 GetShadowRotationMatrix(uint2 pixelCoord) {
    const float2 rotationTexCoord = (float2(pixelCoord) + 0.5f) * (1.0f / 64.0f);
    const float2 cs = GetTexture2D(frameConstants.randomRotMatTextureIndex).SampleLevel(pointWrapSampler, rotationTexCoord, 0.0f).rg * 2.0f - 1.0f;
    return float4(cs.x, -cs.y, cs.y, cs.x); // column-major rotation matrix
}
#endif

float SampleShadowMap_Q0(Texture2D<float> shadowMap, float3 shadowCoord) {
    return shadowMap.SampleCmpLevelZero(shadowComparisonSampler, shadowCoord.xy, shadowCoord.z);
}

float SampleShadowMap_Q1(Texture2D<float> shadowMap, float3 shadowCoord, float2 filterOffset) {
    float shadowSum = shadowMap.SampleCmpLevelZero(shadowComparisonSampler, shadowCoord.xy, shadowCoord.z);
    shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonSampler, shadowCoord.xy + float2(0.0f, filterOffset.y), shadowCoord.z);
    shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonSampler, shadowCoord.xy + float2(0.0f, -filterOffset.y), shadowCoord.z);
    shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonSampler, shadowCoord.xy + float2(filterOffset.x, 0.0f), shadowCoord.z);
    shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonSampler, shadowCoord.xy + float2(-filterOffset.x, 0.0f), shadowCoord.z);
    return shadowSum * 0.2f;
}

float SampleShadowMap_Q2(Texture2D<float> shadowMap, float3 shadowCoord, float2 filterOffset) {
    float shadowSum = 0.0f;
    [unroll]
    for (int y = -1; y <= 1; ++y) {
        [unroll]
        for (int x = -1; x <= 1; ++x) {
            shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonSampler, shadowCoord.xy + float2(x, y) * filterOffset, shadowCoord.z);
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
        shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonSampler, shadowCoord.xy + filterOffset * rotated.xy, shadowCoord.z);
        shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonSampler, shadowCoord.xy + filterOffset * rotated.zw, shadowCoord.z);
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

float EvaluateSpotShadowLighting(uint shadowIndex, uint2 pixelCoord, float3 worldPosition) {
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
    // border 는 필터를 위한 caster 영역이다. receiver 는 원래 조명 영역과 정방향 depth [0, 1] 안에 있어야 한다.
    if (any(shadowCoord.xy < shadow.normalizedBorder) || any(shadowCoord.xy > 1.0f - shadow.normalizedBorder) || shadowCoord.z < 0.0f || shadowCoord.z > 1.0f) {
        return 1.0f;
    }

    const float3 atlasCoord = float3(shadowCoord.xy * view.atlasScaleBias.xy + view.atlasScaleBias.zw, shadowCoord.z);
    Texture2D<float> shadowMap = GetTexture2DFloat(atlas.textureIndex);
#if SHADOW_MAP_QUALITY == 3
    const float4 rotMat = GetShadowRotationMatrix(pixelCoord);
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

// CubeMapFaceToViewAxis + 정방향 90도 투영 + tile UV 변환과 같은 좌표를 만든다.
// Atlas 는 최종 Y 반전 blit 없이 기록하므로 Cubemap.hlsli 의 cubemap UV 와 세로 방향이 반대다.
// facePosition 은 (right, down, forward). Depth 는 radial distance 가 아닌 forward distance 로 비교한다.
// normalizedBorder = borderTexels / tileResolution.
// depthScaleBias = (A, B), A = far / (far - near), B = -near * A.
// 0 < near < far 인 유한한 원근 투영이며 near 에서 depth 0, far 에서 1 이다.
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

float EvaluatePointShadowLighting(uint shadowIndex, uint2 pixelCoord, float3 lightToReceiver, float2 depthScaleBias) {
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

    const uint faceIndex = SelectPointShadowFace(lightToReceiver);
    if (faceIndex == ShaderPointShadowFaceInvalid) {
        return 1.0f;
    }

    // 선택한 면의 UV 는 원래 90도 영역인 [border, 1 - border] 안에 들어간다.
    // 방사 거리가 아닌 면의 전방 거리로 depth 를 비교하며 near/far 밖 receiver 는 제외한다.
    const float3 shadowCoord = CalcPointShadowFaceCoord(faceIndex, lightToReceiver, shadow.normalizedBorder, depthScaleBias);
    if (shadowCoord.z < 0.0f || shadowCoord.z > 1.0f) {
        return 1.0f;
    }

    // Point 는 월드 축 좌표에서 UV/depth 를 직접 계산하므로 view 행렬은 읽지 않는다.
    const float4 atlasScaleBias = GetShaderLocalShadowViewTable(atlas.shadowViewTableIndex)[shadow.firstShadowView + faceIndex].atlasScaleBias;
    const float3 atlasCoord = float3(shadowCoord.xy * atlasScaleBias.xy + atlasScaleBias.zw, shadowCoord.z);
    Texture2D<float> shadowMap = GetTexture2DFloat(atlas.textureIndex);
#if SHADOW_MAP_QUALITY == 3
    const float4 rotMat = GetShadowRotationMatrix(pixelCoord);
    return SampleShadowMap(shadowMap, atlasCoord, atlas.texelSize, shadow.filterRadius, rotMat);
#else
    return SampleShadowMap(shadowMap, atlasCoord, atlas.texelSize);
#endif
}

float SampleShadowMapArray_Q0(Texture2DArray<float> shadowMap, float3 shadowCoord, uint textureSlice) {
    return shadowMap.SampleCmpLevelZero(shadowComparisonSampler, float3(shadowCoord.xy, textureSlice), shadowCoord.z);
}

float SampleShadowMapArray_Q1(Texture2DArray<float> shadowMap, float3 shadowCoord, uint textureSlice, float2 filterOffset) {
    float shadowSum = shadowMap.SampleCmpLevelZero(shadowComparisonSampler, float3(shadowCoord.xy, textureSlice), shadowCoord.z);
    shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonSampler, float3(shadowCoord.xy + float2(0.0f, filterOffset.y), textureSlice), shadowCoord.z);
    shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonSampler, float3(shadowCoord.xy + float2(0.0f, -filterOffset.y), textureSlice), shadowCoord.z);
    shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonSampler, float3(shadowCoord.xy + float2(filterOffset.x, 0.0f), textureSlice), shadowCoord.z);
    shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonSampler, float3(shadowCoord.xy + float2(-filterOffset.x, 0.0f), textureSlice), shadowCoord.z);
    return shadowSum * 0.2f;
}

float SampleShadowMapArray_Q2(Texture2DArray<float> shadowMap, float3 shadowCoord, uint textureSlice, float2 filterOffset) {
    float shadowSum = 0.0f;
    [unroll]
    for (int y = -1; y <= 1; ++y) {
        [unroll]
        for (int x = -1; x <= 1; ++x) {
            shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonSampler, float3(shadowCoord.xy + float2(x, y) * filterOffset, textureSlice), shadowCoord.z);
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
        shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonSampler, float3(shadowCoord.xy + filterOffset * rotated.xy, textureSlice), shadowCoord.z);
        shadowSum += shadowMap.SampleCmpLevelZero(shadowComparisonSampler, float3(shadowCoord.xy + filterOffset * rotated.zw, textureSlice), shadowCoord.z);
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

    [unroll(ShaderCSMCascadeCount)]
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

ShadowLighting SampleCascadedShadowMap(ShaderCSMData csm, uint2 pixelCoord, float3 worldPosition) {
#if DEBUG_CASCADED_SHADOW_MAP == 2
    static const float3 cascadeDebugColors[ShaderCSMCascadeCount] = {
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
    const float4 rotMat = GetShadowRotationMatrix(pixelCoord);
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

ShadowLighting EvaluateDirectionalShadowLighting(uint2 pixelCoord, float3 worldPosition) {
    const ShaderCSMData csm = cameraConstants.csm;
    if (csm.enabled == 0) {
        return 1.0f;
    }

    const float viewDistance = dot(worldPosition - cameraConstants.cameraOrigin, cameraConstants.cameraForward);
    const float farFade = saturate((viewDistance - csm.farFadeStart) * csm.invFarFadeRange);
    if (farFade >= 1.0f) {
        return 1.0f;
    }

    const ShadowLighting shadowLighting = SampleCascadedShadowMap(csm, pixelCoord, worldPosition);
    return lerp(shadowLighting, 1.0f, farFade);
}

#endif // __SHADOW_LIBRARY_HLSLI__
