#ifndef STANDARD_BRDF_INCLUDED
#define STANDARD_BRDF_INCLUDED

$include "StandardConfig.glsl"
$include "BRDFLibrary.glsl"

vec3 DirectLit_Standard(vec3 L, vec3 N, vec3 V, vec3 albedo, vec3 F0, float roughness) {
    vec3 H = normalize(L + V);

    float NdotL = saturate(dot(N, L));
    float NdotH = saturate(dot(N, H));
    float NdotV = saturate(dot(N, V));
    float VdotH = saturate(dot(V, H));

    // We adopted Disney's reparameterization of a = roughness^2
    // a means perceptual linear roughness.
    float linearRoughness = roughness * roughness;

    //----------------------------------
    // Specular BRDF
    //----------------------------------

    // Fresnel reflection term
    vec3 F = F_SchlickSG(F0, VdotH);

    // Normal distribution term
#if PBR_SPEC_D == PBR_SPEC_D_BLINN
    float D = D_Blinn(NdotH, linearRoughness);
#elif PBR_SPEC_D == PBR_SPEC_D_BECKMANN
    float D = D_Beckmann(NdotH, linearRoughness);
#elif PBR_SPEC_D == PBR_SPEC_D_GGX
    float D = D_GGX(NdotH, linearRoughness);
#elif PBR_SPEC_D == PBR_SPEC_D_GGX_ANISO
    float D = D_GGXAniso(NdotH, XdotH, YdotH, ax, ay);
#endif

    // Geometric visibility term
#if PBR_SPEC_G == PBR_SPEC_G_NEUMANN
    float G = G_Neumann(NdotV, NdotL);
#elif PBR_SPEC_G == PBR_SPEC_G_KELEMEN
    float G = G_Kelemen(VdotH);
#elif PBR_SPEC_G == PBR_SPEC_G_COOK_TORRANCE
    float G = G_CookTorrance(NdotV, NdotL, NdotH, VdotH);
#elif PBR_SPEC_G == PBR_SPEC_G_GGX
    // Disney's modification to reduce "hotness" by remapping roughness using (roughness + 1) / 2 before squaring.
    float k = roughness + 1.0; // k for direct lighting
    float G = G_SchlickGGX(NdotV, NdotL, (k * k) * 0.125);
#endif

    // Microfacets specular BRDF = D * G * F / 4 (G term is already divided by (NdotL * NdotV))
    vec3 BRDFspec = D * G * F * 0.25;

    // Final specular lighting
    // Incident radiance is translated to LightColor * PI in direct lighting computation
    vec3 Cs = BRDFspec * NdotL * PI;

    //----------------------------------
    // Diffuse BRDF (already multiplied by PI)
    //----------------------------------

#if PBR_DIFFUSE == PBR_DIFFUSE_LAMBERT
    vec3 Cd = albedo * Fd_Lambert(NdotL);
#elif PBR_DIFFUSE == PBR_DIFFUSE_WRAPPED
    vec3 Cd = albedo * Fd_Wrap(NdotL, diffuseWrap);
#elif PBR_DIFFUSE == PBR_DIFFUSE_BURLEY
    vec3 Cd = albedo * Fd_Burley(NdotL, NdotV, VdotH, linearRoughness);
#elif PBR_DIFFUSE == PBR_DIFFUSE_OREN_NAYAR
    float LdotV = max(dot(L, V), 0);
    vec3 Cd = albedo * Fd_OrenNayar(NdotL, NdotV, LdotV, linearRoughness);
#endif

    // Final diffuse lighting
    // From reflection term F, we can directly calculate the ratio of refraction
    Cd *= (vec3(1.0) - F);

    return Cd + Cs;
}

vec3 IndirectLit_Standard(vec3 N, vec3 S, float NdotV, vec3 albedo, vec3 F0, float roughness) {
    vec3 d1 = texCUBE(irradianceEnvCubeMap0, N).rgb;
    //vec3 d2 = texCUBE(irradianceEnvCubeMap1, N).rgb;

#if USE_SRGB_TEXTURE == 0
    d1 = linearToGammaSpace(d1);
    //d2 = linearToGammaSpace(d2);
#endif

    vec3 Cd = albedo * d1;//mix(d1, d2, ambientLerp);

    vec4 sampleVec;
    sampleVec.xyz = S;
    sampleVec.w = roughness * 7.0; // FIXME: 7.0 == maximum mip level

    vec3 s1 = texCUBElod(prefilteredEnvCubeMap0, sampleVec).rgb;
    //vec3 s2 = texCUBElod(prefilteredEnvCubeMap1, sampleVec).rgb;

#if USE_SRGB_TEXTURE == 0
    s1 = linearToGammaSpace(s1);
    //s2 = linearToGammaSpace(s2);
#endif

    vec2 envBRDF = tex2D(integrationLUTMap, vec2(NdotV, roughness)).xy;

    vec3 F = F_SchlickRoughness(F0, roughness, NdotV);
    vec3 Cs = (F * envBRDF.x + envBRDF.y) * s1;

    return Cd * (vec3(1.0) - F) + Cs;
}

#endif
