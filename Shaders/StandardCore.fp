$include "fragment_common.glsl"

#ifndef _ALBEDO
#define _ALBEDO 0
#endif

#ifndef _NORMAL
#define _NORMAL 0
#endif

#ifndef _SPECULAR
#define _SPECULAR 0
#endif

#ifndef _GLOSS
#define _GLOSS 0
#endif

#ifndef _METALLIC
#define _METALLIC 0
#endif

#ifndef _ROUGHNESS
#define _ROUGHNESS 0
#endif

#ifndef _ANISO
#define _ANISO 0
#endif

#ifndef _CLEARCOAT
#define _CLEARCOAT 0
#endif

#ifndef _CC_ROUGHNESS
#define _CC_ROUGHNESS 0
#endif

#ifndef _CC_NORMAL
#define _CC_NORMAL 0
#endif

#ifndef _PARALLAX
#define _PARALLAX 0
#endif

#ifndef _EMISSION
#define _EMISSION 0
#endif

#if _NORMAL == 2 && !defined(ENABLE_DETAIL_NORMALMAP)
    #undef _NORMAL
    #define _NORMAL 1
#endif

#if _PARALLAX == 1 && !defined(ENABLE_PARALLAXMAP)
    #undef _PARALLAX 
    #define _PARALLAX 0
#endif

#if _ALBEDO != 0 || _NORMAL != 0 || _SPECULAR != 0 || _GLOSS == 3 || _METALLIC >= 1 || (_ROUGHNESS == 1 || _ROUGHNESS == 2) || _PARALLAX != 0 || _EMISSION == 2 || _CLEARCOAT == 2 || _ANISO == 2 || (_CLEARCOAT != 0 && _CC_NORMAL == 1) || _OCC == 1
    #define NEED_BASE_TC
#endif

in LOWP vec4 v2f_color;

#ifdef NEED_BASE_TC
    in MEDIUMP vec2 v2f_tex;
#endif

#if _NORMAL == 0
    in LOWP vec3 v2f_normal;
#endif

#if _PARALLAX
    in vec3 v2f_tangentViewDir;
#endif

#ifdef DIRECT_LIGHTING
    in vec3 v2f_lightVector;
    in vec3 v2f_lightFallOff;
    in vec4 v2f_lightProjection;
#endif

#if defined(DIRECT_LIGHTING) || defined(INDIRECT_LIGHTING)
    in vec3 v2f_viewVector;

    #if _NORMAL != 0 || _ANISO != 0 || (_CLEARCOAT != 0 && _CC_NORMAL == 1)
        in vec4 v2f_tangentToWorldAndPackedWorldPosS;
        in vec4 v2f_tangentToWorldAndPackedWorldPosT;
        in vec4 v2f_tangentToWorldAndPackedWorldPosR;
    #else
        in vec3 v2f_worldPos;
    #endif
#endif

#ifdef USE_SHADOW_MAP
    $include "ShadowLibrary.fp"
#endif

out vec4 o_fragColor : FRAG_COLOR;

//
// Material parameters
//

uniform LOWP float ambientScale;
uniform LOWP float perforatedAlpha;
uniform LOWP float wrappedDiffuse;

#if _ALBEDO == 0
    uniform LOWP vec3 albedoColor;
    uniform LOWP float albedoAlpha;
#elif _ALBEDO == 1
    uniform sampler2D albedoMap;
#endif

#if _NORMAL != 0
    uniform sampler2D normalMap;
    #if _NORMAL == 2
        uniform sampler2D detailNormalMap;
        uniform MEDIUMP float detailRepeat;
    #endif
#endif

#if defined(STANDARD_METALLIC_LIGHTING)
    uniform LOWP float metallicScale;
    #if _METALLIC >= 1
        uniform sampler2D metallicMap;
    #endif

    uniform LOWP float roughnessScale;
    #if _ROUGHNESS == 1 || _ROUGHNESS == 2
        uniform sampler2D roughnessMap;
    #endif
#elif defined(STANDARD_SPECULAR_LIGHTING) || defined(LEGACY_PHONG_LIGHTING)
    #if _SPECULAR == 0
        uniform LOWP vec4 specularColor;
    #elif _SPECULAR == 1
        uniform sampler2D specularMap;
    #endif

    uniform LOWP float glossScale;
    #if _GLOSS == 3
        uniform sampler2D glossMap;
    #endif
#endif

#if _ANISO != 0
    uniform LOWP float anisotropy;
    #if _ANISO == 2
        uniform sampler2D anisotropyMap;
    #endif
#endif

#if _CLEARCOAT != 0
    uniform LOWP float clearCoatScale;
    #if _CLEARCOAT == 2
        uniform sampler2D clearCoatMap;
    #endif

    uniform LOWP float clearCoatRoughnessScale;
    #if _CC_ROUGHNESS == 2
        uniform sampler2D clearCoatRoughnessMap;
    #endif
    
    #if _CC_NORMAL == 1
        uniform sampler2D clearCoatNormalMap;
    #endif
#endif

#if _PARALLAX != 0
    uniform sampler2D heightMap;
    uniform LOWP float heightScale;
#endif

#if _OCC != 0
    uniform LOWP float occlusionStrength;
    #if _OCC == 1
        uniform sampler2D occlusionMap;
    #endif
#endif

#if _EMISSION != 0
    uniform MEDIUMP float emissionScale;
    #if _EMISSION == 1
        uniform LOWP vec3 emissionColor;
    #elif _EMISSION == 2
        uniform sampler2D emissionMap;
    #endif
#endif

uniform sampler2D subSurfaceColorMap;
uniform float subSurfaceRollOff;
uniform float subSurfaceShadowDensity;// = 0.5;

//
// Direct lighting parameters
//
#if defined(DIRECT_LIGHTING)
    uniform sampler2D lightProjectionMap;
    uniform MEDIUMP vec4 lightColor;
    uniform MEDIUMP float lightFallOffExponent;
    uniform samplerCube lightCubeMap;
    uniform bool useLightCube;
    uniform bool useShadowMap;
#endif

//
// Indirect lighting parameters
//
#if defined(INDIRECT_LIGHTING)
    uniform samplerCube envCubeMap;

    uniform samplerCube irradianceEnvCubeMap0;
    uniform samplerCube prefilteredEnvCubeMap0;
    uniform vec3 probePosition0;
    uniform vec3 probeMins0;
    uniform vec3 probeMaxs0;

    uniform samplerCube irradianceEnvCubeMap1;
    uniform samplerCube prefilteredEnvCubeMap1;
    uniform vec3 probePosition1;
    uniform vec3 probeMins1;
    uniform vec3 probeMaxs1;

    uniform sampler2D integrationLUTMap;
    uniform LOWP float ambientLerp;
#endif

struct ShadingParms {
    vec3 v; // view vector in world space
    vec3 l; // light vector in world space
    vec3 n; // normal vector in world space

    mat3 tagentToWorldMatrix;

    vec4 diffuse;
    vec4 specular;
    float roughness;
    float linearRoughness;

#ifdef LEGACY_PHONG_LIGHTING
    float specularPower;
#endif

#if _ANISO != 0
    vec3 anisotropicT;
    vec3 anisotropicB;
    float anisotropy;
#endif

#if _CLEARCOAT != 0
    float clearCoat;
    float clearCoatRoughness;
    float clearCoatLinearRoughness;
    vec3 clearCoatN;
#endif

#if _OCC != 0
    float occlusion;
#endif

#if _EMISSION != 0
    vec3 emission;
#endif
};

ShadingParms shading;

#if defined(DIRECT_LIGHTING) || defined(INDIRECT_LIGHTING)
    $include "StandardBRDF.glsl"
    #ifdef LEGACY_PHONG_LIGHTING
        $include "PhongBRDF.glsl"
    #endif 
#endif

#if defined(INDIRECT_LIGHTING)
    #ifdef BRUTE_FORCE_IBL
        $include "IBL.glsl"
    #endif
#endif

#ifdef NEED_BASE_TC
    vec2 baseTc;
#endif

//#define PARALLAX_CORRECTED_INDIRECT_LIGHTING

#if defined(DIRECT_LIGHTING) || defined(INDIRECT_LIGHTING)
void PrepareShadingParms(vec4 albedo) {
    shading.v = normalize(v2f_viewVector.yzx);

#if _NORMAL != 0 || _ANISO != 0 || (_CLEARCOAT != 0 && _CC_NORMAL == 1)
    shading.tagentToWorldMatrix[0] = normalize(v2f_tangentToWorldAndPackedWorldPosS.yzx);
    shading.tagentToWorldMatrix[1] = normalize(v2f_tangentToWorldAndPackedWorldPosT.yzx);
    shading.tagentToWorldMatrix[2] = normalize(v2f_tangentToWorldAndPackedWorldPosR.yzx);

    #if _NORMAL != 0
        vec3 tangentN = normalize(GetNormal(normalMap, baseTc));

        #if _NORMAL == 2
            vec3 tangentN2 = vec3(tex2D(detailNormalMap, baseTc * detailRepeat).xy * 2.0 - 1.0, 0.0);
            tangentN = normalize(tangentN + tangentN2);
        #endif

        // Convert coordinates from tangent space to GL world space
        shading.n = shading.tagentToWorldMatrix * tangentN;
    #else
        shading.n = shading.tagentToWorldMatrix[2];
    #endif
#else
    shading.n = normalize(v2f_normal.yzx);
#endif

#ifdef TWO_SIDED
    shading.n = gl_FrontFacing ? shading.n : -shading.n;
#endif

#if defined(STANDARD_METALLIC_LIGHTING)
    #if _METALLIC == 0
        vec4 metallic = vec4(1.0, 0.0, 0.0, 0.0);
    #elif _METALLIC >= 1
        vec4 metallic = tex2D(metallicMap, baseTc);
    #endif

    #if _METALLIC == 0
        float metalness = metallicScale;
    #elif _METALLIC == 1
        float metalness = metallic.r * metallicScale;
    #elif _METALLIC == 2
        float metalness = metallic.g * metallicScale;
    #elif _METALLIC == 3
        float metalness = metallic.b * metallicScale;
    #elif _METALLIC == 4
        float metalness = metallic.a * metallicScale;
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
            vec3 anisotropyDir = vec3(1.0, 0.0, 0.0);
        #elif _ANISO == 2
            vec3 anisotropyDir = normalize(GetNormal(anisotropyMap, baseTc));
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
        shading.occlusion = tex2D(occlusionMap, baseTc).r;
    #elif _OCC == 2
        shading.occlusion = metallic.r;
    #elif _OCC == 3
        shading.occlusion = metallic.g;
    #elif _OCC == 4
        shading.occlusion = metallic.b;
    #elif _OCC == 5
        shading.occlusion = metallic.a;
    #endif

    // A base reflectivity of 0.04 holds for most dielectrics
    shading.specular = vec4(mix(vec3(0.04), albedo.rgb, metalness), 1.0);

    shading.diffuse = vec4(albedo.rgb * (1.0 - metalness), albedo.a);
#elif defined(STANDARD_SPECULAR_LIGHTING)
    shading.diffuse = albedo;

    #if _SPECULAR == 0
        shading.specular = specularColor;
    #elif _SPECULAR == 1
        shading.specular = tex2D(specularMap, baseTc);
    #endif

    #if _GLOSS == 0
        float glossiness = glossScale;
    #elif _GLOSS == 1
        float glossiness = albedo.a * glossScale;
    #elif _GLOSS == 2
        float glossiness = shading.specular.a * glossScale;
    #elif _GLOSS == 3
        float glossiness = tex2D(glossMap, baseTc).r * glossScale;
    #endif

    #if _ANISO != 0
        shading.anisotropy = anisotropy;

        #if _ANISO == 1
            vec3 anisotropyDir = vec3(1.0, 0.0, 0.0);
        #elif _ANISO == 2
            vec3 anisotropyDir = normalize(GetNormal(anisotropyMap, baseTc));
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
        shading.occlusion = tex2D(occlusionMap, baseTc).r;
    #elif _OCC == 2
        shading.occlusion = albedo.a;
    #elif _OCC == 3
        shading.occlusion = shading.specular.a;
    #endif

    shading.roughness = 1.0 - glossiness;
#elif defined(LEGACY_PHONG_LIGHTING)
    shading.diffuse = albedo;

    #if _SPECULAR == 0
        shading.specular = specularColor;
    #elif _SPECULAR == 1
        shading.specular = tex2D(specularMap, baseTc);
    #endif

    #if _GLOSS == 0
        float glossiness = glossScale;
    #elif _GLOSS == 1
        float glossiness = albedo.a * glossScale;
    #elif _GLOSS == 2
        float glossiness = shading.specular.a * glossScale;
    #elif _GLOSS == 3
        float glossiness = tex2D(glossMap, baseTc).r * glossScale;
    #endif

    #if _OCC == 1
        shading.occlusion = tex2D(occlusionMap, baseTc).r;
    #elif _OCC == 2
        shading.occlusion = albedo.a;
    #elif _OCC == 3
        shading.occlusion = shading.specular.a;
    #endif

    shading.roughness = 1.0 - glossiness;

    shading.specularPower = GlossinessToSpecularPower(glossiness);
#endif

    // Clamp the roughness to a minimum value to avoid divisions by 0 in the lighting code
    shading.roughness = clamp(shading.roughness, MIN_ROUGHNESS, 1.0);

    // We adopted Disney's reparameterization of linearRoughness = roughness^2
    shading.linearRoughness = shading.roughness * shading.roughness;

#if defined(STANDARD_METALLIC_LIGHTING) || defined(STANDARD_SPECULAR_LIGHTING)
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
            vec3 tangentClearCoatN = normalize(GetNormal(clearCoatNormalMap, baseTc));

            // Convert coordinates from tangent space to GL world space
            shading.clearCoatN = shading.tagentToWorldMatrix * tangentClearCoatN;
        #else
            shading.clearCoatN = shading.n;
        #endif
    #endif
#endif

#if defined(INDIRECT_LIGHTING)
    #if _EMISSION == 1
        shading.emission = emissionColor * emissionScale;
    #elif _EMISSION == 2
        shading.emission = tex2D(emissionMap, baseTc).rgb * emissionScale;
    #endif
#endif
}
#endif

void main() {
#ifdef NEED_BASE_TC
    #if _PARALLAX != 0
        float h = tex2D(heightMap, v2f_tex).x * 2.0 - 1.0;
        baseTc = v2f_tex + h * heightScale * 0.1 * (v2f_tangentViewDir.xy / v2f_tangentViewDir.z);
    #else
        baseTc = v2f_tex;
    #endif
#endif

#if _ALBEDO == 0
    vec4 albedo = vec4(albedoColor, albedoAlpha);
#elif _ALBEDO == 1
    vec4 albedo = tex2D(albedoMap, baseTc);
#endif

#ifdef PERFORATED
    if (albedo.a < perforatedAlpha) {
        discard;
    }
#endif

#if defined(DIRECT_LIGHTING) || defined(INDIRECT_LIGHTING)
    PrepareShadingParms(albedo);
#endif

    vec3 shadingColor = vec3(0.0);

#ifdef INDIRECT_LIGHTING
    #ifdef BRUTE_FORCE_IBL
        #if defined(STANDARD_METALLIC_LIGHTING) || defined(STANDARD_SPECULAR_LIGHTING)
            shadingColor += IBLDiffuseLambertWithSpecularGGX(envCubeMap);
        #elif defined(LEGACY_PHONG_LIGHTING)
            shadingColor += IBLPhongWithFresnel(envCubeMap);
        #endif
    #else
        vec3 worldS = reflect(-shading.v, shading.n);

        #ifdef PARALLAX_CORRECTED_INDIRECT_LIGHTING
            #if _NORMAL != 0 || _ANISO != 0 || (_CLEARCOAT != 0 && _CC_NORMAL == 1)
                vec3 worldPos;
                worldPos.x = v2f_tangentToWorldAndPackedWorldPosS.w;
                worldPos.y = v2f_tangentToWorldAndPackedWorldPosT.w;
                worldPos.z = v2f_tangentToWorldAndPackedWorldPosR.w;
            #else
                worldPos = v2f_worldPos.yzx;
            #endif

            vec4 sampleVec;
            sampleVec.xyz = BoxProjectedCubemapDirection(worldS, worldPos, vec4(0, 50, 250, 1.0), vec3(-2500, 0, -2500), vec3(2500, 250, 2500)); // FIXME
        #else
            vec4 sampleVec;
            sampleVec.xyz = worldS;
        #endif

        #if defined(STANDARD_METALLIC_LIGHTING) || defined(STANDARD_SPECULAR_LIGHTING)
            shadingColor += IndirectLit_Standard(sampleVec.xyz);
        #elif defined(LEGACY_PHONG_LIGHTING)
            shadingColor += IndirectLit_PhongFresnel(sampleVec.xyz);
        #endif
    #endif

    #if _OCC != 0
        shadingColor *= (1.0 - occlusionStrength) + shading.occlusion * occlusionStrength;
    #endif

    #if _EMISSION != 0
        shadingColor += shading.emission;
    #endif
#else
    shadingColor += albedo.rgb * ambientScale;
#endif

#ifdef DIRECT_LIGHTING
    float attenuation = 1.0 - min(dot(v2f_lightFallOff, v2f_lightFallOff), 1.0);
    attenuation = pow(attenuation, lightFallOffExponent);

    vec3 Cl = tex2Dproj(lightProjectionMap, v2f_lightProjection).xyz * lightColor.xyz * attenuation;

    #ifdef USE_SHADOW_MAP
        vec3 shadowLighting = ShadowFunc();
    #else
        vec3 shadowLighting = vec3(1.0);
    #endif

    shading.l = normalize(v2f_lightVector.yzx);

    #ifdef USE_LIGHT_CUBE_MAP
        if (useLightCube) {
            Cl *= texCUBE(lightCubeMap, -shading.l);
        }
    #endif

    #if defined(STANDARD_METALLIC_LIGHTING) || defined(STANDARD_SPECULAR_LIGHTING)
        vec3 lightingColor = DirectLit_Standard();
    #elif defined(LEGACY_PHONG_LIGHTING)
        vec3 lightingColor = DirectLit_PhongFresnel();
    #endif

    #if defined(_SUB_SURFACE_SCATTERING)
        float subLamb = smoothstep(-subSurfaceRollOff, 1.0, NdotL) - smoothstep(0.0, 1.0, NdotL);
        subLamb = max(subLamb, 0.0);
        lightingColor += subLamb * tex2D(subSurfaceColorMap, baseTc).xyz * (shadowLighting * (1.0 - subSurfaceShadowDensity) + subSurfaceShadowDensity);
    #endif

    shadingColor += Cl * lightingColor * shadowLighting;

    #if _OCC != 0
        shadingColor *= (1.0 - occlusionStrength) + shading.occlusion * occlusionStrength;
    #endif
#endif

    vec4 finalColor = v2f_color * vec4(shadingColor, albedo.a);

#ifdef LOGLUV_HDR
    o_fragColor = encodeLogLuv(finalColor.xyz);
#else
    o_fragColor = finalColor;
#endif
}
