#include "Common.hlsli"
#include "Colors.hlsli"

#define COMMON_ROOT_SIGNATURE "DescriptorTable(SRV(t0)), " \
    "RootConstants(num32BitConstants=6, b0), " \
    "StaticSampler(s0, FILTER = FILTER_MIN_MAG_MIP_POINT, ADDRESSU = TEXTURE_ADDRESS_CLAMP, ADDRESSV = TEXTURE_ADDRESS_CLAMP, ADDRESSW = TEXTURE_ADDRESS_CLAMP)"

struct PSInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

Texture2D<float4> inputTexture : register(t0);
SamplerState inputSampler : register(s0);

cbuffer HDR10OutputCB : register(b0) {
    HDR10OutputParams outputParams;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
PSInput VSMain(uint vertexID : SV_VertexID) {
    PSInput result;

    // Creates a full screen CCW triangle from 3 vertices
    result.position.x = (float)(vertexID % 2) * 4.0 - 1.0;
    result.position.y = (float)(vertexID / 2) * 4.0 - 1.0;
    result.position.z = 0.0;
    result.position.w = 1.0;

    result.texCoord.x = (float)(vertexID % 2) * 2.0;
    result.texCoord.y = 1.0 - (float)(vertexID / 2) * 2.0;

    return result;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
float4 PSMain(PSInput input) : SV_TARGET {
    float2 texCoord = input.texCoord * outputParams.texCoordScale + outputParams.texCoordOffset;
    float4 inputColor = inputTexture.Sample(inputSampler, texCoord);

    // The source uses paper-white-relative linear Rec.709 values. Convert to absolute Rec.2020
    // luminance normalized to PQ's 10,000-nit range before applying the ST.2084 curve.
    float3 linearRec709 = clamp(inputColor.rgb, 0.0, outputParams.maxDisplayLinearValue);
    float3 linearRec2020 = REC709toREC2020(linearRec709);
    float3 pqColor = ApplyREC2084Curve(saturate(linearRec2020 * (outputParams.paperWhiteNits / 10000.0)));

    // Dither in encoded signal space immediately before quantization to RGB10.
    float dither = (InterleavedGradientNoise(uint2(input.position.xy)) - 0.5) / 1023.0;
    pqColor = saturate(pqColor + dither);

    return float4(pqColor, saturate(inputColor.a));
}
