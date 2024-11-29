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
    row_major float4x4 viewProjMatrix;
    row_major float3x4 worldMatrix[1024];
};

Texture2D defaultTexture : register(t0);
SamplerState defaultSampler : register(s0);

PSInput VSMain(VSInput input) {
    PSInput result = (PSInput)0;

    float3 positionWS3 = mul(worldMatrix[input.instanceID], input.position);
    float4 positionWS = float4(positionWS3, 1.0);
    result.position = mul(viewProjMatrix, positionWS);
    result.color = input.color;
    result.texCoord = input.texCoord;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET {
    float4 color = defaultTexture.Sample(defaultSampler, input.texCoord);
    return color * input.color;
}
