#include "ShaderInterop.h"
#include "ColorSpaceUtility.hlsli"
#include "ToneMapping/ToneMapOperators.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), SRV(t2), UAV(u0)), " \
    "CBV(b0), " \
    "StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)"

SamplerState linearClampSampler : register(s100);

cbuffer FinalPostProcessCB : register(b0) {
    FinalPostProcessParams finalPostProcessParams;
}

Texture2D<float4> inputTexture : register(t0);
ByteAddressBuffer luminanceBuffer : register(t1);
Texture2D<float4> bloomTexture : register(t2);
RWTexture2D<float4> outputTexture : register(u0);

float InterleavedGradientNoise(uint2 pixelCoord) {
    return frac(52.9829189 * frac(dot(float2(pixelCoord), float2(0.06711056, 0.00583715))));
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FinalPostProcessBlockSizeX, FinalPostProcessBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint2 pixelCoord = dispatchThreadId.xy;
    if (pixelCoord.x >= finalPostProcessParams.outputSize.x || pixelCoord.y >= finalPostProcessParams.outputSize.y) {
        return;
    }

    float4 hdrColor = inputTexture.Load(int3(pixelCoord, 0));

    if (finalPostProcessParams.chromaticAberration > 0.0) {
        float2 uv = (float2(pixelCoord) + 0.5) * finalPostProcessParams.outputTexelSize;
        float2 centerOffsetPixels = (uv - 0.5) * float2(finalPostProcessParams.outputSize);
        float centerDistance = length(centerOffsetPixels);
        float normalizedRadius = saturate(length(uv - 0.5) * 2.0);
        float2 radialDirection = centerOffsetPixels / max(centerDistance, 0.0001);
        float2 sampleOffset = radialDirection * finalPostProcessParams.outputTexelSize * finalPostProcessParams.chromaticAberration * normalizedRadius * normalizedRadius;

        hdrColor.r = inputTexture.SampleLevel(linearClampSampler, uv - sampleOffset, 0).r;
        hdrColor.b = inputTexture.SampleLevel(linearClampSampler, uv + sampleOffset, 0).b;
    }

    if (finalPostProcessParams.bloomIntensity > 0.0) {
        float2 uv = (float2(pixelCoord) + 0.5) * finalPostProcessParams.outputTexelSize;
        hdrColor.rgb += bloomTexture.SampleLevel(linearClampSampler, uv, 0).rgb * finalPostProcessParams.bloomIntensity;
    }

    float3 color = max(hdrColor.rgb, float3(0.0, 0.0, 0.0));

    if ((finalPostProcessParams.flags & FinalPostProcessFlagToneMapping) != 0) {
        float exposure = luminanceBuffer.Load<float>(LuminanceBufferOffsetExposure);
        color = ToneMap(color * exposure, finalPostProcessParams.toneMappingOperator);
    }

    if (finalPostProcessParams.vignetteIntensity > 0.0) {
        float2 uv = (float2(pixelCoord) + 0.5) * finalPostProcessParams.outputTexelSize;
        float2 centerOffset = uv * 2.0 - 1.0;
        float vignette = smoothstep(0.25, 1.0, dot(centerOffset, centerOffset));
        color *= 1.0 - vignette * finalPostProcessParams.vignetteIntensity;
    }

    if ((finalPostProcessParams.flags & FinalPostProcessFlagDithering) != 0) {
        float dither = (InterleavedGradientNoise(pixelCoord) - 0.5) / 255.0;
        color += dither;
    }

    outputTexture[pixelCoord] = float4(saturate(color), saturate(hdrColor.a));
}
