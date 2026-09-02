#include "../ShaderInterop.h"
#include "../Common.hlsli"
#include "OceanFieldCommon.hlsli"

#define OCEAN_SURFACE_QUERY_ROOT_SIGNATURE \
    "RootFlags(0), " \
    "DescriptorTable(SRV(t0, numDescriptors = 2), UAV(u0)), " \
    "CBV(b0), " \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

DECLARE_COMMON_STATIC_SAMPLERS

cbuffer OceanSurfaceQueryCB : register(b0) {
    OceanSurfaceQueryParams queryParams;
};

// 짝수 slice 는 displacement (Dx, Dy, Dz, Dxdy), 홀수 slice 는 derivative (Dzdx, Dzdy, Dxdx, Dydy) 이다.
Texture2DArray<float4> displacementAndDerivativeMap : register(t0);
StructuredBuffer<OceanSurfaceQueryInput> surfaceQueries : register(t1);
RWStructuredBuffer<OceanSurfaceQueryResult> surfaceQueryResults : register(u0);

// sample diameter 가 차지하는 texel 크기를 계산하여 샘플링에 필요한 mip level 을 계산한다.
float GetQueryMipLevel(float patchSize, float sampleDiameter, uint fieldWidth) {
    const float diameterTexels = sampleDiameter * float(fieldWidth) / patchSize;
    return log2(max(diameterTexels, 1.0f));
}

// query 위치의 fixed-point iteration 에 사용할 수평 displacement 를 sample diameter 에 맞는 mip 에서 cascade 별로 합산한다.
float2 SampleOceanHorizontalDisplacement(float2 parameterPositionXY, float sampleDiameter, uint fieldWidth, uint maxMipLevel) {
    float2 horizontalDisplacement = 0.0f;

    [unroll]
    for (uint cascadeIndex = 0; cascadeIndex < OceanCascadeCount; ++cascadeIndex) {
        const float patchSize = queryParams.cascadePatchSizes[cascadeIndex];
        const float mipLevel = GetQueryMipLevel(patchSize, sampleDiameter, fieldWidth);
        // 샘플링할 mip level 의 해상도가 너무 낮으면 해당 cascade 를 생략한다.
        if (mipLevel >= float(maxMipLevel)) {
            continue;
        }

        const float2 uv = parameterPositionXY / patchSize;
        horizontalDisplacement += displacementAndDerivativeMap.SampleLevel(linearWrapSampler, float3(uv, float(2u * cascadeIndex)), mipLevel).xy;
    }

    return horizontalDisplacement * queryParams.displacementScale;
}

// parameterPositionXY 의 displacement 와 derivative field 를 sample diameter 에 맞는 mip 에서 cascade 별로 합산한다.
void SampleOceanFields(float2 parameterPositionXY, float sampleDiameter, uint fieldWidth, uint maxMipLevel, out float4 displacementField, out float4 derivativeField) {
    displacementField = 0.0f;
    derivativeField = 0.0f;

    [unroll]
    for (uint cascadeIndex = 0; cascadeIndex < OceanCascadeCount; ++cascadeIndex) {
        const float patchSize = queryParams.cascadePatchSizes[cascadeIndex];
        const float mipLevel = GetQueryMipLevel(patchSize, sampleDiameter, fieldWidth);
        // 샘플링할 mip level 의 해상도가 너무 낮으면 해당 cascade 를 생략한다.
        if (mipLevel >= float(maxMipLevel)) {
            continue;
        }

        const float2 uv = parameterPositionXY / patchSize;
        displacementField += displacementAndDerivativeMap.SampleLevel(linearWrapSampler, float3(uv, float(2u * cascadeIndex)), mipLevel);
        derivativeField += displacementAndDerivativeMap.SampleLevel(linearWrapSampler, float3(uv, float(2u * cascadeIndex + 1u)), mipLevel);
    }

    displacementField *= queryParams.displacementScale;
    derivativeField *= queryParams.displacementScale;
}

[RootSignature(OCEAN_SURFACE_QUERY_ROOT_SIGNATURE)]
[numthreads(OceanSurfaceQueryBlockSize, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    const uint queryIndex = dispatchThreadId.x;
    if (queryIndex >= queryParams.queryCount) {
        return;
    }

    uint fieldWidth;
    uint fieldHeight;
    uint fieldSliceCount;
    uint mipLevelCount;
    displacementAndDerivativeMap.GetDimensions(0, fieldWidth, fieldHeight, fieldSliceCount, mipLevelCount);
    uint maxMipLevel = mipLevelCount - 1;

    const OceanSurfaceQueryInput query = surfaceQueries[queryIndex];

    // query.worldPositionXY 에 대응하는 displacement 전 위치를 fixed-point iteration 으로 역산한다.
    float2 parameterPositionXY = query.worldPositionXY;
    [unroll]
    for (uint iteration = 0; iteration < OceanSurfaceQueryFixedPointIterationCount; ++iteration) {
        // worldXY = parameterXY + horizontalDisplacement(parameterXY)
        // parameterXY = worldXY - horizontalDisplacement(parameterXY)
        const float2 displacementXY = SampleOceanHorizontalDisplacement(parameterPositionXY, query.sampleDiameter, fieldWidth, maxMipLevel);
        parameterPositionXY = query.worldPositionXY - displacementXY;
    }

    float4 displacementField;
    float4 derivativeField;
    SampleOceanFields(parameterPositionXY, query.sampleDiameter, fieldWidth, maxMipLevel, displacementField, derivativeField);

    OceanSurfaceQueryResult result;
    result.surfacePosition = float3(query.worldPositionXY, queryParams.surfaceHeight + displacementField.z);
    result.queryId = query.queryId;
    result.surfaceNormal = CalculateOceanSurfaceNormal(displacementField.w, derivativeField);
    result.sampleTime = queryParams.sampleTime;
    surfaceQueryResults[queryIndex] = result;
}
