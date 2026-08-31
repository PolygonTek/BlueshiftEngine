#include "ShaderInterop.h"

#define COMMON_ROOT_SIGNATURE "DescriptorTable(SRV(t0)), " \
    "RootConstants(num32BitConstants=4, b0), " \
    "StaticSampler(s0, FILTER = FILTER_MIN_MAG_MIP_POINT, ADDRESSU = TEXTURE_ADDRESS_CLAMP, ADDRESSV = TEXTURE_ADDRESS_CLAMP, ADDRESSW = TEXTURE_ADDRESS_CLAMP), " \
    "StaticSampler(s1, FILTER = FILTER_MIN_MAG_MIP_LINEAR, ADDRESSU = TEXTURE_ADDRESS_CLAMP, ADDRESSV = TEXTURE_ADDRESS_CLAMP, ADDRESSW = TEXTURE_ADDRESS_CLAMP)"

struct PSInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

Texture2D inputTexture : register(t0);
SamplerState pointClampSampler : register(s0);
SamplerState linearClampSampler : register(s1);

cbuffer FullscreenImageCB : register(b0) {
    FullscreenImageParams imageParams;
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
    float2 texCoord = input.texCoord * imageParams.texCoordScale + imageParams.texCoordOffset;
    return inputTexture.Sample(pointClampSampler, texCoord);
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
float4 PSLinear(PSInput input) : SV_TARGET {
    float2 texCoord = input.texCoord * imageParams.texCoordScale + imageParams.texCoordOffset;
    return inputTexture.Sample(linearClampSampler, texCoord);
}
