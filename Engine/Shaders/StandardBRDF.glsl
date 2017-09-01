#ifndef STANDARD_BRDF_INCLUDED
#define STANDARD_BRDF_INCLUDED

$include "StandardConfig.glsl"
$include "BRDFLibrary.glsl"

vec3 DirectLit_Standard(vec3 L, vec3 N, vec3 V, vec3 albedo, vec3 F0, float roughness) {
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    //----------------------------------
    // Diffuse BRDF
    //----------------------------------
#if PBR_DIFFUSE == 0
    vec3 Cd = albedo * litDiffuseLambert(NdotL);
#elif PBR_DIFFUSE == 1
    vec3 Cd = albedo * litDiffuseBurley(NdotL, NdotV, VdotH, sqrt(roughness));
#elif PBR_DIFFUSE == 2
    float LdotV = max(dot(L, V), 0);
    vec3 Cd = albedo * litDiffuseOrenNayar(NdotL, NdotV, LdotV, roughness);
#endif

    //----------------------------------
    // Specular BRDF
    //----------------------------------
    float m = roughness * roughness;

    // Normal distribution term
#if PBR_SPEC_D == 0
    float D = D_Blinn(NdotH, m);
#elif PBR_SPEC_D == 1
    float D = D_Beckmann(NdotH, m);
#elif PBR_SPEC_D == 2
    float D = D_GGX(NdotH, m);
#elif PBR_SPEC_D == 3
    float D = D_GGXAniso(NdotH, XdotH, YdotH, mx, my);
#endif

    // Geometric visibility term
#if PBR_SPEC_G == 0
    float G = G_Neumann(NdotV, NdotL);
#elif PBR_SPEC_G == 1
    float G = G_Kelemen(VdotH);
#elif PBR_SPEC_G == 2
    float G = G_CookTorrance(NdotV, NdotL, NdotH, VdotH);
#elif PBR_SPEC_G == 3
    float k = roughness + 1.0; // k for direct lighting
    float G = G_SchlickGGX(NdotV, NdotL, (k * k) * 0.125);
#endif

    // Specular Fresnel term
    vec3 F = F_SchlickSG(F0, VdotH);

    // Microfacets specular BRDF = D * G * F / 4 (G term is divided by (NdotL * NdotV))
    vec3 BRDFspec = D * G * F * 0.25;

    // Final specular lighting
    vec3 Cs = PI * NdotL * BRDFspec;

    // Final diffuse lighting with fresnel
    Cd *= (vec3(1.0) - F);

    return Cd + Cs;
}

vec3 IndirectLit_Standard(vec3 worldN, vec3 worldS, float NdotV, vec3 albedo, vec3 F0, float roughness) {
    vec3 d1 = texCUBE(irradianceEnvCubeMap0, worldN).rgb;
    //vec3 d2 = texCUBE(irradianceEnvCubeMap1, worldN).rgb;

    vec3 Cd = albedo * d1;//mix(d1, d2, ambientLerp);

    vec4 sampleVec;
    sampleVec.xyz = worldS;
    sampleVec.w = roughness * 7.0; // FIXME: 7.0 == maximum mip level

    vec3 s1 = texCUBElod(prefilteredEnvCubeMap0, sampleVec).rgb;
    //vec3 s2 = texCUBElod(prefilteredEnvCubeMap1, sampleVec).rgb;

    vec2 envBRDF = tex2D(integrationLUTMap, vec2(NdotV, roughness)).xy;

    vec3 F = F_SchlickRoughness(F0, roughness, NdotV);
    vec3 Cs = (F * envBRDF.x + envBRDF.y) * s1;

    return Cd * (vec3(1.0) - F) + Cs;
}

#endif
