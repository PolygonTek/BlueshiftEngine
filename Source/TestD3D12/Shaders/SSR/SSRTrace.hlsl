#include "../ShaderInterop.h"
#include "../Common.hlsli"
#include "../CommonBindless.hlsli"
#include "../Octahedral.hlsli"
#include "../STBN.hlsli"
#include "../PBR/ImportanceSampling.hlsli"
#include "SSRResolution.hlsli"

#define COMMON_ROOT_SIGNATURE \
    "RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | SAMPLER_HEAP_DIRECTLY_INDEXED), " \
    "DescriptorTable(SRV(t0), SRV(t1), SRV(t2), SRV(t3), SRV(t4), SRV(t5), UAV(u0), UAV(u1)), " \
    "RootConstants(num32BitConstants=16, b0), " \
    "CBV(b1), " \
    "CBV(b2), " \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

DECLARE_COMMON_STATIC_SAMPLERS

cbuffer SSRTraceCB : register(b0) {
    SSRTraceParams ssrTraceParams;
}

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
}

cbuffer FrameCB : register(b2) {
    FrameConstants frameConstants;
}

Texture2D<float> inputSceneDepthTexture : register(t0); // Opaque scene depth used to refine Hi-Z intersections.
Texture2D<float2> inputSceneVelocityTexture : register(t1); // Unjittered opaque motion vector, including object/skinning motion.
Texture2D<float> inputReceiverDepthTexture : register(t2); // Reflection receiver depth; transparent SSR uses its dedicated surface prepass.
Texture2D<float4> inputReceiverNormalRoughnessTexture : register(t3); // RG: UNORM octahedral view-space shading normal, B: perceptual roughness.
Texture2D<float2> inputSSRHiZTexture : register(t4); // Hi-Z device-depth mip chain; R: max depth, G: min depth. Mip 0 covers 2x2 full-resolution pixels.
Texture2D<float2> inputPrevDepthExtremaTexture : register(t5); // Previous-frame 2x2 blocks; R: max depth, G: min depth.

// 아래 출력 텍스처는 ssrResolutionMode 에 따라 Full(0), Half(1), Quarter(2) 해상도를 사용한다.
RWTexture2D<float4> outputTraceTexture : register(u0); // RG = history radiance UV, B = normalized iteration count in iteration debug mode (otherwise 0), A = positive confidence or a (non-positive) trace status code.
RWTexture2D<float4> outputReceiverSurfaceTexture : register(u1); // RG: FP16 screen-space ray length in pixels, B: normalized block pixel index, A: perceptual roughness.

#include "ScreenSpaceRayMarching.hlsli"

// 입력 텍스처의 1x1/2x2/4x4 블럭 영역에서 카메라에 가장 가까운 픽셀을 선택한다.
// 선택한 픽셀의 픽셀 좌표, 블럭 내부 픽셀 인덱스, device depth 를 반환한다.
void LoadClosestDepth(uint2 blockCoord, out uint2 closestPixelCoord, out uint closestBlockPixelIndex, out float closestDepth) {
    const uint2 inputCoordMin = blockCoord * SSR_RESOLUTION_DIVISOR;
    closestPixelCoord = inputCoordMin;
    closestBlockPixelIndex = 0u;
    closestDepth = inputReceiverDepthTexture.Load(int3(closestPixelCoord, 0));

#if SSR_RESOLUTION_DIVISOR > 1
    [unroll]
    for (uint candidateIndex = 1; candidateIndex < SSR_BLOCK_PIXEL_COUNT; ++candidateIndex) {
        const uint2 offset = uint2(candidateIndex % SSR_RESOLUTION_DIVISOR, candidateIndex / SSR_RESOLUTION_DIVISOR);
        const uint2 candidatePixelCoord = min(inputCoordMin + offset, ssrTraceParams.inputSize - 1u);
        const float candidateDepth = inputReceiverDepthTexture.Load(int3(candidatePixelCoord, 0));
        const bool isCloser = ssrTraceParams.reverseZ != 0 ? candidateDepth > closestDepth : candidateDepth < closestDepth;
        if (isCloser) {
            closestPixelCoord = candidatePixelCoord;
            closestBlockPixelIndex = candidateIndex;
            closestDepth = candidateDepth;
        }
    }
#endif
}

float3 ComputeReflectionDirection(uint2 pixelCoord, float3 incidentDir, float3 viewNormal, float roughness) {
    const float linearRoughness = roughness * roughness;
    const float rayLinearRoughness = linearRoughness * saturate(ssrTraceParams.rayRoughnessScale);
    if (rayLinearRoughness <= 1.0e-4f) {
        return normalize(reflect(incidentDir, viewNormal));
    }

    // view space 의 view 벡터를 local tangent space 로 변환한다.
    const float3x3 localFrame = GetLocalFrame(viewNormal);
    const float3 viewDirTS = mul(localFrame, -incidentDir);

    // STBNVec2 텍스처에서 샘플링한 난수 벡터 [0, 1] 를 얻는다.
    float2 xi = ReadSTBNVec2(GetTexture2DArray(frameConstants.stbnVec2TextureIndex), pixelCoord, ssrTraceParams.noiseIndex);

    // STBNVec2 는 R8G8 UNORM 포맷이므로 각 성분을 양자화 구간의 중앙으로 옮겨 샘플 값이 정확히 0 또는 1 이 되지 않도록 한다.
    xi = (xi * 255.0f + 0.5f) / 256.0f;

    const float3 halfVectorTS = ImportanceSampleGGXVNDF(xi, viewDirTS, rayLinearRoughness);
    const float3 halfVectorVS = normalize(mul(halfVectorTS, localFrame));
    return normalize(reflect(incidentDir, halfVectorVS));
}

// view-space ray 의 방향을 screen UV/device depth 공간의 방향으로 변환한다. 단위 벡터일 필요는 없다.
float3 ComputeScreenRayDirection(float3 rayStartScreen, float3 rayDirView, bool orthogonal) {
    // P(s) = P0 + sD, D = rayDirView, w = -P.z 이므로 w' = -D.z 이다.
    // S/B = projectionScaleBiasXY.xy/zw, A/C = projectionScaleBiasZ.x/y 라 두면,
    // clip.xy = P.xy * S + w * B, clip.z = A * P.z + C, ndc = clip.xyz / w 이다.
    // 몫의 미분법: ndc' = (clip.xyz' - ndc * w') / w.
    // 따라서 ndc.xy' = (D.xy * S + D.z * (ndc.xy - B)) / w, depth' = D.z * (A + depth) / w.
    // 카메라 앞에서는 w > 0 이므로 공통 배율 1 / w 를 생략해도 방향은 같고, 마칭 t 의 배율만 달라진다.
    // 직교 투영은 w = 1, w' = 0 이므로 원근 보정 항을 제외한다.
    // 두 위치의 투영값을 빼지 않아 원거리의 상쇄 오차와 카메라 뒤 보조 지점의 투영을 피한다.
    const float2 ndcXY = rayStartScreen.xy * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    const float2 perspectiveOffset = orthogonal ? 0.0f : ndcXY - cameraConstants.projectionScaleBiasXY.zw;
    const float depthScale = cameraConstants.projectionScaleBiasZ.x + (orthogonal ? 0.0f : rayStartScreen.z);
    return float3((rayDirView.xy * cameraConstants.projectionScaleBiasXY.xy + rayDirView.z * perspectiveOffset) * float2(0.5f, -0.5f), rayDirView.z * depthScale);
}

// prevPixelCoord : 이전 프레임 화면의 (반해상도) 좌표.
// prevDepth : 물체가 정지했다고 가정해 추정한 입력 지점의 이전 프레임 깊이.
bool IsPrevDepthCompatible(uint2 prevPixelCoord, float prevDepth) {
    const float2 prevDepthExtrema = inputPrevDepthExtremaTexture.Load(int3(prevPixelCoord, 0));
    if (prevDepthExtrema.y > prevDepthExtrema.x || !all(isfinite(prevDepthExtrema))) {
        return false;
    }

    // 추정 깊이와 이전 프레임 2x2 깊이 범위 사이의 오차를 선형 깊이로 비교한다.
    // 오차는 추정 선형 깊이의 1% 까지 허용한다.
    const float clampedDepth = clamp(prevDepth, prevDepthExtrema.y, prevDepthExtrema.x);
    const float prevLinearDepth = cameraConstants.GetLinearDepth(prevDepth);
    const float clampedLinearDepth = cameraConstants.GetLinearDepth(clampedDepth);
    const float relativeDepthTolerance = 0.01f;
    return isfinite(prevLinearDepth) && isfinite(clampedLinearDepth) &&
           prevLinearDepth > 0.0f && clampedLinearDepth > 0.0f &&
           abs(prevLinearDepth - clampedLinearDepth) <= prevLinearDepth * relativeDepthTolerance;
}

// velocity 와 jitter 변화량으로 uv 를 이전 프레임의 jittered UV 로 재투영한다.
// uv 와 depth 로 계산한 입력 지점의 예상 이전 프레임 깊이를 이용해 재투영 결과를 검증한다.
// UV 또는 깊이가 유효하지 않거나 깊이 검증에 실패하면 false 를 반환한다.
bool ReprojectUV(float2 uv, float depth, out float2 prevUV) {
    const float3 worldPos = ReconstructWorldPosition(uv, depth, cameraConstants.invViewProjMatrix);
    const float4 prevClipPos = mul(cameraConstants.prevViewProjMatrix, float4(worldPos, 1.0f));
    if (prevClipPos.w <= 0.0f) {
        prevUV = 0.0f;
        return false;
    }

    // expectedPrevDepth 는 물체가 정지했다고 가정해 계산한 입력 지점의 예상 이전 프레임 깊이이다.
    const float3 prevNDC = prevClipPos.xyz / prevClipPos.w;
    const float expectedPrevDepth = prevNDC.z;

    // prevUV 는 velocity 로 물체의 이동까지 반영한 입력 지점의 이전 프레임 UV 이다.
    const uint2 pixelCoord = min(uint2(uv * float2(ssrTraceParams.inputSize)), ssrTraceParams.inputSize - 1u);
    const float2 velocity = inputSceneVelocityTexture.Load(int3(pixelCoord, 0));
    prevUV = uv - velocity - ssrTraceParams.uvJitterDelta; // uv 는 jittered UV 이므로, 이전 프레임의 jittered UV 를 얻으려면 uvJitterDelta 를 빼야 한다.

    // 이전 프레임 UV 나 예상 깊이가 NaN/Inf 이거나 [0, 1] 범위를 벗어나면 재투영을 거부한다.
    if (!all(isfinite(prevUV)) || any(prevUV < 0.0f) || any(prevUV > 1.0f) || !isfinite(expectedPrevDepth) || expectedPrevDepth < 0.0f || expectedPrevDepth > 1.0f) {
        return false;
    }

    // 입력 지점의 예상 이전 프레임 깊이가 재투영 위치의 이전 프레임 depth 허용 범위에 포함되는지 검사한다.
    // IsPrevDepthCompatible 에 넘기는 픽셀 좌표는 절반 해상도 기준이다.
    const uint2 prevPixelCoord = min(uint2(prevUV * float2(ssrTraceParams.inputSize)), ssrTraceParams.inputSize - 1u);
    return IsPrevDepthCompatible(prevPixelCoord >> 1, expectedPrevDepth);
}

float ComputeThicknessConfidence(float hitDepthDelta) {
    // thickness 경계에서 잘못된 깊이 교차가 갑자기 유효해지지 않도록 부드럽게 감쇠한다.
    if (ssrTraceParams.thickness > 0.0f) {
        return 1.0f - smoothstep(0.0f, ssrTraceParams.thickness, hitDepthDelta);
    }
    return 1.0f;
}

float ComputeRoughnessConfidence(float roughness) {
    const float fadeRange = max(ssrTraceParams.roughnessFadeEnd - ssrTraceParams.roughnessFadeStart, 1.0e-4f);
    const float fade = (roughness - ssrTraceParams.roughnessFadeStart) / fadeRange;
    //return 1.0f - saturate(fade);
    return 1.0f - smoothstep(0.0f, 1.0f, fade);
}

float ComputeScreenEdgeConfidence(float2 uv) {
    if (ssrTraceParams.screenEdgeFade <= 0.0f) {
        return 1.0f;
    }

    // 세로 화면 비율 (1.0) 을 기준으로 세로와 가로 fade 폭을 계산한다.
    const float aspectScaleX = float(ssrTraceParams.inputSize.y) / float(ssrTraceParams.inputSize.x);
    const float2 fadeWidth = min(ssrTraceParams.screenEdgeFade * float2(aspectScaleX, 1.0f), 0.5f);
    const float2 edgeDistance = min(uv, 1.0f - uv);
    const float2 borderConfidence = smoothstep(0.0f, fadeWidth, edgeDistance);
    return borderConfidence.x * borderConfidence.y;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(SSRTraceBlockSizeX, SSRTraceBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 outputPixelCoord = dispatchThreadId.xy;
    if (any(outputPixelCoord >= ssrTraceParams.outputSize)) {
        return;
    }

    // 출력 픽셀 좌표에 대응하는 receiver 표면 블럭 중 가장 가까운 픽셀 좌표와 블럭 내부 픽셀 인덱스 그리고 표면의 깊이를 가져온다.
    uint2 inputPixelCoord;
    uint inputBlockPixelIndex;
    float receiverDepth;
    LoadClosestDepth(outputPixelCoord, inputPixelCoord, inputBlockPixelIndex, receiverDepth);

    if (receiverDepth == cameraConstants.depthClearValue) {
        outputTraceTexture[outputPixelCoord] = float4(0.0f, 0.0f, 0.0f, SSRTraceValidityNoSurface);
        outputReceiverSurfaceTexture[outputPixelCoord] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        return;
    }

    // outputReceiverSurfaceTexture 의 B = normalized block pixel index, A = perceptual roughness 를 기록한다.
    const float4 normalRoughness = inputReceiverNormalRoughnessTexture.Load(int3(inputPixelCoord, 0));
    const float roughness = normalRoughness.z;
    const float encodedInputBlockPixelIndex = EncodeSSRBlockPixelIndex(inputBlockPixelIndex);
    outputReceiverSurfaceTexture[outputPixelCoord] = float4(0.0f, 0.0f, encodedInputBlockPixelIndex, roughness);

    // roughnessFadeEnd 이상인 roughness 를 갖는 표면은 confidence 가 0 이므로 ray march 자체를 생략한다.
    if (roughness >= ssrTraceParams.roughnessFadeEnd) {
        // outputTraceTexture 의 B = 0 (레이 마칭 생략), A = trace 상태 코드 (roughness rejected)
        outputTraceTexture[outputPixelCoord] = float4(0.0f, 0.0f, 0.0f, SSRTraceValidityRoughnessRejected);
        return;
    }

    // receiver 표면에서의 view normal 과 view position 을 구한다.
    const bool orthogonal = cameraConstants.IsOrthogonal();
    const float linearDepth = cameraConstants.GetLinearDepth(receiverDepth);

    // 카메라 전방의 유한한 표면에서만 레이를 시작한다.
    if (linearDepth <= 0.0f || !isfinite(linearDepth)) {
        outputTraceTexture[outputPixelCoord] = float4(0.0f, 0.0f, 0.0f, SSRTraceValidityMiss);
        return;
    }

    const float3 viewNormal = DecodeUnitVectorOctahedralUNorm(normalRoughness.xy);
    const float2 inputUV = (float2(inputPixelCoord) + 0.5f) / ssrTraceParams.inputSize;
    const float3 viewPosition = cameraConstants.GetViewPosition(inputUV, linearDepth);

    // receiver 표면에서의 입사 방향과 반사 방향을 구한다.
    const float3 incidentDir = orthogonal ? float3(0.0f, 0.0f, -1.0f) : normalize(viewPosition);
    const float3 reflectionDir = ComputeReflectionDirection(inputPixelCoord, incidentDir, viewNormal, roughness);

    // GGX 미세면 법선으로 반사한 방향은 receiver 접평면 아래로 향할 수 있다.
    // receiver 법선 기준으로 접평면과 평행하거나 표면 아래로 향하는 방향은 ray march 를 생략하고 miss 로 처리한다.
    if (dot(reflectionDir, viewNormal) <= 0.0f) {
        outputTraceTexture[outputPixelCoord] = float4(0.0f, 0.0f, 0.0f, SSRTraceValidityMiss);
        return;
    }

    // 표면 위치에서 출발하며, self-intersection 은 레이 마칭의 출발 cell 생략으로 방지한다.
    const float3 rayStartScreen = float3(inputUV, receiverDepth);
    const float3 rayDirScreen = ComputeScreenRayDirection(rayStartScreen, reflectionDir, orthogonal);

    // 레이 마칭
    const ScreenSpaceRayTraceResult traceResult = TraceScreenSpaceRayHiZ(rayStartScreen, rayDirScreen, ssrTraceParams.inputSize, ssrTraceParams.ssrHiZMipCount,
        ssrTraceParams.maxIterations, ssrTraceParams.thickness, cameraConstants.depthClearValue, ssrTraceParams.reverseZ != 0, cameraConstants.projectionScaleBiasZ, orthogonal);

    float2 prevHitUV = traceResult.hitUV;
    float traceValidity = SSRTraceValidityMiss;

    if (traceResult.status == ScreenSpaceRayTraceStatusHit) {
        float confidence = ComputeThicknessConfidence(traceResult.hitDepthDelta) * ComputeRoughnessConfidence(roughness);

        // 이전 프레임으로 재투영된 유효한 hitUV 를 계산한다.
        if (ReprojectUV(traceResult.hitUV, traceResult.hitDepth, prevHitUV)) {
            confidence *= min(ComputeScreenEdgeConfidence(traceResult.hitUV), ComputeScreenEdgeConfidence(prevHitUV));
            traceValidity = confidence <= 0.0f ? SSRTraceValidityConfidenceRejected : confidence;
        } else {
            traceValidity = SSRTraceValidityHistoryRejected;
        }
    } else if (traceResult.status == ScreenSpaceRayTraceStatusMaxIterations) {
        traceValidity = SSRTraceValidityMaxIterations;
    }

    // 이터레이션 디버그 모드에서만 이터레이션 횟수를 0~1 범위로 인코딩한다.
    const float iterationRatio = ssrTraceParams.debugMode == SSRDebugModeIterations ? float(traceResult.iterationCount) / float(ssrTraceParams.maxIterations) : 0.0f;

    // RG = history radiance UV, B = normalized iteration count in iteration debug mode (otherwise 0), A = positive confidence or a (non-positive) trace status code.
    // prevHitUV 를 기록하는 이유는 이전 프레임의 color (radiance) 를 샘플링해야 하기 때문이다. (SSRRadianceResolve.hlsl)
    outputTraceTexture[outputPixelCoord] = float4(prevHitUV, iterationRatio, traceValidity);

    float2 packedRayLength = 0.0f;
    if (traceValidity > 0.0f) {
        const float2 receiverBlockCenterPixels = min(float2(outputPixelCoord * SSR_RESOLUTION_DIVISOR) + float(SSR_RESOLUTION_DIVISOR) * 0.5f, float2(ssrTraceParams.inputSize) - 0.5f);
        const float2 hitPositionPixels = traceResult.hitUV * float2(ssrTraceParams.inputSize);
        const float rayLengthPixels = length(hitPositionPixels - receiverBlockCenterPixels);

        // outputReceiverSurfaceTexture 의 RG 에 픽셀 단위 레이의 길이를 FP16 으로 인코딩하여 저장한다.
        packedRayLength = PackFP16ToRG8UNorm(rayLengthPixels);
    }

    // RG = ray length, B = normalized block pixel index, A = perceptual roughness
    outputReceiverSurfaceTexture[outputPixelCoord] = float4(packedRayLength, encodedInputBlockPixelIndex, roughness);
}
