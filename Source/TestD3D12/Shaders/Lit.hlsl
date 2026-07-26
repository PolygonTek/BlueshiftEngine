#include "SurfaceDrawCommon.hlsli"
#include "LightCullingCommon.hlsli"
#include "LightCullingDebug.hlsli"
#include "ForwardLighting.hlsli"

#define COMMON_ROOT_SIGNATURE ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS \
    ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    ROOT_SIGNATURE_SURFACE_DRAW_RESOURCES \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

struct VSInput {
    float4 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR;
    float4 normal : NORMAL;
    float4 tangent : TEXCOORD1;
    uint instanceID : SV_InstanceID;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
    nointerpolation uint shaderMaterialIndex : TEXCOORD1;
    float3 worldPosition : TEXCOORD2;
    float3 worldNormal : TEXCOORD3;
#ifdef OUTPUT_VELOCITY
    float4 currentVelocityPosition : TEXCOORD4;
    float4 prevVelocityPosition : TEXCOORD5;
#endif
};

[RootSignature(COMMON_ROOT_SIGNATURE)]
PSInput VSMain(VSInput input) {
    PSInput result;

    ShaderSurface shaderSurface = GetShaderSurfaceByInstanceID(input.instanceID);

    float4 positionWS = float4(mul(shaderSurface.worldMatrix, input.position), 1.0);
    result.position = mul(cameraConstants.viewProjMatrix, positionWS);
    result.color = input.color;
    result.texCoord = input.texCoord;
    result.shaderMaterialIndex = shaderSurface.shaderMaterialIndex;
    result.worldPosition = positionWS.xyz;
    result.worldNormal = TransformSurfaceNormal(DecodeVertexNormal(input.normal.xyz), shaderSurface.worldMatrix, (shaderSurface.flags & ShaderSurfaceFlagNonUniformScale) != 0);
#ifdef OUTPUT_VELOCITY
    result.currentVelocityPosition = mul(cameraConstants.unjitteredViewProjMatrix, positionWS);
    result.prevVelocityPosition = CalcClipPosition(input.position, shaderSurface.prevWorldMatrix, cameraConstants.prevUnjitteredViewProjMatrix);
#endif

    return result;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
SurfacePSOutput PSMain(PSInput input) {
    ShaderMaterial shaderMaterial = GetShaderMaterial(input.shaderMaterialIndex);

    if (frameConstants.feedbackBufferIndex != 0 && shaderMaterial.streamingMaterialIndex != TextureStreamingInvalidIndex) {
        WriteMipmapFeedback(GetRWBufferUInt(frameConstants.feedbackBufferIndex), shaderMaterial.streamingMaterialIndex, ddx_coarse(input.texCoord.xy), ddy_coarse(input.texCoord.xy));
    }

    float4 textureColor = GetTexture2D(shaderMaterial.baseColorTextureIndex).Sample(linearWrapSampler, input.texCoord);
    const float3 worldNormal = normalize(input.worldNormal);

    SurfacePSOutput result;
    result.color = textureColor * UnpackShaderMaterialBaseColorFactor(shaderMaterial) * input.color;
#ifdef OUTPUT_NORMAL
    result.normal = EncodeOctahedralNormal(worldNormal);
#endif
#ifdef OUTPUT_VELOCITY
    result.velocity = CalcVelocity(input.currentVelocityPosition, input.prevVelocityPosition);
#endif

    if (cameraConstants.lightCullingDebugMode != ShaderLightCullingDebugModeNone) {
        result.color.rgb = VisualizeLightCulling((uint2)input.position.xy, input.position.z, cameraConstants.lightCullingDebugMode);
        return result;
    }

    const float3 lighting = EvaluateLit((uint2)input.position.xy, input.position.z, input.worldPosition, worldNormal);
    result.color.rgb *= lighting;
    return result;
}
