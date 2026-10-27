#include "../SurfaceDrawCommon.hlsli"

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
};

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

    ShaderSurface shaderSurface = GetShaderSurfaceByInstanceID(input.instanceID, cameraConstants.surfaceTableIndex);

    result.texCoord = input.position.xyz;

    float4 positionWS = float4(mul(shaderSurface.worldMatrix, input.position), 1.0);
    result.position = mul(cameraConstants.viewProjMatrix, positionWS);
    result.position.z = cameraConstants.depthClearValue * result.position.w;
    result.color = input.color * cameraConstants.skyboxRadianceScale;

    return result;
}

[RootSignature(SKYBOX_CUBEMAP_ROOT_SIGNATURE)]
float4 PSMain(PSInput input) : SV_TARGET {
    float3 color = skyCubeMap.Sample(linearClampSampler, input.texCoord).rgb;

    return input.color * float4(color, 1.0);
}
