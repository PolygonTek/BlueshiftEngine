$include "FragmentCommon.glsl"
$include "StandardConfig.glsl"

out vec4 o_fragColor : FRAG_COLOR;

#if _ALBEDO != 0 || _NORMAL != 0 || _SPECULAR != 0 || _GLOSS == 3 || _METALLIC >= 1 || (_ROUGHNESS == 1 || _ROUGHNESS == 2) || _PARALLAX != 0 || _EMISSION == 2 || _CLEARCOAT == 2 || (_CLEARCOAT != 0 && _CC_NORMAL == 1) || _ANISO == 2 || _OCC == 1
    #define NEED_BASE_TC
#endif

#ifdef NEED_BASE_TC
    vec2 baseTc;
#endif

in VS_OUT {
    LOWP vec4 color;

#ifdef NEED_BASE_TC
    MEDIUMP vec2 tex;
#endif

#if _NORMAL == 0
    LOWP vec3 normalWS;
#endif

#if _PARALLAX
    vec3 viewTS;
#endif

#ifdef DIRECT_LIGHTING
    vec3 lightVector;
    vec3 lightFallOff;
    vec4 lightProjection;
#endif

#if defined(DIRECT_LIGHTING) || defined(INDIRECT_LIGHTING)
    vec3 viewWS;

    #if _NORMAL != 0 || _ANISO != 0 || (_CLEARCOAT != 0 && _CC_NORMAL == 1)
        vec4 tangentToWorldAndPackedWorldPosS;
        vec4 tangentToWorldAndPackedWorldPosT;
        vec4 tangentToWorldAndPackedWorldPosR;
    #else
        vec3 positionWS;
    #endif
#endif
} fs_in;

#ifdef USE_SHADOW_MAP
    $include "ShadowLibrary.fp"
#endif

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
    uniform samplerCube probe0DiffuseCubeMap;
    uniform samplerCube probe0SpecularCubeMap;
    uniform float probe0SpecularCubeMapMaxMipLevel;
    uniform vec4 probe0Position;
    uniform vec3 probe0Mins;
    uniform vec3 probe0Maxs;

    uniform samplerCube probe1DiffuseCubeMap;
    uniform samplerCube probe1SpecularCubeMap;
    uniform float probe1SpecularCubeMapMaxMipLevel;
    uniform vec4 probe1Position;
    uniform vec3 probe1Mins;
    uniform vec3 probe1Maxs;

    uniform sampler2D prefilteredDfgMap;
    uniform LOWP float probeLerp;
#endif

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

#if _PARALLAX != 0
    $include "ParallaxMapping.glsl"
#endif

void main() {
#ifdef NEED_BASE_TC
    #if _PARALLAX != 0
        baseTc = ParallaxMapping(heightMap, fs_in.tex, heightScale, normalize(fs_in.viewTS));
    #else
        baseTc = fs_in.tex;
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

        #ifdef SPECULAR_PROBE_BOX_PROJECTION
            #if _NORMAL != 0 || _ANISO != 0 || (_CLEARCOAT != 0 && _CC_NORMAL == 1)
                vec3 worldPos;
                worldPos.x = fs_in.tangentToWorldAndPackedWorldPosS.w;
                worldPos.y = fs_in.tangentToWorldAndPackedWorldPosT.w;
                worldPos.z = fs_in.tangentToWorldAndPackedWorldPosR.w;
            #else
                vec3 worldPos = fs_in.positionWS.xyz;
            #endif

            shading.s0 = BoxProjectedCubemapDirection(worldS, worldPos, probe0Position, probe0Mins, probe0Maxs);

            #ifdef PROBE_BLENDING
                shading.s1 = BoxProjectedCubemapDirection(worldS, worldPos, probe1Position, probe1Mins, probe1Maxs);
            #endif
        #else
            shading.s0 = worldS;

            #ifdef PROBE_BLENDING
                shading.s1 = worldS;
            #endif
        #endif

        #if defined(STANDARD_METALLIC_LIGHTING) || defined(STANDARD_SPECULAR_LIGHTING)
            shadingColor += IndirectLit_Standard();
        #elif defined(LEGACY_PHONG_LIGHTING)
            shadingColor += IndirectLit_PhongFresnel();
        #endif
    #endif

    #if _EMISSION != 0
        shadingColor += shading.emission;
    #endif
#else
    shadingColor += albedo.rgb * ambientScale;
#endif

#ifdef DIRECT_LIGHTING
    float attenuation = 1.0 - min(dot(fs_in.lightFallOff, fs_in.lightFallOff), 1.0);
    attenuation = pow(attenuation, lightFallOffExponent);

    vec3 Cl = tex2Dproj(lightProjectionMap, fs_in.lightProjection).rgb * lightColor.rgb * attenuation;

    #ifdef USE_SHADOW_MAP
        vec3 shadowLighting = ShadowFunc();
    #else
        vec3 shadowLighting = vec3(1.0);
    #endif

    shading.l = normalize(fs_in.lightVector.xyz);

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
#endif

    vec4 finalColor = fs_in.color * vec4(shadingColor, albedo.a);

#ifdef LOGLUV_HDR
    o_fragColor = encodeLogLuv(finalColor.xyz);
#else
    o_fragColor = finalColor;
#endif
}
