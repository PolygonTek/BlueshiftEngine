#include "Common.hlsli"

#define COMMON_ROOT_SIGNATURE ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS \
    "DescriptorTable(CBV(b0)), " \
    ROOT_SIGNATURE_BINDLESS_TABLES \
    ROOT_SIGNATURE_STATIC_SAMPLERS

SamplerState linearClampSampler : register(s100);
SamplerState linearWrapSampler : register(s101);
SamplerState linearMirrorSampler : register(s102);
SamplerState pointClampSampler : register(s103);
SamplerState pointWrapSampler : register(s104);
SamplerState pointMirrorSampler : register(s105);
SamplerState anisoClampSampler : register(s106);
SamplerState anisoWrapSampler : register(s107);
SamplerState anisoMirrorSampler : register(s108);

struct VSInput {
    float4 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR;
    float4 normal : NORMAL;
    float4 tangent : TEXCOORD1;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
};

cbuffer ObjectCB : register(b0) {
    row_major float4x4 modelViewProjMatrix;
    uint baseColorTextureIndex;
};

[RootSignature(COMMON_ROOT_SIGNATURE)]
PSInput VSMain(VSInput input) {
    PSInput result;
    result.position = mul(modelViewProjMatrix, input.position);
    result.color = input.color;
    result.texCoord = input.texCoord;

    return result;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
float4 PSMain(PSInput input) : SV_TARGET {
    float4 color = GetTexture2D(baseColorTextureIndex).Sample(anisoClampSampler, input.texCoord);
    return color * input.color;
}
