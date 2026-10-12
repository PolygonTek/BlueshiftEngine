#include "../ShaderInterop.h"
#include "../Common.hlsli"
#include "../Octahedral.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), SRV(t2), UAV(u0)), " \
    "RootConstants(num32BitConstants=11, b0), " \
    "CBV(b1)"

cbuffer SSRTraceCB : register(b0) {
    SSRTraceParams ssrTraceParams;
}

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
}

Texture2D<float> inputDepthTexture : register(t0);
Texture2D<float2> inputNormalTexture : register(t1); // Octahedral view-space shading normal.
Texture2D<float2> inputDepthPyramidTexture : register(t2);
RWTexture2D<float4> outputTraceTexture : register(u0);

#include "ScreenSpaceRayMarching.hlsli"

// blockCoord 에 대응하는 입력 텍스처의 2x2 영역에서 카메라에 가장 가까운 픽셀을 선택한다.
// 선택한 픽셀의 입력 텍스처 좌표, device depth, 디코딩한 뷰 공간 shading normal 을 반환한다.
void LoadClosestDepthAndNormal(uint2 blockCoord, out uint2 closestPixelCoord, out float deviceDepth, out float3 viewNormal) {
    const uint2 inputMin = blockCoord * 2u;
    const uint2 inputMax = min(inputMin + 1u, ssrTraceParams.inputSize - 1u);
    const uint2 candidatePixelCoords[4] = {
        inputMin,
        uint2(inputMax.x, inputMin.y),
        uint2(inputMin.x, inputMax.y),
        inputMax
    };

    closestPixelCoord = candidatePixelCoords[0];
    deviceDepth = inputDepthTexture.Load(int3(closestPixelCoord, 0));

    [unroll]
    for (uint candidateIndex = 1; candidateIndex < 4; ++candidateIndex) {
        const uint2 candidatePixelCoord = candidatePixelCoords[candidateIndex];
        const float candidateDepth = inputDepthTexture.Load(int3(candidatePixelCoord, 0));
        const bool isCloser = ssrTraceParams.useMinDepth != 0 ? candidateDepth < deviceDepth : candidateDepth > deviceDepth;
        if (isCloser) {
            closestPixelCoord = candidatePixelCoord;
            deviceDepth = candidateDepth;
        }
    }

    viewNormal = DecodeUnitVectorOctahedral(inputNormalTexture.Load(int3(closestPixelCoord, 0)));
}

float4 GetTraceDebugColor(uint status) {
    if (status == ScreenSpaceRayTraceStatusHit) {
        return float4(0.0f, 1.0f, 0.0f, 1.0f);
    }
    if (status == ScreenSpaceRayTraceStatusMaxIterations) {
        return float4(0.0f, 0.0f, 1.0f, 1.0f);
    }
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(SSRTraceBlockSizeX, SSRTraceBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint2 outputPixelCoord = dispatchThreadId.xy;
    if (any(outputPixelCoord >= ssrTraceParams.outputSize)) {
        return;
    }

    const float farPlaneDeviceDepth = (cameraConstants.ndcFarZ - cameraConstants.deviceDepthToNdcZScaleBias.y) / cameraConstants.deviceDepthToNdcZScaleBias.x;

    uint2 inputPixelCoord;
    float surfaceDepth;
    float3 viewNormal;
    LoadClosestDepthAndNormal(outputPixelCoord, inputPixelCoord, surfaceDepth, viewNormal);

    // 원본 깊이에서 선택한 샘플이므로 clear 값만 배경으로 취급한다.
    if (surfaceDepth == farPlaneDeviceDepth) {
        outputTraceTexture[outputPixelCoord] = 0.0f;
        return;
    }

    const float2 uv = (float2(inputPixelCoord) + 0.5f) / ssrTraceParams.inputSize;
    const bool orthogonal = (cameraConstants.flags & ShaderCameraFlagProjectionOrthogonal) != 0;
    const float linearDepth = ToLinearDepth(surfaceDepth, cameraConstants.deviceDepthToNdcZScaleBias, cameraConstants.projectionScaleBiasZ, orthogonal);
    const float3 viewPosition = ReconstructViewPosition(uv, linearDepth, cameraConstants.uvToViewScaleBiasXY, orthogonal);

    // 카메라에서 표면으로 향하는 입사 방향 벡터. 오른손 뷰 공간의 전방은 -Z 이다.
    const float3 incidentDir = orthogonal ? float3(0.0f, 0.0f, -1.0f) : normalize(viewPosition);

    // 표면에서 반사되는 방향 벡터
    const float3 reflectDir = normalize(reflect(incidentDir, viewNormal));

    const float3 rayOrigin = viewPosition + viewNormal * ssrTraceParams.normalBias; // self-intersection 방지 offset (world 단위 유지)
    const float3 rayEnd = rayOrigin + reflectDir * ssrTraceParams.maxDistance;

    // Depth 생성과 같은 jitter 를 포함한 투영 상수를 사용한다.
    const float4 rayOriginClip = ProjectViewPosition(rayOrigin, cameraConstants.projectionScaleBiasXY, cameraConstants.projectionScaleBiasZ, orthogonal);
    float4 rayEndClip = ProjectViewPosition(rayEnd, cameraConstants.projectionScaleBiasXY, cameraConstants.projectionScaleBiasZ, orthogonal);

    // ray origin 이 카메라 평면에 너무 가깝거나 뒤에 있으면 Miss 로 처리한다.
    const float minClipW = 0.0001f;
    if (rayOriginClip.w <= minClipW) {
        outputTraceTexture[outputPixelCoord] = GetTraceDebugColor(ScreenSpaceRayTraceStatusMiss);
        return;
    }

    // ray end 가 카메라 평면 뒤에 있으면 clip.w == minClipW 이 되는 지점까지 ray 를 자른다.
    if (rayEndClip.w <= minClipW) {
        // ray 의 W 성분이 minClipW 에 도달하는 t 를 구한다.
        // rayW(t) = originW + t * (endW - originW) = minClipW
        // t = (minClipW - originW) / (endW - originW)
        const float clipT = saturate((minClipW - rayOriginClip.w) / (rayEndClip.w - rayOriginClip.w));
        rayEndClip = lerp(rayOriginClip, rayEndClip, clipT);
    }

    const float3 rayStartScreen = ClipPositionToScreenPosition(rayOriginClip, cameraConstants.deviceDepthToNdcZScaleBias);
    const float3 rayEndScreen = ClipPositionToScreenPosition(rayEndClip, cameraConstants.deviceDepthToNdcZScaleBias);

    const ScreenSpaceRayTraceResult traceResult = TraceScreenSpaceRayHiZ(rayStartScreen, rayEndScreen,
        ssrTraceParams.inputSize, ssrTraceParams.depthPyramidMipCount, ssrTraceParams.maxIterations,
        ssrTraceParams.binaryRefinementSteps, ssrTraceParams.thickness,
        farPlaneDeviceDepth, ssrTraceParams.useMinDepth != 0, cameraConstants.deviceDepthToNdcZScaleBias,
        cameraConstants.projectionScaleBiasZ, orthogonal);

    outputTraceTexture[outputPixelCoord] = GetTraceDebugColor(traceResult.status);
}
