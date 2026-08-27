#include "../ShaderInterop.h"
#include "../Common.hlsli"
#include "../CommonBindless.hlsli"
#include "../CameraPassOutputCommon.hlsli"

#define OCEAN_ROOT_SIGNATURE ROOT_SIGNATURE_GRAPHICS_ROOT_FLAGS \
    ROOT_SIGNATURE_BINDLESS_DESCRIPTOR_TABLES \
    "CBV(b0), " \
    "CBV(b1), " \
    "CBV(b2), " \
    "DescriptorTable(SRV(t0, numDescriptors = 2)), " \
    ROOT_SIGNATURE_COMMON_STATIC_SAMPLERS

DECLARE_COMMON_STATIC_SAMPLERS

cbuffer FrameCB : register(b0) {
    FrameConstants frameConstants;
};

cbuffer CameraCB : register(b1) {
    CameraConstants cameraConstants;
};

cbuffer OceanSurfaceCB : register(b2) {
    OceanSurfaceParams oceanSurfaceParams;
};

Texture2DArray<float4> displacementAndDerivativeMap : register(t0);
Texture2DArray<float> foamHistoryMap : register(t1);

struct PSInput {
    float4 position : SV_POSITION;
    float3 worldPosition : TEXCOORD0;
    float2 samplePositionXY : TEXCOORD1;
#ifdef OUTPUT_VELOCITY
    float4 currentVelocityPosition : TEXCOORD2;
    float4 prevVelocityPosition : TEXCOORD3;
#endif
};

float GetCascadePatchSize(uint cascadeIndex) {
    return oceanSurfaceParams.cascadePatchSizes[cascadeIndex];
}

float3 SampleOceanDisplacement(float2 worldPositionXY) {
    float3 displacement = 0.0f;

    [unroll]
    for (uint cascadeIndex = 0; cascadeIndex < OceanCascadeCount; ++cascadeIndex) {
        const float2 uv = worldPositionXY / GetCascadePatchSize(cascadeIndex);
        const float displacementSlice = float(2u * cascadeIndex);
        displacement += displacementAndDerivativeMap.SampleLevel(linearWrapSampler, float3(uv, displacementSlice), 0.0f).xyz;
    }
    return displacement * oceanSurfaceParams.waveAmplitude;
}

float3 SampleOceanReflection(float3 reflectionDir, float roughness) {
    if (cameraConstants.environmentLightSpecularCubeMapIndex != 0u && cameraConstants.environmentLightSpecularMipCount > 0u) {
        const TextureCube<float4> specularCubeMap = GetTextureCube(cameraConstants.environmentLightSpecularCubeMapIndex);
        const float3 cubeMapDirection = mul(cameraConstants.environmentLightAxis, float4(reflectionDir, 0.0f));
        const float mipLevel = roughness * float(cameraConstants.environmentLightSpecularMipCount - 1u);
        return specularCubeMap.SampleLevel(linearClampSampler, cubeMapDirection, mipLevel).rgb * cameraConstants.environmentLightColor * cameraConstants.environmentLightIntensity;
    }

    const float skyGradient = saturate(reflectionDir.z * 0.5f + 0.5f);
    return lerp(float3(0.055f, 0.085f, 0.12f), float3(0.28f, 0.48f, 0.78f), skyGradient);
}

[RootSignature(OCEAN_ROOT_SIGNATURE)]
PSInput VSMain(uint vertexID : SV_VertexID) {
    PSInput result;

    const uint gridX = vertexID % OceanSurfaceGridResolution;
    const uint gridY = vertexID / OceanSurfaceGridResolution;
    const float gridHalfExtent = float(OceanSurfaceGridResolution - 1u) * 0.5f;
    const float2 gridPosition = float2(gridX, gridY) - gridHalfExtent;

    // 카메라 이동 시 grid 를 한 vertex 간격으로만 이동시켜 정점 위치의 연속성을 유지한다.
    const float2 snappedCameraPosition = floor(cameraConstants.cameraOrigin.xy / oceanSurfaceParams.gridSpacing + 0.5f) * oceanSurfaceParams.gridSpacing;
    const float2 samplePositionXY = snappedCameraPosition + gridPosition * oceanSurfaceParams.gridSpacing;
    const float3 displacement = SampleOceanDisplacement(samplePositionXY);
    const float3 worldPosition = float3(samplePositionXY + displacement.xy, oceanSurfaceParams.surfaceHeight + displacement.z);
    const float4 worldPosition4 = float4(worldPosition, 1.0f);

    result.position = mul(cameraConstants.viewProjMatrix, worldPosition4);
    result.worldPosition = worldPosition;
    result.samplePositionXY = samplePositionXY;

#ifdef OUTPUT_VELOCITY
    // 이전 displacement texture 는 보존하지 않으므로 우선 카메라 이동에 의한 velocity 만 기록한다.
    result.currentVelocityPosition = mul(cameraConstants.unjitteredViewProjMatrix, worldPosition4);
    result.prevVelocityPosition = mul(cameraConstants.prevUnjitteredViewProjMatrix, worldPosition4);
#endif
    return result;
}

[RootSignature(OCEAN_ROOT_SIGNATURE)]
CameraPassPSOutput PSMain(PSInput input) {
    float displacementX_dy = 0.0f;
    float4 derivatives = 0.0f;
    float foamComplement = 1.0f;

    [unroll]
    for (uint cascadeIndex = 0; cascadeIndex < OceanCascadeCount; ++cascadeIndex) {
        const float2 uv = input.samplePositionXY / GetCascadePatchSize(cascadeIndex);
        const float displacementSlice = float(2u * cascadeIndex);
        const float derivativeSlice = displacementSlice + 1.0f;
        
        displacementX_dy += displacementAndDerivativeMap.Sample(linearWrapSampler, float3(uv, displacementSlice)).w;
        derivatives += displacementAndDerivativeMap.Sample(linearWrapSampler, float3(uv, derivativeSlice));
        
        const float cascadeFoam = saturate(foamHistoryMap.Sample(linearWrapSampler, float3(uv, float(cascadeIndex))));
        foamComplement *= 1.0f - cascadeFoam;
    }

    displacementX_dy *= oceanSurfaceParams.waveAmplitude;
    derivatives *= oceanSurfaceParams.waveAmplitude;

    const float displacementZ_dx = derivatives.x;
    const float displacementZ_dy = derivatives.y;
    const float displacementX_dx = derivatives.z;
    const float displacementY_dy = derivatives.w;
    const float foam = 1.0f - foamComplement;

    // Horizontal displacement Jacobian determinant
    const float J = (1.0f + displacementX_dx) * (1.0f + displacementY_dy) - displacementX_dy * displacementX_dy;

    // Surface normal
    const float3 unnormalizedNormal = float3(displacementZ_dy * displacementX_dy - displacementZ_dx * (1.0f + displacementY_dy), displacementZ_dx * displacementX_dy - displacementZ_dy * (1.0f + displacementX_dx), J);
    const float normalLengthSquared = dot(unnormalizedNormal, unnormalizedNormal);
    const float3 normal = normalLengthSquared > 0.0f ? unnormalizedNormal * rsqrt(normalLengthSquared) : float3(0.0f, 0.0f, 1.0f);

    const float3 viewDir = normalize(cameraConstants.cameraOrigin - input.worldPosition);
    const float NdotV = saturate(dot(normal, viewDir));
    const float fresnel = oceanSurfaceParams.fresnelF0 + (1.0f - oceanSurfaceParams.fresnelF0) * Pow5(1.0f - NdotV);

    const float3 reflectionDir = reflect(-viewDir, normal);
    const float3 reflection = SampleOceanReflection(reflectionDir, oceanSurfaceParams.roughness);
    const float3 waterBodyColor = oceanSurfaceParams.waterColor * lerp(0.35f, 1.0f, NdotV);
    float3 color = lerp(waterBodyColor, reflection, fresnel);

    if ((cameraConstants.flags & ShaderCameraFlagUseDirectionalLight) != 0u) {
        const float3 lightDir = -frameConstants.directionalLight.direction;
        const float NdotL = saturate(dot(normal, lightDir));
        
        if (NdotL > 0.0f) {
            const float3 halfDir = normalize(lightDir + viewDir);
            const float roughnessSquared = max(oceanSurfaceParams.roughness * oceanSurfaceParams.roughness, 0.0004f);
            const float specularPower = max(2.0f / roughnessSquared - 2.0f, 1.0f);
            const float specularNormalization = (specularPower + 2.0f) * (0.125f * INV_PI);
            const float specular = pow(saturate(dot(normal, halfDir)), specularPower) * specularNormalization;
            const float halfDotView = saturate(dot(halfDir, viewDir));
            const float sunFresnel = oceanSurfaceParams.fresnelF0 + (1.0f - oceanSurfaceParams.fresnelF0) * Pow5(1.0f - halfDotView);
            
            color += frameConstants.directionalLight.lightColor * NdotL * (oceanSurfaceParams.waterColor * 0.02f + specular * sunFresnel);
        }
    }

    color = lerp(color, float3(0.9f, 0.95f, 1.0f), foam);

    CameraPassPSOutput result;
    result.color = float4(max(color, 0.0f), 1.0f);

#ifdef OUTPUT_NORMAL
    result.normal = EncodeOctahedralNormal(normal);
#endif
#ifdef OUTPUT_VELOCITY
    result.velocity = CalcVelocity(input.currentVelocityPosition, input.prevVelocityPosition);
#endif
    return result;
}
