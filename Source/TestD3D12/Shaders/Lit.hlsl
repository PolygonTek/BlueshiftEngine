#ifdef SURFACE_PREPASS
#include "SurfacePrepass.hlsl"
#else
#include "LitSurfaceCommon.hlsli"

#include "TextureStreamingFeedback.hlsli"
#include "Lighting/LightCullingCommon.hlsli"
#include "Lighting/LightCullingDebug.hlsli"
#include "Lighting/ForwardLighting.hlsli"
#include "PBR/ShadingParams.hlsli"

#if defined(INDIRECT_LIGHTING) || defined(APPLY_SSR) || (defined(DIRECT_LIGHTING) && USE_MULTIPLE_SCATTERING_COMPENSATION)
    #include "PBR/ImageBasedLighting.hlsli"
#endif

#if defined(APPLY_SSR)
    #include "SSR/SSRComposite.hlsli"
#endif

[RootSignature(COMMON_ROOT_SIGNATURE)]
SurfacePassPSOutput PSMain(PSInput input) {
    ShaderMaterial shaderMaterial = GetShaderMaterial(input.shaderMaterialIndex);
    SamplerState materialSampler = GetSampler(shaderMaterial.samplerIndex);
    const float2 texCoord = shaderMaterial.TransformTexCoord(input.texCoord);

    float2 texCoordDx = 0.0f;
    float2 texCoordDy = 0.0f;
    bool hasTexCoordGradients = false;

    if (frameConstants.feedbackBufferIndex != 0 && shaderMaterial.streamingMaterialIndex != TextureStreamingInvalidIndex) {
        texCoordDx = ddx_coarse(texCoord);
        texCoordDy = ddy_coarse(texCoord);
        hasTexCoordGradients = true;

        WriteMipmapFeedback(GetRWBufferUInt(frameConstants.feedbackBufferIndex), shaderMaterial.streamingMaterialIndex, texCoordDx, texCoordDy);
    }

    const LitSurfaceInput surfaceInput = PrepareLitSurfaceInput(input, shaderMaterial, materialSampler, texCoord, texCoordDx, texCoordDy, hasTexCoordGradients);
    const float4 baseColor = EvaluateLitBaseColor(input, shaderMaterial, materialSampler, surfaceInput.texCoord);

    const bool useSpecularAA = (cameraConstants.flags & ShaderCameraFlagUseSpecularAA) != 0u;
#if _NORMAL != 0 || (_CLEARCOAT != 0 && _CLEARCOAT_NORMAL != 0)
    ShadingParams shading = PrepareShadingParams(shaderMaterial, baseColor, surfaceInput.texCoord, surfaceInput.viewDir, surfaceInput.worldNormal, surfaceInput.tangentToWorld, useSpecularAA, materialSampler);
#else
    ShadingParams shading = PrepareShadingParams(shaderMaterial, baseColor, surfaceInput.texCoord, surfaceInput.viewDir, surfaceInput.worldNormal, useSpecularAA, materialSampler);
#endif

    SurfacePassPSOutput result;
#if defined(OUTPUT_NORMAL) || defined(OUTPUT_ROUGHNESS)
    result.normalRoughness = float4(0.5f, 0.5f, 1.0f, 0.0f);
#endif
    result.color = baseColor;
#ifdef OUTPUT_VELOCITY
    result.velocity = CalcVelocity(input.currUnjitteredClipPos, input.prevUnjitteredClipPos);
    result.taaReactiveMask = 0.0f;
#endif
#ifdef OUTPUT_NORMAL
    // Lighting stays in world space; the shared AO/SSR buffer stores view-space normals.
    result.normalRoughness.xy = EncodeUnitVectorOctahedralUNorm(mul((float3x3)cameraConstants.viewRotation, shading.normal));
#endif
#ifdef OUTPUT_ROUGHNESS
    result.normalRoughness.z = shading.roughness;
#endif
#if DEBUG_CASCADED_SHADOW_MAP == 1
    result.color.rgb = EvaluateDirectionalShadowLighting((uint2)input.position.xy, input.worldPosition);
    return result;
#endif

    if (cameraConstants.lightCullingDebugMode != ShaderLightCullingDebugModeNone) {
        result.color.rgb = VisualizeLightCulling((uint2)input.position.xy, input.position.z, cameraConstants.lightCullingDebugMode);
        return result;
    }

#if defined(INDIRECT_LIGHTING) || defined(APPLY_SSR) || (defined(DIRECT_LIGHTING) && USE_MULTIPLE_SCATTERING_COMPENSATION)
    const ImageBasedLightingDFG dfg = SampleImageBasedLightingDFG(shading);
#endif

#if USE_MULTIPLE_SCATTERING_COMPENSATION && (defined(DIRECT_LIGHTING) || defined(INDIRECT_LIGHTING) || defined(APPLY_SSR))
    shading.energyCompensation = GetMultipleScatteringEnergyCompensation(dfg.baseLayer.x, shading.specularF0);

    #if _CLEARCOAT != 0
        shading.clearCoatEnergyCompensation = GetMultipleScatteringEnergyCompensation(dfg.clearCoat.x, 0.04f);
    #endif
#endif

    float3 lighting = 0.0f;

#ifdef DIRECT_LIGHTING
    lighting += EvaluateDirectLighting((uint2)input.position.xy, input.position.z, input.worldPosition, shading);
#endif

#if defined(INDIRECT_LIGHTING) || defined(APPLY_SSR)
    // Opaque and alpha-tested surfaces use screen-space AO from the depth prepass.
    if ((cameraConstants.flags & ShaderCameraFlagUseScreenSpaceAO) != 0 &&
        (shaderMaterial.flags & ShaderMaterialFlagTransparent) == 0) {
        const float screenAO = GetTexture2D(cameraConstants.aoTextureIndex).Load(int3(uint2(input.position.xy), 0)).r;
        // Material AO and screen-space AO can describe the same cavity; use the stronger
        // occlusion instead of counting it twice. The IBL path also derives
        // roughness-aware specular/clear-coat occlusion from this value.
        shading.ambientOcclusion = min(shading.ambientOcclusion, screenAO);
    }
#endif

#if defined(INDIRECT_LIGHTING) || defined(APPLY_SSR)
    ImageBasedLightingSample envLightSample;
    #ifdef INDIRECT_LIGHTING
        envLightSample = SampleEnvironmentLight(shading);
    #else
        envLightSample.diffuseIrradiance = 0.0f;
        envLightSample.specularRadiance = 0.0f;
        #if _CLEARCOAT != 0
            envLightSample.clearCoatSpecularRadiance = 0.0f;
        #endif
    #endif

    #ifdef APPLY_SSR
        if ((shaderMaterial.flags & ShaderMaterialFlagTransparent) != 0u) {
            if ((cameraConstants.flags & ShaderCameraFlagUseTransparentSSR) != 0u) {
                envLightSample.specularRadiance = CompositeTransparentSSRRadiance((uint2)input.position.xy, input.position.z, envLightSample.specularRadiance, cameraConstants.transparentSSRUpsampledReflectionTextureIndex, cameraConstants.transparentSSRDepthTextureIndex);
            }
        } else if ((cameraConstants.flags & ShaderCameraFlagUseSSR) != 0u) {
            envLightSample.specularRadiance = CompositeSSRRadiance((uint2)input.position.xy, envLightSample.specularRadiance, cameraConstants.ssrUpsampledReflectionTextureIndex);
        }
    #endif

    lighting += EvaluateStandardImageBasedLighting(shading, envLightSample, dfg);
#endif

    result.color.rgb = lighting + shading.emission;
    return result;
}

#endif // SURFACE_PREPASS
