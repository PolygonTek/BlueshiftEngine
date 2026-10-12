#include "../ShaderInterop.h"
#include "../Common.hlsli"
#include "../Colors.hlsli"
#include "../FullscreenTriangle.hlsli"

#define COMMON_ROOT_SIGNATURE "DescriptorTable(SRV(t0)), " \
    "RootConstants(num32BitConstants=6, b0), " \
    "StaticSampler(s0, FILTER = FILTER_MIN_MAG_MIP_POINT, ADDRESSU = TEXTURE_ADDRESS_CLAMP, ADDRESSV = TEXTURE_ADDRESS_CLAMP, ADDRESSW = TEXTURE_ADDRESS_CLAMP)"

struct PSInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

Texture2D<float4> inputTexture : register(t0);
SamplerState inputSampler : register(s0);

cbuffer DisplayP3EDROutputCB : register(b0) {
    DisplayP3EDROutputParams outputParams;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
PSInput VSMain(uint vertexID : SV_VertexID) {
    PSInput result;
    GetFullscreenTriangleVertex(vertexID, result.position, result.texCoord);
    return result;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
float4 PSMain(PSInput input) : SV_TARGET {
    float2 texCoord = input.texCoord * outputParams.texCoordScale + outputParams.texCoordOffset;
    float4 inputColor = inputTexture.Sample(inputSampler, texCoord);

    // 입력은 페이퍼 화이트 기준의 선형 Rec.709 값을 사용한다.
    // P3-D65 원색으로 변환한 뒤 선형 EDR 출력 범위로 제한한다.
    float maxLinearValue = max(outputParams.maxDisplayLinearValue, 1.0);
    float3 linearRec709 = clamp(inputColor.rgb, 0.0, maxLinearValue);
    float3 linearP3D65 = REC709toP3D65(linearRec709);

    return float4(clamp(linearP3D65, 0.0, maxLinearValue), saturate(inputColor.a));
}
