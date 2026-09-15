#include "../ShaderInterop.h"
#include "../Common.hlsli"
#include "../NormalEncoding.hlsli"

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
Texture2D<float2> inputNormalTexture : register(t1);

Texture2D<float2> inputDepthPyramidTexture : register(t2);
RWTexture2D<float4> outputTraceTexture : register(u0);

#include "ScreenSpaceRayMarching.hlsli"

// Half-resolution pixel 에 대응하는 full-resolution 2x2 영역에서 가장 가까운 depth 와 그 normal 을 선택한다.
void LoadClosestDepthAndNormal(uint2 outputPixelCoord, out uint2 inputPixelCoord, out float deviceDepth, out float3 normal) {
    const uint2 inputMin = outputPixelCoord * 2u;
    const uint2 inputMax = min(inputMin + 1u, ssrTraceParams.inputSize - 1u);
    const uint2 candidatePixelCoords[4] = {
        inputMin,
        uint2(inputMax.x, inputMin.y),
        uint2(inputMin.x, inputMax.y),
        inputMax
    };

    inputPixelCoord = candidatePixelCoords[0];
    deviceDepth = inputDepthTexture.Load(int3(inputPixelCoord, 0));

    [unroll]
    for (uint candidateIndex = 1; candidateIndex < 4; ++candidateIndex) {
        const uint2 candidatePixelCoord = candidatePixelCoords[candidateIndex];
        const float candidateDepth = inputDepthTexture.Load(int3(candidatePixelCoord, 0));
        const bool isCloser = ssrTraceParams.useMinDepth != 0 ? candidateDepth < deviceDepth : candidateDepth > deviceDepth;
        if (isCloser) {
            inputPixelCoord = candidatePixelCoord;
            deviceDepth = candidateDepth;
        }
    }

    normal = DecodeOctahedralNormal(inputNormalTexture.Load(int3(inputPixelCoord, 0)));
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

    const float farPlaneDeviceDepth = (cameraConstants.ndcFarZ - cameraConstants.ndcDepthScaleBias.y) / cameraConstants.ndcDepthScaleBias.x;

    uint2 inputPixelCoord;
    float surfaceDepth;
    float3 surfaceNormal;
    LoadClosestDepthAndNormal(outputPixelCoord, inputPixelCoord, surfaceDepth, surfaceNormal);
    // 원본 깊이에서 선택한 샘플이므로 clear 값만 배경으로 취급한다.
    if (surfaceDepth == farPlaneDeviceDepth) {
        outputTraceTexture[outputPixelCoord] = 0.0f;
        return;
    }

    const float2 uv = (float2(inputPixelCoord) + 0.5f) / ssrTraceParams.inputSize;
    const float surfaceNDCDepth = surfaceDepth * cameraConstants.ndcDepthScaleBias.x + cameraConstants.ndcDepthScaleBias.y;
    const float3 surfacePosition = ReconstructPosition(uv, surfaceNDCDepth, cameraConstants.invViewProjMatrix);
    const bool orthogonal = (cameraConstants.flags & ShaderCameraFlagProjectionOrthogonal) != 0;

    // 카메라에서 표면으로 향하는 입사 방향 벡터
    const float3 incidentDirection = orthogonal ? normalize(cameraConstants.cameraForward) : normalize(surfacePosition - cameraConstants.cameraOrigin);

    // 표면에서 반사되는 방향 벡터
    const float3 reflectionDirection = normalize(reflect(incidentDirection, surfaceNormal));

    const float3 rayOrigin = surfacePosition + surfaceNormal * ssrTraceParams.normalBias; // self-intersection 방지 offset
    const float3 rayEnd = rayOrigin + reflectionDirection * ssrTraceParams.maxDistance;
    const float4 rayOriginClip = mul(cameraConstants.viewProjMatrix, float4(rayOrigin, 1.0f));
    float4 rayEndClip = mul(cameraConstants.viewProjMatrix, float4(rayEnd, 1.0f));

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

    const float3 rayStartScreen = ClipPositionToScreenPosition(rayOriginClip, cameraConstants.ndcDepthScaleBias);
    const float3 rayEndScreen = ClipPositionToScreenPosition(rayEndClip, cameraConstants.ndcDepthScaleBias);

    const ScreenSpaceRayTraceResult traceResult = TraceScreenSpaceRayHiZ(rayStartScreen, rayEndScreen,
        ssrTraceParams.inputSize, ssrTraceParams.depthPyramidMipCount, ssrTraceParams.maxIterations,
        ssrTraceParams.binaryRefinementSteps, ssrTraceParams.thickness,
        farPlaneDeviceDepth, ssrTraceParams.useMinDepth != 0, cameraConstants.ndcDepthScaleBias,
        cameraConstants.projectionZ, orthogonal);

    outputTraceTexture[outputPixelCoord] = GetTraceDebugColor(traceResult.status);
}
