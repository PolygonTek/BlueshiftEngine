#include "SurfaceDrawCommon.hlsli"
#include "TextureStreamingFeedback.hlsli"

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
};

#define COMMON_ROOT_SIGNATURE ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS \
    ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    ROOT_SIGNATURE_SURFACE_DRAW_RESOURCES \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

struct VSInput {
    float4 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR;
#ifdef OUTPUT_NORMAL
#ifdef PACKED_VERTEX_TANGENT_FRAME
    uint normal : NORMAL;
#else
    float4 normal : NORMAL;
#endif
#endif
    uint instanceID : SV_InstanceID;
#if GPU_SKINNING
    SkinningVertexInput skinning;
#endif
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
    nointerpolation uint shaderMaterialIndex : TEXCOORD1;
#ifdef OUTPUT_VELOCITY
    float4 currentVelocityPosition : TEXCOORD2;
    float4 prevVelocityPosition : TEXCOORD3;
#endif
#ifdef OUTPUT_NORMAL
    float3 worldNormal : TEXCOORD4;
#endif
};

[RootSignature(COMMON_ROOT_SIGNATURE)]
PSInput VSMain(VSInput input) {
    PSInput result;

    ShaderSurface shaderSurface = GetShaderSurfaceByInstanceID(input.instanceID, cameraConstants.surfaceTableIndex);

    float4 localPosition = input.position;
#if GPU_SKINNING
    const float3x4 skinningMatrix = GetSkinningMatrix(input.skinning, shaderSurface.skinningJointOffset);
    localPosition = float4(mul(skinningMatrix, localPosition), 1.0f);
#endif
    float4 positionWS = float4(mul(shaderSurface.worldMatrix, localPosition), 1.0);
    result.position = mul(cameraConstants.viewProjMatrix, positionWS);
    result.color = input.color;
    result.texCoord = input.texCoord;
    result.shaderMaterialIndex = shaderSurface.shaderMaterialIndex;

#ifdef OUTPUT_VELOCITY
    result.currentVelocityPosition = mul(cameraConstants.unjitteredViewProjMatrix, positionWS);
    float4 prevLocalPosition = localPosition;
    #if GPU_SKINNING
        prevLocalPosition = GetPreviousSkinnedPosition(input.skinning, input.position, localPosition, shaderSurface.skinningJointOffset, shaderSurface.prevSkinningJointOffset);
    #endif
    result.prevVelocityPosition = CalcClipPosition(prevLocalPosition, shaderSurface.prevWorldMatrix, cameraConstants.prevUnjitteredViewProjMatrix);
#endif

#ifdef OUTPUT_NORMAL
    #ifdef PACKED_VERTEX_TANGENT_FRAME
        float3 normal = DecodeTangentFrameNormal(input.normal);
    #else
        float3 normal = DecodeVertexNormal(input.normal);
    #endif
    #if GPU_SKINNING
        normal = mul((float3x3)skinningMatrix, normal);
    #endif
    result.worldNormal = TransformSurfaceNormal(normal, shaderSurface.worldMatrix, (shaderSurface.flags & ShaderSurfaceFlagNonUniformScale) != 0);
#endif

    return result;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
CameraPassPSOutput PSMain(PSInput input) {
    ShaderMaterial shaderMaterial = GetShaderMaterial(input.shaderMaterialIndex);
    SamplerState materialSampler = GetSampler(shaderMaterial.samplerIndex);

    if (frameConstants.feedbackBufferIndex != 0 && shaderMaterial.streamingMaterialIndex != TextureStreamingInvalidIndex) {
        WriteMipmapFeedback(GetRWBufferUInt(frameConstants.feedbackBufferIndex), shaderMaterial.streamingMaterialIndex, ddx_coarse(input.texCoord.xy), ddy_coarse(input.texCoord.xy));
    }

    float4 textureColor = GetTexture2D(shaderMaterial.baseColorTextureIndex).Sample(materialSampler, input.texCoord);
    CameraPassPSOutput result;
    result.color = textureColor * shaderMaterial.GetBaseColorFactor() * input.color;
#ifdef OUTPUT_NORMAL
    result.normal = EncodeOctahedralNormal(normalize(input.worldNormal));
#endif
#ifdef OUTPUT_VELOCITY
    result.velocity = CalcVelocity(input.currentVelocityPosition, input.prevVelocityPosition);
#endif
    return result;
}
