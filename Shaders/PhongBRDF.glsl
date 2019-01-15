#ifndef PHONG_BRDF_INCLUDED
#define PHONG_BRDF_INCLUDED

$include "BRDFLibrary.glsl"

// Phong/Blinn-Phong lighting
vec3 DirectLit_Phong(vec3 L, vec3 N, vec3 V, vec3 albedo, vec3 specular, float specularPower) {
    float NdotL = dot(N, L);

#if defined(_WRAPPED_DIFFUSE)
    float oneMinusW = 1.0 + wrappedDiffuse;
    vec3 Cd = albedo.rgb * (NdotL + wrappedDiffuse) / (oneMinusW * oneMinusW);
    NdotL = max(NdotL, 0.0);
#else // Lambertian
    NdotL = max(NdotL, 0.0);
    vec3 Cd = albedo.rgb * NdotL;
#endif

#ifdef USE_BLINN_PHONG
    vec3 H = normalize(L + V);

    float NdotH = max(dot(N, H), 0.0);

    float normFactor = specularPower * 0.125 + 0.25;

    vec3 Cs = specular.rgb * normFactor * pow(NdotH, specularPower) * NdotL;
#else
    vec3 R = reflect(-L, N);

    float RdotV = max(dot(R, V), 0.0);

    float normFactor = specularPower * 0.5 + 1.0;

    vec3 Cs = specular.rgb * normFactor * pow(RdotV, specularPower);
#endif

    return Cd * (vec3(1.0) - specular.rgb) + Cs;
}

// Phong/Blinn-Phong lighting with Fresnel
vec3 DirectLit_PhongFresnel(vec3 L, vec3 N, vec3 V, vec3 albedo, vec3 F0, float specularPower) {
    float NdotL = dot(N, L);

#if defined(_WRAPPED_DIFFUSE)
    float oneMinusW = 1.0 + wrappedDiffuse;
    vec3 Cd = albedo.rgb * (NdotL + wrappedDiffuse) / (oneMinusW * oneMinusW);
    NdotL = max(NdotL, 0.0);
#else // Lambertian
    NdotL = max(NdotL, 0.0);
    vec3 Cd = albedo.rgb * NdotL;
#endif

#ifdef USE_BLINN_PHONG // Microfacet Blinn-Phong
    vec3 H = normalize(L + V);

    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);
    
    // Fresnel reflection term
    vec3 F = F_SchlickSG(F0.rgb, VdotH);

    float normFactor = specularPower * 0.125 + 0.25;

    // Final specular lighting
    vec3 Cs = F * normFactor * pow(NdotH, specularPower) * NdotL;
#else
    vec3 R = reflect(-L, N);

    float RdotV = max(dot(R, V), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    // Fresnel reflection term
    vec3 F = F_SchlickSG(F0.rgb, NdotV);

    float normFactor = specularPower * 0.5 + 1.0;

    // Final specular lighting
    vec3 Cs = F * normFactor * pow(RdotV, specularPower);
#endif

    // Final diffuse lighting
    // From reflection term F, we can directly calculate the ratio of refraction
    return Cd * (vec3(1.0) - F) + Cs;
}

vec3 IndirectLit_PhongFresnel(vec3 N, vec3 S, float NdotV, vec3 albedo, vec3 F0, float specularPower, float roughness) {
    vec3 d1 = texCUBE(irradianceEnvCubeMap0, N).rgb;
    //vec3 d2 = texCUBE(irradianceEnvCubeMap1, N).rgb;

    vec3 Cd = albedo * d1;//mix(d1, d2, ambientLerp);

    // (log2(specularPower) - log2(maxSpecularPower)) / log2(pow(maxSpecularPower, -1/numMipmaps))
    // (log2(specularPower) - 11) / (-11/8)
    float specularMipLevel = -(8.0 / 11.0) * log2(specularPower) + 8.0;

    vec4 sampleVec;
    sampleVec.xyz = S;
    sampleVec.w = specularMipLevel;

    // This is single cubemap texture lookup with Phong not Blinn-Phong
    vec3 s1 = texCUBElod(prefilteredEnvCubeMap0, sampleVec).rgb;
    //vec3 s2 = texCUBElod(prefilteredEnvCubeMap1, sampleVec).rgb;

    vec3 F = F_SchlickRoughness(F0, roughness, NdotV);

    vec3 Cs = F * s1;

    return Cd * (vec3(1.0) - F) + Cs;
}

#endif
