#include "SurfaceDrawCommon.hlsli"
#include "LightCullingCommon.hlsli"
#include "LightCullingDebug.hlsli"
#include "ForwardLighting.hlsli"
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
#if USE_SURFACE_TANGENT_FRAME
    float4 worldTangent : TEXCOORD4;
#endif
#ifdef OUTPUT_VELOCITY
    float4 currentVelocityPosition : TEXCOORD5;
    float4 prevVelocityPosition : TEXCOORD6;
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
#if USE_SURFACE_TANGENT_FRAME
    result.worldTangent = TransformSurfaceTangent(DecodeVertexTangent(input.tangent), shaderSurface.worldMatrix);
#endif
#ifdef OUTPUT_VELOCITY
    result.currentVelocityPosition = mul(cameraConstants.unjitteredViewProjMatrix, positionWS);
    result.prevVelocityPosition = CalcClipPosition(input.position, shaderSurface.prevWorldMatrix, cameraConstants.prevUnjitteredViewProjMatrix);
#endif

    return result;
}

[RootSignature(COMMON_ROOT_SIGNATURE)]
SurfacePSOutput PSMain(PSInput input) {
    ShaderMaterial shaderMaterial = GetShaderMaterial(input.shaderMaterialIndex);

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

    baseTc = ParallaxMapping(GetTexture2D(shaderMaterial.heightTextureIndex), linearWrapSampler, baseTc, shaderMaterial.heightScale, viewTS, texCoordDx, texCoordDy);
#endif

    float4 baseColor = UnpackShaderMaterialBaseColorFactor(shaderMaterial) * input.color;
#if _BASECOLOR != 0
    baseColor *= GetTexture2D(shaderMaterial.baseColorTextureIndex).Sample(linearWrapSampler, baseTc);
#endif

#ifdef PERFORATED
    clip(baseColor.a - shaderMaterial.alphaCutoff);
#endif

#if _NORMAL != 0 || (_CLEARCOAT != 0 && _CLEARCOAT_NORMAL != 0)
    ShadingParams shading = PrepareShadingParams(shaderMaterial, baseColor, baseTc, viewDir, worldNormal, tangentToWorld, linearWrapSampler);
#else
    ShadingParams shading = PrepareShadingParams(shaderMaterial, baseColor, baseTc, viewDir, worldNormal, linearWrapSampler);
#endif

    SurfacePSOutput result;
    result.color = baseColor;
#ifdef OUTPUT_NORMAL
    result.normal = EncodeOctahedralNormal(shading.normal);
#endif
#ifdef OUTPUT_VELOCITY
    result.velocity = CalcVelocity(input.currentVelocityPosition, input.prevVelocityPosition);
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
    const ImageBasedLightingSample environmentLightSample = SampleEnvironmentLight(shading);
    lighting += EvaluateStandardImageBasedLighting(shading, environmentLightSample, dfg);
#endif

    result.color.rgb = lighting + shading.emission;
    return result;
}
