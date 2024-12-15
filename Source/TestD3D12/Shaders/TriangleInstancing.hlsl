#define COMMON_ROOT_SIGNATURE "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
    "DescriptorTable(SRV(t0), CBV(b0)), " \
    "StaticSampler(s0, FILTER = FILTER_MIN_MAG_MIP_LINEAR, ADDRESSU = TEXTURE_ADDRESS_CLAMP, ADDRESSV = TEXTURE_ADDRESS_CLAMP, ADDRESSW = TEXTURE_ADDRESS_CLAMP)"

struct VSInput {
    float4 position : POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
    uint instanceID : SV_InstanceID;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
};

cbuffer CONSTANT_BUFFER_DEFAULT : register(b0) {
    float4 offset[1024];
};

Texture2D defaultTexture : register(t0);
SamplerState defaultSampler : register(s0);

[RootSignature(COMMON_ROOT_SIGNATURE)]

PSInput VSMain(VSInput input) {
    PSInput result = (PSInput)0;

    result.position = input.position;
    result.position.xy += offset[input.instanceID].xy;
    result.color = input.color;
    result.texCoord = input.texCoord;

    return result;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]

float4 PSMain(PSInput input) : SV_TARGET {
    float4 color = defaultTexture.Sample(defaultSampler, input.texCoord);
    return color * input.color;
}
