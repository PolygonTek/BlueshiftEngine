#include "../ShaderInterop.h"
#include "../ColorSpaceUtility.hlsli"
#include "ToneMapOperators.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), SRV(t2), UAV(u0)), " \
    "CBV(b0), " \
    "StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)"

SamplerState linearClampSampler : register(s100);

cbuffer ToneMappingCB : register(b0) {
    ToneMappingParams toneMappingParams;
}

Texture2D<float4> inputTexture : register(t0);
ByteAddressBuffer luminanceBuffer : register(t1);
Texture2D<float4> bloomTexture : register(t2);
RWTexture2D<float4> outputTexture : register(u0);

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(ToneMappingBlockSizeX, ToneMappingBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint2 pixelCoord = dispatchThreadId.xy;
    if (pixelCoord.x >= toneMappingParams.outputSize.x || pixelCoord.y >= toneMappingParams.outputSize.y) {
        return;
    }

    float4 hdrColor = inputTexture.Load(int3(pixelCoord, 0));

    if (toneMappingParams.bloomIntensity > 0.0) {
        float2 uv = (float2(pixelCoord) + 0.5) * toneMappingParams.outputTexelSize;
        hdrColor.rgb += bloomTexture.SampleLevel(linearClampSampler, uv, 0).rgb * toneMappingParams.bloomIntensity;
    }

    float exposure = luminanceBuffer.Load<float>(LuminanceBufferOffsetExposure) * toneMappingParams.exposureScale;
    float3 color = max(hdrColor.rgb, float3(0.0, 0.0, 0.0)) * exposure;

    color = saturate(ToneMap(color, toneMappingParams.toneMappingOperator));

    outputTexture[pixelCoord] = float4(color, saturate(hdrColor.a));
}
