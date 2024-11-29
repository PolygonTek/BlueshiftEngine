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

PSInput VSMain(VSInput input) {
    PSInput result = (PSInput)0;

    result.position = input.position;
    result.position.xy += offset[input.instanceID].xy;
    result.color = input.color;
    result.texCoord = input.texCoord;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET {
    float4 color = defaultTexture.Sample(defaultSampler, input.texCoord);
    return color * input.color;
}
