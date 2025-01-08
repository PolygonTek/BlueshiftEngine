#define COMMON_ROOT_SIGNATURE "DescriptorTable(SRV(t0)), " \
    "StaticSampler(s0, FILTER = FILTER_MIN_MAG_MIP_POINT, ADDRESSU = TEXTURE_ADDRESS_CLAMP, ADDRESSV = TEXTURE_ADDRESS_CLAMP, ADDRESSW = TEXTURE_ADDRESS_CLAMP)"

struct PSInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

Texture2D defaultTexture : register(t0);
SamplerState defaultSampler : register(s0);

[RootSignature(COMMON_ROOT_SIGNATURE)]

PSInput VSMain(uint vertexID : SV_VertexID) {
    PSInput result;

    // Creates a full screen CCW triangle from 3 vertices
    result.position.x = (float)(vertexID % 2) * 4.0 - 1.0;
    result.position.y = (float)(vertexID / 2) * 4.0 - 1.0;
    result.position.z = 0;
    result.position.w = 1;

    result.texCoord.x = (float)(vertexID % 2) * 2;
    result.texCoord.y = (float)(vertexID / 2) * 2;

    return result;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]

float4 PSMain(PSInput input) : SV_TARGET {
    return defaultTexture.Sample(defaultSampler, input.texCoord);
}
