#include "SurfaceDrawCommon.hlsli"

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
};

#include "TextureStreamingFeedback.hlsli"
#include "Lighting/LightCullingCommon.hlsli"
#include "Lighting/LightCullingDebug.hlsli"
#include "Lighting/ForwardLighting.hlsli"
#include "PBR/ParallaxMapping.hlsli"
#include "PBR/ShadingParams.hlsli"

#if defined(INDIRECT_LIGHTING) || (defined(DIRECT_LIGHTING) && USE_MULTIPLE_SCATTERING_COMPENSATION)
    #include "PBR/ImageBasedLighting.hlsli"
#endif

#if _PARALLAX != 0 || _NORMAL != 0 || (_CLEARCOAT != 0 && _CLEARCOAT_NORMAL != 0)
    #define USE_SURFACE_TANGENT_FRAME 1
#else
    #define USE_SURFACE_TANGENT_FRAME 0
#endif

#define COMMON_ROOT_SIGNATURE ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS \
    ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    ROOT_SIGNATURE_SURFACE_DRAW_RESOURCES \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

struct VSInput {
    float4 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR;
#ifdef PACKED_VERTEX_TANGENT_FRAME
    uint normal : NORMAL;
#else
    float4 normal : NORMAL;
    float4 tangent : TEXCOORD1;
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
    float3 worldPosition : TEXCOORD2;
    float3 worldNormal : TEXCOORD3;
#if USE_SURFACE_TANGENT_FRAME
    float4 worldTangent : TEXCOORD4;
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
    result.worldPosition = positionWS.xyz;
    float3 vertexNormal;
#if USE_SURFACE_TANGENT_FRAME
    float4 vertexTangent;
    #ifdef PACKED_VERTEX_TANGENT_FRAME
        DecodeTangentFrame(input.normal, vertexNormal, vertexTangent);
    #else
        vertexNormal = DecodeVertexNormal(input.normal);
        vertexTangent = DecodeVertexTangent(input.tangent);
    #endif
#else
    #ifdef PACKED_VERTEX_TANGENT_FRAME
        vertexNormal = DecodeTangentFrameNormal(input.normal);
    #else
        vertexNormal = DecodeVertexNormal(input.normal);
    #endif
#endif

#if GPU_SKINNING
    vertexNormal = mul((float3x3)skinningMatrix, vertexNormal);
    #if USE_SURFACE_TANGENT_FRAME
        vertexTangent.xyz = mul((float3x3)skinningMatrix, vertexTangent.xyz);
    #endif
#endif

    result.worldNormal = TransformSurfaceNormal(vertexNormal, shaderSurface.worldMatrix, (shaderSurface.flags & ShaderSurfaceFlagNonUniformScale) != 0);
#if USE_SURFACE_TANGENT_FRAME
    result.worldTangent = TransformSurfaceTangent(vertexTangent, shaderSurface.worldMatrix);
#endif

    return result;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
CameraPassPSOutput PSMain(PSInput input) {
    ShaderMaterial shaderMaterial = GetShaderMaterial(input.shaderMaterialIndex);
    SamplerState materialSampler = GetSampler(shaderMaterial.samplerIndex);

    float2 texCoordDx = 0.0f;
    float2 texCoordDy = 0.0f;
    bool hasTexCoordGradients = false;

    if (frameConstants.feedbackBufferIndex != 0 && shaderMaterial.streamingMaterialIndex != TextureStreamingInvalidIndex) {
        texCoordDx = ddx_coarse(input.texCoord);
        texCoordDy = ddy_coarse(input.texCoord);
        hasTexCoordGradients = true;

        WriteMipmapFeedback(GetRWBufferUInt(frameConstants.feedbackBufferIndex), shaderMaterial.streamingMaterialIndex, texCoordDx, texCoordDy);
    }

    const float3 worldNormal = normalize(input.worldNormal);

#if USE_SURFACE_TANGENT_FRAME
    // worldTangent 를 worldNormal 에 대해 Gram-Schmidt 직교화 + normalize
    float3 worldTangent = input.worldTangent.xyz - worldNormal * dot(worldNormal, input.worldTangent.xyz);
    const float worldTangentLengthSquared = dot(worldTangent, worldTangent);
    worldTangent = worldTangentLengthSquared > FLT_MIN ? worldTangent * rsqrt(worldTangentLengthSquared) : float3(1.0f, 0.0f, 0.0f);

    const float3 worldBitangent = cross(worldNormal, worldTangent) * input.worldTangent.w;
    const float3x3 tangentToWorld = float3x3(worldTangent, worldBitangent, worldNormal);
#endif

    const float3 viewDir = normalize(cameraConstants.cameraOrigin - input.worldPosition);
    float2 baseTc = input.texCoord;

#if _PARALLAX != 0
    const float3 viewTS = mul(tangentToWorld, viewDir);
    if (!hasTexCoordGradients) {
        texCoordDx = ddx_coarse(input.texCoord);
        texCoordDy = ddy_coarse(input.texCoord);
    }

    baseTc = ParallaxMapping(GetTexture2D(shaderMaterial.heightTextureIndex), materialSampler, baseTc, shaderMaterial.heightScale, viewTS, texCoordDx, texCoordDy);
#endif

    float4 baseColor = shaderMaterial.GetBaseColorFactor() * input.color;
#if _BASECOLOR != 0
    baseColor *= GetTexture2D(shaderMaterial.baseColorTextureIndex).Sample(materialSampler, baseTc);
#endif

#ifdef PERFORATED
    clip(baseColor.a - shaderMaterial.alphaCutoff);
#endif

    const bool useSpecularAA = (cameraConstants.flags & ShaderCameraFlagUseSpecularAA) != 0u;
#if _NORMAL != 0 || (_CLEARCOAT != 0 && _CLEARCOAT_NORMAL != 0)
    ShadingParams shading = PrepareShadingParams(shaderMaterial, baseColor, baseTc, viewDir, worldNormal, tangentToWorld, useSpecularAA, materialSampler);
#else
    ShadingParams shading = PrepareShadingParams(shaderMaterial, baseColor, baseTc, viewDir, worldNormal, useSpecularAA, materialSampler);
#endif

    CameraPassPSOutput result;
    result.color = baseColor;
#ifdef OUTPUT_NORMAL
    // Lighting stays in world space; the shared AO/SSR buffer stores view-space normals.
    result.normal = EncodeUnitVectorOctahedral(mul((float3x3)cameraConstants.viewRotation, shading.normal));
#endif

#if DEBUG_CASCADED_SHADOW_MAP == 1
    result.color.rgb = EvaluateDirectionalShadowLighting((uint2)input.position.xy, input.worldPosition);
    return result;
#endif

    if (cameraConstants.lightCullingDebugMode != ShaderLightCullingDebugModeNone) {
        result.color.rgb = VisualizeLightCulling((uint2)input.position.xy, input.position.z, cameraConstants.lightCullingDebugMode);
        return result;
    }

#if defined(INDIRECT_LIGHTING) || (defined(DIRECT_LIGHTING) && USE_MULTIPLE_SCATTERING_COMPENSATION)
    const ImageBasedLightingDFG dfg = SampleImageBasedLightingDFG(shading);
#endif

#if USE_MULTIPLE_SCATTERING_COMPENSATION && (defined(DIRECT_LIGHTING) || defined(INDIRECT_LIGHTING))
    shading.energyCompensation = GetMultipleScatteringEnergyCompensation(dfg.baseLayer.x, shading.specularF0);

    #if _CLEARCOAT != 0
        shading.clearCoatEnergyCompensation = GetMultipleScatteringEnergyCompensation(dfg.clearCoat.x, 0.04f);
    #endif
#endif

    float3 lighting = 0.0f;

#ifdef DIRECT_LIGHTING
    lighting += EvaluateDirectLighting((uint2)input.position.xy, input.position.z, input.worldPosition, shading);
#endif

#ifdef INDIRECT_LIGHTING
    // Opaque and alpha-tested surfaces use screen-space AO from the depth prepass.
    if ((cameraConstants.flags & ShaderCameraFlagUseAO) != 0 &&
        (shaderMaterial.flags & ShaderMaterialFlagTransparent) == 0) {
        const float screenAO = GetTexture2D(cameraConstants.aoTextureIndex).Load(int3(uint2(input.position.xy), 0)).r;
        // Material AO and screen-space AO can describe the same cavity; use the stronger
        // occlusion instead of counting it twice. The IBL path also derives
        // roughness-aware specular/clear-coat occlusion from this value.
        shading.ambientOcclusion = min(shading.ambientOcclusion, screenAO);
    }
    const ImageBasedLightingSample envLightSample = SampleEnvironmentLight(shading);
    lighting += EvaluateStandardImageBasedLighting(shading, envLightSample, dfg);
#endif

    result.color.rgb = lighting + shading.emission;
    return result;
}
