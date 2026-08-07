#include "Common.hlsli"
#include "ToneMapping/ToneMapOperators.hlsli"

#define COMMON_ROOT_SIGNATURE "RootFlags(0), " \
    "DescriptorTable(SRV(t0), SRV(t1), SRV(t2), SRV(t3), SRV(t4), UAV(u0)), " \
    "CBV(b0), " \
    "StaticSampler(s100, addressU = TEXTURE_ADDRESS_CLAMP, addressV = TEXTURE_ADDRESS_CLAMP, addressW = TEXTURE_ADDRESS_CLAMP, filter = FILTER_MIN_MAG_MIP_LINEAR)"

SamplerState linearClampSampler : register(s100);

cbuffer FinalPostProcessCB : register(b0) {
    FinalPostProcessParams finalPostProcessParams;
}

Texture2D<float4> inputTexture : register(t0);
ByteAddressBuffer luminanceBuffer : register(t1);
Texture2D<float4> bloomTexture : register(t2);
Texture3D<float4> colorGradingLUTTexture : register(t3);
Texture2D<float4> bloomDirtMaskTexture : register(t4);
RWTexture2D<float4> outputTexture : register(u0);

[RootSignature(COMMON_ROOT_SIGNATURE)]
[numthreads(FinalPostProcessBlockSizeX, FinalPostProcessBlockSizeY, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID) {
    uint2 pixelCoord = dispatchThreadId.xy;
    if (pixelCoord.x >= finalPostProcessParams.outputSize.x || pixelCoord.y >= finalPostProcessParams.outputSize.y) {
        return;
    }

    float2 uv = (float2(pixelCoord) + 0.5) * finalPostProcessParams.outputTexelSize;
    float2 centerOffset = uv - 0.5;
    float2 centerOffsetPixels = centerOffset * float2(finalPostProcessParams.outputSize);
    float2 inputUv = uv;
    float4 hdrColor;

    // 렌즈 왜곡 적용
    if (finalPostProcessParams.lensDistortion != 0.0) {
        float normalizedRadiusSq = dot(centerOffsetPixels, centerOffsetPixels) * finalPostProcessParams.lensDistortionRadiusSqScale;
        float radialScale = finalPostProcessParams.lensDistortionFitScale / (1.0 - finalPostProcessParams.lensDistortion * normalizedRadiusSq);
        inputUv = 0.5 + centerOffset * radialScale;
        hdrColor = inputTexture.SampleLevel(linearClampSampler, inputUv, 0);
    } else {
        hdrColor = inputTexture.Load(int3(pixelCoord, 0));
    }

    // 크로마틱 수차 적용
    if (finalPostProcessParams.chromaticAberration > 0.0) {
        float centerDistance = length(centerOffsetPixels);
        float normalizedRadius = saturate(length(centerOffset) * 2.0);
        float2 radialDirection = centerOffsetPixels / max(centerDistance, 0.0001);
        float2 sampleOffset = radialDirection * finalPostProcessParams.outputTexelSize * finalPostProcessParams.chromaticAberration * normalizedRadius * normalizedRadius;

        hdrColor.r = inputTexture.SampleLevel(linearClampSampler, inputUv - sampleOffset, 0).r;
        hdrColor.b = inputTexture.SampleLevel(linearClampSampler, inputUv + sampleOffset, 0).b;
    }

    // 블룸 효과 적용
    if (finalPostProcessParams.bloomIntensity > 0.0) {
        float3 bloomColor = bloomTexture.SampleLevel(linearClampSampler, inputUv, 0).rgb * finalPostProcessParams.bloomIntensity;
        if (finalPostProcessParams.bloomDirtIntensity > 0.0) {
            float2 dirtUv = centerOffset * finalPostProcessParams.bloomDirtUvScale + 0.5;
            float3 dirtMask = bloomDirtMaskTexture.SampleLevel(linearClampSampler, dirtUv, 0).rgb;
            bloomColor += bloomColor * dirtMask * finalPostProcessParams.bloomDirtIntensity;
        }
        hdrColor.rgb += bloomColor;
    }

    // 화이트 밸런스 적용
    float3 color = max(hdrColor.rgb * finalPostProcessParams.whiteBalance, float3(0.0, 0.0, 0.0));

    bool hdrOutput = (finalPostProcessParams.flags & FinalPostProcessFlagHDROutput) != 0;
    float maxOutputLinearValue = hdrOutput ? max(finalPostProcessParams.maxDisplayLinearValue, 1.0) : 1.0;

    // 톤 매핑 적용
    if ((finalPostProcessParams.flags & FinalPostProcessFlagToneMapping) != 0) {
        float exposure = luminanceBuffer.Load<float>(LuminanceBufferOffsetExposure);
        float3 exposedColor = color * exposure;
        // HDR 출력 시에는 Filmic GT 톤 매핑 적용, 그렇지 않으면 선택한 톤 매핑 연산자 적용
        color = hdrOutput ? ToneMapFilmicGT(exposedColor, maxOutputLinearValue) : ToneMap(exposedColor, finalPostProcessParams.toneMappingOperator);
    }

    // 컬러 그레이딩 적용
    if ((finalPostProcessParams.flags & FinalPostProcessFlagColorGrading) != 0) {
        float colorScale = hdrOutput ? max(max(color.r, color.g), max(color.b, 1.0)) : 1.0;
        float lutSize = (float)ColorGradingLUTSize;
        float3 lutCoord = (saturate(color / colorScale) * (lutSize - 1.0) + 0.5) / lutSize;
        color = colorGradingLUTTexture.SampleLevel(linearClampSampler, lutCoord, 0).rgb * colorScale;
    }

    // 비네팅 효과 적용
    if (finalPostProcessParams.vignetteIntensity > 0.0) {
        float2 vignetteOffset = centerOffset * 2.0;
        float vignette = smoothstep(0.8, 1.5, dot(vignetteOffset, vignetteOffset));
        color *= 1.0 - vignette * finalPostProcessParams.vignetteIntensity;
    }

    // 디더링 적용
    if ((finalPostProcessParams.flags & FinalPostProcessFlagDithering) != 0) {
        float dither = (InterleavedGradientNoise(pixelCoord) - 0.5) / 255.0;
        color += dither;
    }

    if (hdrOutput) {
        outputTexture[pixelCoord] = float4(clamp(color, 0.0, maxOutputLinearValue), saturate(hdrColor.a));
    } else {
        outputTexture[pixelCoord] = float4(saturate(color), saturate(hdrColor.a));
    }
}
