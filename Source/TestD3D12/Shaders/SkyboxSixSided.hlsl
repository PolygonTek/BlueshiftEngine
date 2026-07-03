#include "SurfaceDrawCommon.hlsli"

#define SKYBOX_SIX_SIDED_ROOT_SIGNATURE ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS \
    ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    ROOT_SIGNATURE_SURFACE_DRAW_RESOURCES \
    "DescriptorTable(" \
        "SRV(t1, numDescriptors = 1), " \
        "SRV(t2, numDescriptors = 1), " \
        "SRV(t3, numDescriptors = 1), " \
        "SRV(t4, numDescriptors = 1), " \
        "SRV(t5, numDescriptors = 1), " \
        "SRV(t6, numDescriptors = 1)), " \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

Texture2D skyBackMap : register(t1);
Texture2D skyFrontMap : register(t2);
Texture2D skyRightMap : register(t3);
Texture2D skyLeftMap : register(t4);
Texture2D skyTopMap : register(t5);
Texture2D skyDownMap : register(t6);

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

[RootSignature(SKYBOX_SIX_SIDED_ROOT_SIGNATURE)]
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

    result.texCoord.x = dir.y;
    result.texCoord.y = dir.z;
    result.texCoord.z = dir.x;

    float4 positionWS = float4(mul(shaderSurface.worldMatrix, input.position), 1.0);
    result.position = mul(cameraConstants.viewProjMatrix, positionWS);
    result.position.z = cameraConstants.ndcFarZ * result.position.w;
    result.color = input.color;

    return result;
}

[RootSignature(SKYBOX_SIX_SIDED_ROOT_SIGNATURE)]
float4 PSMain(PSInput input) : SV_TARGET {
    static const float3 tint = float3(1.0, 1.0, 1.0);
    static const float exposure = 1.0;

    float3 dir = normalize(input.texCoord);
    float3 absDir = abs(dir);

    float3 color = float3(1.0, 1.0, 1.0);
    float2 st;

    if (absDir.x > absDir.y && absDir.x > absDir.z) {
        if (dir.x > 0.0) {
            st = float2(-dir.z, -dir.y) / absDir.x * 0.5 + 0.5;
            color = skyRightMap.Sample(linearClampSampler, st).rgb;
        } else {
            st = float2(dir.z, -dir.y) / absDir.x * 0.5 + 0.5;
            color = skyLeftMap.Sample(linearClampSampler, st).rgb;
        }
    } else {
        if (absDir.y > absDir.z) {
            if (dir.y > 0.0) {
                st = float2(dir.x, dir.z) / absDir.y * 0.5 + 0.5;
                color = skyTopMap.Sample(linearClampSampler, st).rgb;
            } else {
                st = float2(dir.x, -dir.z) / absDir.y * 0.5 + 0.5;
                color = skyDownMap.Sample(linearClampSampler, st).rgb;
            }
        } else {
            if (dir.z > 0.0) {
                st = float2(dir.x, -dir.y) / absDir.z * 0.5 + 0.5;
                color = skyBackMap.Sample(linearClampSampler, st).rgb;
            } else {
                st = float2(-dir.x, -dir.y) / absDir.z * 0.5 + 0.5;
                color = skyFrontMap.Sample(linearClampSampler, st).rgb;
            }
        }
    }

    color *= tint * exposure;
    return input.color * float4(color, 1.0);
}
