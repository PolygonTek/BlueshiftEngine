#ifndef __VFOG_COMMON_HLSLI__
#define __VFOG_COMMON_HLSLI__

#include "../ShaderInterop.h"

// 슬라이스 경계 좌표 [0, sliceCount] -> 뷰 공간 깊이
float VFogSliceToDepth(float slice, VFogParams vfogParams) {
    if (slice <= 0.0f) {
        return vfogParams.nearDepth;
    }
    if (slice >= vfogParams.sliceCount) {
        return vfogParams.farDepth;
    }
    return vfogParams.nearDepth + vfogParams.depthScale * Expm1(slice * vfogParams.exponentPerSlice * LN2);
}

// 뷰 공간 깊이 -> 슬라이스 좌표 [0, sliceCount]
float VFogDepthToSlice(float depth, VFogParams vfogParams) {
    const float x = max(depth - vfogParams.nearDepth, 0.0f) / vfogParams.depthScale;
    const float logValue = x < 0.05f ? x * (1.0f - x * (0.5f - x * (1.0f / 3.0f - x * (0.25f - x * 0.2f)))) * 1.44269504089f : log2(1.0f + x);
    return clamp(logValue * vfogParams.slicesPerExponent, 0.0f, (float)vfogParams.sliceCount);
}

// 2D 타일 좌표의 중심 UV 를 구한다.
float2 VFogTileCenterUV(uint2 tileCoord, VFogParams vfogParams) {
    return (float2(tileCoord) + 0.5f) * vfogParams.tileCountRcp;
}

// Unjittered UV 를 raster UV 로 변환해 카메라 투영의 TAA jitter 를 상쇄한다.
// 원근 투영에서는 z = -1 평면의 XY, 직교 투영에서는 깊이와 무관한 뷰 공간 XY 를 반환한다.
float2 VFogGetRayXY(float2 unjitteredUV, CameraConstants camera) {
    const float2 rasterUV = unjitteredUV + camera.uvJitter;
    return rasterUV * camera.uvToViewScaleBiasXY.xy + camera.uvToViewScaleBiasXY.zw;
}

// 뷰 공간 단위 깊이당 광선 길이를 계산한다.
float VFogRayLengthPerViewDepth(float2 unjitteredUV, CameraConstants camera) {
    if (camera.IsOrthogonal()) {
        return 1.0f;
    }
    const float2 rayXY = VFogGetRayXY(unjitteredUV, camera);
    // length(float3(rayXY, -1.0f))
    return sqrt(1.0f + dot(rayXY, rayXY));
}

// 프록셀 내부 감쇠를 고려하여 누적 산란/발광 radiance 와 투과율을 갱신한다.
float4 VFogIntegrateSegment(float4 accumulated, float4 sourceExtinction, float stepLength) {
    // Optical depth = extinction coefficient * distance
    const float tau = sourceExtinction.a * stepLength;

    // Beer-Lambert law: T = exp(-tau)
    const float stepT = exp(-tau);

    // tau 가 충분히 작으면 유효 자릿수 손실과 0/0 을 피하기 위해
    // (1 - exp(-tau)) / tau 를 테일러 전개로 근사한다.
    const float factor = tau < 1e-3f ? 1.0f - tau * (0.5f - tau / 6.0f) : (1.0f - stepT) / tau;
    const float3 stepL = sourceExtinction.rgb * (stepLength * factor);

    // Lacc_new = Lacc + Tacc * stepL(i)
    accumulated.rgb += accumulated.a * stepL;

    // Tacc_new = Tacc * stepT(i)
    accumulated.a *= stepT;
    return accumulated;
}

// fog 시작 깊이 nearDepth 부터 min(depth, farDepth) 까지의 누적 산란/발광 radiance 와 투과율을 샘플링한다.
float4 VFogSample(Texture3D<float4> integratedTexture, SamplerState linearClamp, float2 unjitteredUV, float depth, CameraConstants camera) {
    const VFogParams vfogParams = camera.vfogParams;
    if (depth <= vfogParams.nearDepth) {
        return float4(0, 0, 0, 1);
    }

    // depth 로 slice 인덱스를 구한다.
    const float clampedDepth = min(depth, vfogParams.farDepth);
    uint slice = min((uint)VFogDepthToSlice(clampedDepth, vfogParams), vfogParams.sliceCount - 1u);

    // 부동소수점 오차로 인해 slice 가 clampedDepth 보다 깊은 경우가 발생할 수 있으므로, slice 를 한 단계 낮춘다.
    if (slice > 0 && VFogSliceToDepth(slice, vfogParams) > clampedDepth) {
        --slice;
    }

    // 프록셀의 가까운 경계에서 clampedDepth 까지의 상대 깊이 (0~1) 를 구한다.
    const float nearBoundary = VFogSliceToDepth(slice, vfogParams);
    const float farBoundary = VFogSliceToDepth(slice + 1u, vfogParams);
    const float depthRange = max(farBoundary - nearBoundary, FLT_MIN);
    const float depthFraction = saturate((clampedDepth - nearBoundary) / depthRange);

    // 프록셀 앞뒤 경계의 누적값을 실제 깊이 비율에 따라 한 번의 선형 샘플링으로 보간한다.
    const float boundaryCountRcp = 1.0f / (vfogParams.sliceCount + 1.0f);
    const float sampleW = (slice + 0.5f + depthFraction) * boundaryCountRcp;
    return integratedTexture.SampleLevel(linearClamp, float3(unjitteredUV, sampleW), 0);
}

#endif // __VFOG_COMMON_HLSLI__
