#include "../BindlessResources.hlsli"
#include "../Common.hlsli"
#include "../FullscreenTriangle.hlsli"

#define FOG_COMPOSITE_ROOT_SIGNATURE ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS \
    ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    "DescriptorTable(SRV(t0)), CBV(b0), CBV(b1), " \
    "RootConstants(num32BitConstants=4, b2), " \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

DECLARE_COMMON_STATIC_SAMPLERS
cbuffer FrameCB : register(b0) {
    FrameConstants frameConstants;
}

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
}

cbuffer FogCompositeCB : register(b2) {
    uint sourceTextureIndex;
    uint integratedTextureIndex;
    uint useVolumetricFog;
    uint useAnalyticHeightFog;
}

#include "FogSurface.hlsli"

#ifdef FOG_MSAA
Texture2DMS<float> sceneDepthTexture : register(t0);
#else
Texture2D<float> sceneDepthTexture : register(t0);
#endif

[RootSignature(FOG_COMPOSITE_ROOT_SIGNATURE)]
float4 VSMain(uint vertexID : SV_VertexID) : SV_POSITION {
    return GetFullscreenTriangleVertex(vertexID);
}

[RootSignature(FOG_COMPOSITE_ROOT_SIGNATURE)]
#ifdef FOG_MSAA
float4 PSMain(float4 position : SV_POSITION, uint sampleIndex : SV_SampleIndex) : SV_TARGET {
#else
float4 PSMain(float4 position : SV_POSITION) : SV_TARGET {
#endif
    const uint2 pixel = uint2(position.xy);
#ifdef FOG_MSAA
    const float deviceDepth = sceneDepthTexture.Load(pixel, sampleIndex);
#else
    const float deviceDepth = sceneDepthTexture.Load(int3(pixel, 0));
#endif
    // 클리어 깊이는 무한대를 나타낼 수 있으므로, 선형 깊이 (-z in view-space) 를 복원하기 전에 클리어 값인지 검사한다.
    const float depth = abs(deviceDepth - cameraConstants.depthClearValue) <= 1.0e-7f ? FLT_MAX : cameraConstants.GetLinearDepth(deviceDepth);

    // 화면 픽셀 좌표를 raster UV 로 변환한다. 볼륨 샘플링 시에만 jitter 를 제거한다.
    const float2 uv = position.xy * cameraConstants.renderSizeRcp;

    // depth 까지의 누적 산란/발광 radiance 와 투과율을 샘플링한다.
    const float4 scatteringTransmittance = FogSampleScene(uv, depth, useVolumetricFog, useAnalyticHeightFog, integratedTextureIndex);

    // 컬러 렌더 타겟에 산란/발광 RGB 를 PremultipliedAlphaBlend 로 블렌딩한다.
    // S + (1 - (1 - T)) * Dest.RGB = S + T * Dest.RGB
    if (useVolumetricFog == 0 || cameraConstants.vfogParams.debugMode == 0) {
        return float4(scatteringTransmittance.rgb, 1.0f - scatteringTransmittance.a);
    }

    // --------------------------------------------------------------
    // ------------------------- Debug 모드 -------------------------
    // --------------------------------------------------------------
    const VFogParams vfogParams = cameraConstants.vfogParams;
    const float2 volumeUV = uv - cameraConstants.uvJitter;
    [branch]
    // Optical depth
    if (vfogParams.debugMode == 1) {
        // 표면까지 각 fog 경로의 유효 구간에서 누적된 optical depth: tau = -ln(T).
        // 표시 범위 0 ~ 3 에 맞춰 T 를 제한하여 log(0) 을 피하고, 3 이상은 빨강으로 포화시킨다.
        const float opticalDepth = -log(clamp(scatteringTransmittance.a, exp(-3.0f), 1.0f));
        // tau = 0, 1, 2, 3: Blue -> Cyan -> Yellow -> Red.
        const float3 color = saturate(float3(opticalDepth - 1.0f, min(opticalDepth, 3.0f - opticalDepth), 2.0f - opticalDepth));
        return float4(color, 1);
    }

    // Depth slices
    if (vfogParams.debugMode == 2) {
        const float slicePos = VFogDepthToSlice(min(depth, vfogParams.farDepth), vfogParams);
        return float4(floor(slicePos) / vfogParams.sliceCount, frac(slicePos), 0, 1);
    }

    const uint slice = min(vfogParams.debugSlice, vfogParams.sliceCount - 1u);
    const Texture3D<float4> sourceVolumeTexture = GetTexture3D(sourceTextureIndex);
    const Texture3D<float4> integratedVolumeTexture = GetTexture3D(integratedTextureIndex);
    const float4 froxelValue = sourceVolumeTexture.SampleLevel(linearClampSampler, float3(volumeUV, (slice + 0.5f) / vfogParams.sliceCount), 0);
    const float4 boundary = integratedVolumeTexture.SampleLevel(linearClampSampler, float3(volumeUV, (slice + 1.5f) / (vfogParams.sliceCount + 1.0f)), 0);

    float3 color = 0;
    // Extinction Coefficient
    if (vfogParams.debugMode == 3) {
        color = froxelValue.aaa;
    }
    // Scattering + Emission Source
    if (vfogParams.debugMode == 4) {
        color = froxelValue.rgb;
    }
    // Accumulated Scattering + Emission Radiance
    if (vfogParams.debugMode == 5) {
        color = boundary.rgb;
    }
    // Accumulated Transmittance
    if (vfogParams.debugMode == 6) {
        color = boundary.aaa;
    }
    return float4(color, 1);
}
