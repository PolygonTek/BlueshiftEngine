#ifndef __IBL_INTEGRATION_HLSLI__
#define __IBL_INTEGRATION_HLSLI__

#include "BRDFLibrary.hlsli"
#include "ImportanceSampling.hlsli"

#define USE_MIPMAP_FILTERED_SAMPLING

static const uint IntegrateLDSampleGridSize = 32u;
static const uint IntegrateDFGSampleGridSize = 100u;

// Computes the LD term (first sum in "Real Shading in Unreal Engine 4", page 6).
// N must be normalized and linearRoughness must be greater than or equal to MinLinearRoughness.
// When USE_MIPMAP_FILTERED_SAMPLING is defined, radianceCubeMapSize must be greater than 0.
float3 IntegrateLD(float3 N, float linearRoughness, TextureCube<float4> radianceCubeMap, SamplerState radianceSampler, float radianceCubeMapSize) {
    const float3x3 tangentToWorld = GetLocalFrame(N);

    // As we don't know the view direction when convoluting the environment map, make a further
    // approximation by assuming the view direction is always equal to the surface normal.
    // This avoids lengthy reflections at grazing angles.
    const float3 V = N;

    float3 color = 0.0f;
    float totalWeight = 0.0f;

    const float sampleGridSizeRcp = rcp((float)IntegrateLDSampleGridSize);

#ifdef USE_MIPMAP_FILTERED_SAMPLING
    const float sampleCount = (float)(IntegrateLDSampleGridSize * IntegrateLDSampleGridSize);
    const float maxMipLevel = log2(radianceCubeMapSize);

    // Inverse solid angle associated with a cubemap texel.
    const float invOmegaP = 6.0f * radianceCubeMapSize * radianceCubeMapSize * INV_FOUR_PI;
#endif

    [loop]
    for (uint y = 0u; y < IntegrateLDSampleGridSize; ++y) {
        [loop]
        for (uint x = 0u; x < IntegrateLDSampleGridSize; ++x) {
            const float2 xi = float2(x, y) * sampleGridSizeRcp;
            const float3 H = mul(ImportanceSampleGGX(xi, linearRoughness), tangentToWorld);
            const float3 L = 2.0f * dot(V, H) * H - V;

            // Integrate { Li * NdotL }.
            const float NdotL = max(dot(N, L), 0.0f);
            if (NdotL <= 0.0f) {
                continue;
            }

#ifdef USE_MIPMAP_FILTERED_SAMPLING
            // Use a lower mipmap level for samples with low probability to reduce variance.
            const float NdotH = saturate(dot(N, H));

            // N == V and therefore NdotH == VdotH.
            // PDF(H) = D * NdotH
            // PDF(L) = D * NdotH / (4 * VdotH)
            //        = D / 4
            const float pdf = D_GGX(NdotH, linearRoughness) * 0.25f;

            // Solid angle associated with a sample.
            const float omegaS = rcp(sampleCount * pdf);
            const float mipLevel = clamp(0.5f * log2(omegaS * invOmegaP) + 1.0f, 0.0f, maxMipLevel);
#else
            const float mipLevel = 0.0f;
#endif

            color += radianceCubeMap.SampleLevel(radianceSampler, L, mipLevel).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    return color / totalWeight;
}

// Computes the DFG term (second sum in "Real Shading in Unreal Engine 4", page 6).
// NdotV must be in the (0, 1] range and roughness must be in the [0, 1] range.
float2 IntegrateDFG(float NdotV, float roughness) {
    const float3 N = float3(0.0f, 0.0f, 1.0f);
    const float3x3 tangentToWorld = GetLocalFrame(N);

    // theta = cos^-1(NdotV), phi = 0
    const float3 V = float3(sqrt(1.0f - NdotV * NdotV), 0.0f, NdotV);

    float A = 0.0f;
    float B = 0.0f;

    const float linearRoughness = roughness * roughness;
    const float k = linearRoughness * 0.5f; // k for IBL
    const float sampleGridSizeRcp = rcp((float)IntegrateDFGSampleGridSize);

    [loop]
    for (uint y = 0u; y < IntegrateDFGSampleGridSize; ++y) {
        [loop]
        for (uint x = 0u; x < IntegrateDFGSampleGridSize; ++x) {
            const float2 xi = float2(x, y) * sampleGridSizeRcp;
            const float3 H = mul(ImportanceSampleGGX(xi, linearRoughness), tangentToWorld);
            const float3 L = normalize(2.0f * dot(V, H) * H - V);

            const float NdotL = max(L.z, 0.0f);
            if (NdotL <= 0.0f) {
                continue;
            }

            const float NdotH = max(H.z, 0.0f);
            const float VdotH = max(dot(V, H), 0.0f);

            const float G = G_SchlickGGX(NdotV, NdotL, k);
            // BRDF/F = D * G (G term is divided by (4 * NdotL * NdotV)).
            //
            // PDF(H) = D * NdotH
            // PDF(L) = PDF(H) * dH / dL = D * NdotH / (4 * VdotH) (PBRT 2nd Edition, page 698)
            //
            // Integrate { BRDF/F * NdotL }.
            //
            // F_N = 1/N Sigma^N { BRDF/F * NdotL / PDF(L) }
            //     = 1/N Sigma^N { (D * G * NdotL) / (D * NdotH / (4 * VdotH)) }
            //     = 1/N Sigma^N { 4 * G * NdotL * VdotH / NdotH }
            const float G_Vis = 4.0f * G * NdotL * VdotH / NdotH;
            const float Fc = Pow5(1.0f - VdotH);

            A += (1.0f - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    const float sampleCount = (float)(IntegrateDFGSampleGridSize * IntegrateDFGSampleGridSize);
    return float2(A, B) / sampleCount;
}

#endif // __IBL_INTEGRATION_HLSLI__
