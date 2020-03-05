#ifndef SHADING_PARMS_INCLUDED
#define SHADING_PARMS_INCLUDED

struct ShadingParms {
    MEDIUMP vec4 diffuse;
    MEDIUMP vec4 specular;
    MEDIUMP float roughness;
    MEDIUMP float linearRoughness;

    MEDIUMP vec3 v; // view vector in world space
    MEDIUMP vec3 l; // light vector in world space
    MEDIUMP vec3 n; // normal vector in world space
    MEDIUMP vec3 s0;
    MEDIUMP vec3 s1;
    MEDIUMP float ndotv;
    MEDIUMP vec2 preDFG;
    HIGHP vec3 energyCompensation;

#if _NORMAL != 0 || _ANISO != 0 || (_CLEARCOAT != 0 && _CC_NORMAL == 1)
    HIGHP mat3 tagentToWorldMatrix;
#endif

#ifdef LEGACY_PHONG_LIGHTING
    MEDIUMP float specularPower;
#endif

#if _ANISO != 0
    MEDIUMP vec3 anisotropicT;
    MEDIUMP vec3 anisotropicB;
    MEDIUMP float anisotropy;
#endif

#if _CLEARCOAT != 0
    MEDIUMP float clearCoat;
    MEDIUMP float clearCoatRoughness;
    MEDIUMP float clearCoatLinearRoughness;
    MEDIUMP vec3 clearCoatN;
#endif

#if _OCC != 0
    MEDIUMP float ambientOcclusion;
#endif
};

ShadingParms shading;

void PrepareShadingParms(vec4 albedo) {
    shading.v = normalize(v2f_viewWS.xyz);

#if _NORMAL != 0 || _ANISO != 0 || (_CLEARCOAT != 0 && _CC_NORMAL == 1)
    shading.tagentToWorldMatrix[0] = normalize(v2f_tangentToWorldAndPackedWorldPosS.xyz);
    shading.tagentToWorldMatrix[1] = normalize(v2f_tangentToWorldAndPackedWorldPosT.xyz);
    shading.tagentToWorldMatrix[2] = normalize(v2f_tangentToWorldAndPackedWorldPosR.xyz);

    #if _NORMAL != 0
        MEDIUMP vec3 normalTS = normalize(GetNormal(normalMap, baseTc));

        #if _NORMAL == 2
            MEDIUMP vec3 detailNormalTS = vec3(tex2D(detailNormalMap, baseTc * detailRepeat).xy * 2.0 - 1.0, 0.0);
            normalTS = normalize(normalTS + detailNormalTS);
        #endif

        // Convert coordinates from tangent space to GL world space
        shading.n = shading.tagentToWorldMatrix * normalTS;
    #else
        shading.n = shading.tagentToWorldMatrix[2];
    #endif
#else
    shading.n = normalize(v2f_normalWS.xyz);
#endif

#ifdef TWO_SIDED
    shading.n = gl_FrontFacing ? shading.n : -shading.n;
#endif

    shading.ndotv = max(dot(shading.n, shading.v), 1e-4); // avoid artifact

#if defined(STANDARD_METALLIC_LIGHTING)
    #if _METALLIC == 0
        MEDIUMP vec4 metallic = vec4(1.0, 0.0, 0.0, 0.0);
    #elif _METALLIC >= 1
        MEDIUMP vec4 metallic = tex2D(metallicMap, baseTc);
    #endif

    #if _METALLIC == 0
        MEDIUMP float metalness = metallicScale;
    #elif _METALLIC == 1
        MEDIUMP float metalness = metallic.r * metallicScale;
    #elif _METALLIC == 2
        MEDIUMP float metalness = metallic.g * metallicScale;
    #elif _METALLIC == 3
        MEDIUMP float metalness = metallic.b * metallicScale;
    #elif _METALLIC == 4
        MEDIUMP float metalness = metallic.a * metallicScale;
    #endif

    #if _ROUGHNESS == 0
        shading.roughness = roughnessScale;
    #elif _ROUGHNESS == 1
        shading.roughness = tex2D(roughnessMap, baseTc).r * roughnessScale;
    #elif _ROUGHNESS == 2
        shading.roughness = (1.0 - tex2D(roughnessMap, baseTc).r) * roughnessScale;
    #elif _ROUGHNESS == 3
        shading.roughness = metallic.r * roughnessScale;
    #elif _ROUGHNESS == 4
        shading.roughness = metallic.g * roughnessScale;
    #elif _ROUGHNESS == 5
        shading.roughness = metallic.b * roughnessScale;
    #elif _ROUGHNESS == 6
        shading.roughness = metallic.a * roughnessScale;
    #endif

    #if _ANISO != 0
        shading.anisotropy = anisotropy;

        #if _ANISO == 1
            MEDIUMP vec3 anisotropyDir = vec3(1.0, 0.0, 0.0);
        #elif _ANISO == 2
            MEDIUMP vec3 anisotropyDir = normalize(GetNormal(anisotropyMap, baseTc));
        #endif
    #endif

    #if _CLEARCOAT != 0
        #if _CLEARCOAT == 1
            shading.clearCoat = clearCoatScale;
        #elif _CLEARCOAT == 2
            shading.clearCoat = tex2D(clearCoatMap, baseTc).r * clearCoatScale;
        #elif _CLEARCOAT == 3
            shading.clearCoat = metallic.r * clearCoatScale;
        #elif _CLEARCOAT == 4
            shading.clearCoat = metallic.g * clearCoatScale;
        #elif _CLEARCOAT == 5
            shading.clearCoat = metallic.b * clearCoatScale;
        #elif _CLEARCOAT == 6
            shading.clearCoat = metallic.a * clearCoatScale;
        #endif

        #if _CC_ROUGHNESS == 0
            shading.clearCoatRoughness = clearCoatRoughnessScale;
        #elif _CC_ROUGHNESS == 1
            shading.clearCoatRoughness = tex2D(clearCoatRoughnessMap, baseTc).r * clearCoatRoughnessScale;
        #elif _CC_ROUGHNESS == 2
            shading.clearCoatRoughness = metallic.r * clearCoatRoughnessScale;
        #elif _CC_ROUGHNESS == 3
            shading.clearCoatRoughness = metallic.g * clearCoatRoughnessScale;
        #elif _CC_ROUGHNESS == 4
            shading.clearCoatRoughness = metallic.b * clearCoatRoughnessScale;
        #elif _CC_ROUGHNESS == 5
            shading.clearCoatRoughness = metallic.a * clearCoatRoughnessScale;
        #endif
    #endif

    #if _OCC == 1
        shading.ambientOcclusion = tex2D(occlusionMap, baseTc).r;
    #elif _OCC == 2
        shading.ambientOcclusion = metallic.r;
    #elif _OCC == 3
        shading.ambientOcclusion = metallic.g;
    #elif _OCC == 4
        shading.ambientOcclusion = metallic.b;
    #elif _OCC == 5
        shading.ambientOcclusion = metallic.a;
    #endif

    // A base reflectivity (F0) of 0.04 holds for most dielectrics
    shading.specular = vec4(mix(vec3(0.04), albedo.rgb, metalness), 1.0);
    //shading.specular = vec4(mix(0.16 * shading.reflectance * shading.reflectance, albedo.rgb, metalness), 1.0);

    shading.diffuse = vec4(albedo.rgb * (1.0 - metalness), albedo.a);
#elif defined(STANDARD_SPECULAR_LIGHTING)
    shading.diffuse = albedo;

    #if _SPECULAR == 0
        shading.specular = vec4(specularColor, specularAlpha);
    #elif _SPECULAR == 1
        shading.specular = tex2D(specularMap, baseTc);
    #endif

    #if _GLOSS == 0
        MEDIUMP float glossiness = glossScale;
    #elif _GLOSS == 1
        MEDIUMP float glossiness = albedo.a * glossScale;
    #elif _GLOSS == 2
        MEDIUMP float glossiness = shading.specular.a * glossScale;
    #elif _GLOSS == 3
        MEDIUMP float glossiness = tex2D(glossMap, baseTc).r * glossScale;
    #endif

    #if _ANISO != 0
        shading.anisotropy = anisotropy;

        #if _ANISO == 1
            MEDIUMP vec3 anisotropyDir = vec3(1.0, 0.0, 0.0);
        #elif _ANISO == 2
            MEDIUMP vec3 anisotropyDir = normalize(GetNormal(anisotropyMap, baseTc));
        #endif
    #endif

    #if _CLEARCOAT != 0
        #if _CLEARCOAT == 1
            shading.clearCoat = clearCoatScale;
        #elif _CLEARCOAT == 2
            shading.clearCoat = tex2D(clearCoatMap, baseTc).r * clearCoatScale;
        #elif _CLEARCOAT == 3
            shading.clearCoat = albedo.a * clearCoatScale;
        #elif _CLEARCOAT == 4
            shading.clearCoat = shading.specular.a * clearCoatScale;
        #endif

        #if _CC_ROUGHNESS == 0
            shading.clearCoatRoughness = clearCoatRoughnessScale;
        #elif _CC_ROUGHNESS == 1
            shading.clearCoatRoughness = tex2D(clearCoatRoughnessMap, baseTc).r * clearCoatRoughnessScale;
        #elif _CC_ROUGHNESS == 2
            shading.clearCoatRoughness = albedo.a * clearCoatRoughnessScale;
        #elif _CC_ROUGHNESS == 3
            shading.clearCoatRoughness = shading.specular.a * clearCoatRoughnessScale;
        #endif
    #endif

    #if _OCC == 1
        shading.ambientOcclusion = tex2D(occlusionMap, baseTc).r;
    #elif _OCC == 2
        shading.ambientOcclusion = albedo.a;
    #elif _OCC == 3
        shading.ambientOcclusion = shading.specular.a;
    #endif

    shading.roughness = 1.0 - glossiness;
#elif defined(LEGACY_PHONG_LIGHTING)
    shading.diffuse = albedo;

    #if _SPECULAR == 0
        shading.specular = vec4(specularColor, specularAlpha);
    #elif _SPECULAR == 1
        shading.specular = tex2D(specularMap, baseTc);
    #endif

    #if _GLOSS == 0
        MEDIUMP float glossiness = glossScale;
    #elif _GLOSS == 1
        MEDIUMP float glossiness = albedo.a * glossScale;
    #elif _GLOSS == 2
        MEDIUMP float glossiness = shading.specular.a * glossScale;
    #elif _GLOSS == 3
        MEDIUMP float glossiness = tex2D(glossMap, baseTc).r * glossScale;
    #endif

    #if _OCC == 1
        shading.ambientOcclusion = tex2D(occlusionMap, baseTc).r;
    #elif _OCC == 2
        shading.ambientOcclusion = albedo.a;
    #elif _OCC == 3
        shading.ambientOcclusion = shading.specular.a;
    #endif

    shading.roughness = 1.0 - glossiness;

    shading.specularPower = GlossinessToSpecularPower(glossiness);
#endif

#if _OCC != 0
    shading.ambientOcclusion = mix(1.0, shading.ambientOcclusion, occlusionStrength);
#endif

    // Clamp the roughness to a minimum value to avoid divisions by 0 in the lighting code
    shading.roughness = clamp(shading.roughness, MIN_ROUGHNESS, 1.0);

    // We adopted Disney's reparameterization of linearRoughness = roughness^2
    shading.linearRoughness = shading.roughness * shading.roughness;

#if defined(STANDARD_METALLIC_LIGHTING) || defined(STANDARD_SPECULAR_LIGHTING)
    #if defined(INDIRECT_LIGHTING) || defined(USE_MULTIPLE_SCATTERING_COMPENSATION)
        shading.preDFG = tex2D(prefilteredDfgMap, vec2(shading.ndotv, shading.roughness)).xy;
    #else
        shading.preDFG = vec2(1.0, 1.0);
    #endif

    #ifdef USE_MULTIPLE_SCATTERING_COMPENSATION
        // Energy compensation for multiple scattering in a microfacet model
        // See "Multiple-Scattering Microfacet BSDFs with the Smith Model"
        shading.energyCompensation = 1.0 + shading.specular.rgb * (1.0 / shading.preDFG.x - 1.0);
    #else
        shading.energyCompensation = vec3(1.0);
    #endif

    #if _ANISO != 0
        shading.anisotropicT = shading.tagentToWorldMatrix * anisotropyDir;
        shading.anisotropicB = normalize(cross(shading.tagentToWorldMatrix[2], shading.anisotropicT));
    #endif

    #if _CLEARCOAT != 0
        shading.specular.rgb = mix(shading.specular.rgb, F0ForAirInterfaceToF0ForClearCoat15(shading.specular.rgb), shading.clearCoat);

        // Remapping of clear coat roughness
        shading.clearCoatRoughness = mix(MIN_CLEARCOAT_ROUGHNESS, MAX_CLEARCOAT_ROUGHNESS, shading.clearCoatRoughness);

        shading.clearCoatLinearRoughness = shading.clearCoatRoughness * shading.clearCoatRoughness;

        #if _CC_NORMAL == 1
            MEDIUMP vec3 tangentClearCoatN = normalize(GetNormal(clearCoatNormalMap, baseTc));

            // Convert coordinates from tangent space to GL world space
            shading.clearCoatN = shading.tagentToWorldMatrix * tangentClearCoatN;
        #else
            shading.clearCoatN = shading.n;
        #endif
    #endif
#endif
}

#endif
