#include "../SurfaceDrawCommon.hlsli"

#define SKYBOX_CUBEMAP_ROOT_SIGNATURE ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS \
    ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    ROOT_SIGNATURE_SURFACE_DRAW_RESOURCES \
    "DescriptorTable(SRV(t1, numDescriptors = 1)), " \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

TextureCube skyCubeMap : register(t1);

struct VSInput {
    float4 position : POSITION;
    float4 color : COLOR;
    uint instanceID : SV_InstanceID;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float3 texCoord : TEXCOORD0;
};

[RootSignature(SKYBOX_CUBEMAP_ROOT_SIGNATURE)]
PSInput VSMain(VSInput input) {
    PSInput result;

    ShaderSurface shaderSurface = GetShaderSurfaceByInstanceID(input.instanceID);

    float3 v = input.position.xyz;

    static const float rotation = 0.0;
    float radian = rotation * 0.01745329251;
    float s;
    float c;
    sincos(radian, s, c);

    float3 dir;
    dir.x = c * v.x - s * v.y;
    dir.y = s * v.x + c * v.y;
    dir.z = v.z;

    result.texCoord = dir;

    float4 positionWS = float4(mul(shaderSurface.worldMatrix, input.position), 1.0);
    result.position = mul(cameraConstants.viewProjMatrix, positionWS);
    result.position.z = cameraConstants.ndcFarZ * result.position.w;
    result.color = input.color;

    return result;
}

[RootSignature(SKYBOX_CUBEMAP_ROOT_SIGNATURE)]
float4 PSMain(PSInput input) : SV_TARGET {
    static const float3 tint = float3(1.0, 1.0, 1.0);
    static const float exposure = 1.0;

    float3 color = skyCubeMap.Sample(linearClampSampler, input.texCoord).rgb;
    color *= tint * exposure;

    return input.color * float4(color, 1.0);
}
