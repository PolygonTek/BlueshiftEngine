#ifndef __FOG_SURFACE_HLSLI__
#define __FOG_SURFACE_HLSLI__

#include "../CommonBindless.hlsli"
#include "HeightFogCommon.hlsli"
#include "VFogCommon.hlsli"

// rasterUV includes projection jitter; depth is positive linear view depth.
float4 FogSampleScene(float2 rasterUV, float depth, bool useVolumetricFog, bool useAnalyticHeightFog, uint integratedTextureIndex) {
    float4 scatteringTransmittance = float4(0, 0, 0, 1);
    float startDepth = cameraConstants.fogNearDepth;

    // 프록셀 볼륨을 사용한다면, 볼륨 범위 내에서 depth 까지의 누적 산란광/발광과 투과율을 계산한다.
    if (useVolumetricFog) {
        const VFogParams vfogParams = cameraConstants.vfogParams;
        const float2 volumeUV = rasterUV - cameraConstants.uvJitter;
        const Texture3D<float4> integratedTexture = GetTexture3D(integratedTextureIndex);
        scatteringTransmittance = VFogSample(integratedTexture, linearClampSampler, volumeUV, depth, cameraConstants);
        startDepth = vfogParams.farDepth;
    }

    // 프록셀 볼륨을 통한 계산과 관계없이 Analytic HeightFog 를 사용한다면, 볼륨 이후 구간을 적분한다. 볼륨이 없으면 nearDepth 부터 적분한다.
    if (useAnalyticHeightFog && depth > startDepth) {
        const HeightFogParams hfogParams = cameraConstants.heightFogParams;
        const HeightFogRay ray = HeightFogGetRay(rasterUV, cameraConstants);
        const float maxFogDepth = hfogParams.maxDistance / ray.lengthPerViewDepth;
        const float endDepth = min(depth, maxFogDepth);

        if (endDepth > startDepth) {
            const float3 directionalRadiance = (cameraConstants.flags & ShaderCameraFlagUseDirectionalLight) ? frameConstants.directionalLight.intensityRGB : float3(0, 0, 0);
            const float4 farFog = HeightFogIntegrate(ray, startDepth, endDepth, hfogParams, directionalRadiance, frameConstants.directionalLight.direction);
            scatteringTransmittance = float4(scatteringTransmittance.rgb + scatteringTransmittance.a * farFog.rgb, scatteringTransmittance.a * farFog.a);
        }
    }
    return scatteringTransmittance;
}

// screenPosition is raster SV_POSITION.xyz, including projection jitter.
float4 FogSampleSurface(float3 screenPosition) {
    const float2 uv = screenPosition.xy * cameraConstants.renderSizeRcp;
    const float depth = cameraConstants.GetLinearDepth(screenPosition.z);
    const bool useVFog = (cameraConstants.flags & ShaderCameraFlagUseVFog) != 0;
    const bool useHFog = (cameraConstants.flags & ShaderCameraFlagUseHeightFog) != 0;
    return FogSampleScene(uv, depth, useVFog, useHFog, cameraConstants.vfogIntegratedTextureIndex);
}

float3 FogApplyToOpaqueSurface(float3 surfaceColor, float3 screenPosition) {
    const float4 scatteringTransmittance = FogSampleSurface(screenPosition);
    return surfaceColor * scatteringTransmittance.a + scatteringTransmittance.rgb;
}

float4 FogApplyToTransparentSurface(float4 surfaceColor, float3 screenPosition, uint blendMode) {
    const float4 scatteringTransmittance = FogSampleSurface(screenPosition);

    // 이 함수의 반환값은 최종 화면 색이 아니라, 이후 하드웨어 블렌딩에 들어갈 source 색이다.
    // 따라서 블렌딩 이후 산란광/발광이 올바르게 남도록 scatteringWeight 를 조정한다.
    // 블렌딩 대상에는 이미 fog 가 적용되어 있다.
    const float scatteringWeight = blendMode == ShaderMaterialVFogBlendAlpha ? 1.0f : (blendMode == ShaderMaterialVFogBlendPremultiplied ? surfaceColor.a : 0.0f);
    surfaceColor.rgb = surfaceColor.rgb * scatteringTransmittance.a + scatteringTransmittance.rgb * scatteringWeight;
    return surfaceColor;
}

#endif // __FOG_SURFACE_HLSLI__
