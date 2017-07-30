$include "fragment_common.glsl"
$include "Lighting.glsl"
$include "IBL.glsl"

#ifdef USE_SHADOW_MAP
$include "shadow.fp"
#endif

in vec4 v2f_color;
in vec2 v2f_tex;

#if _NORMAL_SOURCE == 0
    in vec3 v2f_normal;
#endif

#if DIRECT_LIGHTING
    in vec3 v2f_lightVector;
    in vec3 v2f_lightFallOff;
    in vec4 v2f_lightProjection;
#endif

#if AMBIENT_LIGHTING
    in vec4 v2f_toWorldAndPackedWorldPosS;
    in vec4 v2f_toWorldAndPackedWorldPosT;
    in vec4 v2f_toWorldAndPackedWorldPosR;
#endif

#if AMBIENT_LIGHTING || DIRECT_LIGHTING || _PARALLAX_SOURCE == 1
    in vec3 v2f_viewVector;
#endif

out vec4 o_fragColor : FRAG_COLOR;

uniform sampler2D albedoMap;
uniform vec4 albedoColor;
uniform float perforatedAlpha;

uniform sampler2D normalMap;
uniform sampler2D detailNormalMap;
uniform float detailRepeat;

uniform sampler2D specularMap;
uniform vec4 specularColor;

uniform sampler2D glossMap;
uniform float glossScale;

uniform sampler2D metallicMap;
uniform float metallicScale;

uniform sampler2D roughnessMap;
uniform float roughnessScale;

uniform sampler2D heightMap;
uniform float heightScale;

uniform sampler2D occlusionMap;
uniform float occlusionScale;

uniform sampler2D emissionMap;
uniform vec3 emissionColor;
uniform float emissionScale;

uniform float rimLightExponent;
uniform float rimLightShadowDensity;// = 0.5;

uniform sampler2D subSurfaceColorMap;
uniform float subSurfaceRollOff;
uniform float subSurfaceShadowDensity;// = 0.5;

uniform sampler2D lightProjectionMap;
uniform vec4 lightColor;
uniform float lightFallOffExponent;
uniform samplerCube lightCubeMap;
uniform bool useLightCube;
uniform bool useShadowMap;

uniform samplerCube envCubeMap;
uniform samplerCube diffuseIrradianceCubeMap0;
uniform samplerCube diffuseIrradianceCubeMap1;
uniform samplerCube specularPrefilteredCubeMap0;
uniform samplerCube specularPrefilteredCubeMap1;
uniform sampler2D integrationLUTMap;
uniform float ambientLerp;
uniform float ambientScale;

void main() {
#if DIRECT_LIGHTING
    float A = 1.0 - min(dot(v2f_lightFallOff, v2f_lightFallOff), 1.0);
    A = pow(A, lightFallOffExponent);

    vec3 Cl = tex2Dproj(lightProjectionMap, v2f_lightProjection).xyz * lightColor.xyz * A;
    
    if (Cl == vec3(0.0)) {
        discard;
    }
#endif

#if AMBIENT_LIGHTING || DIRECT_LIGHTING || _PARALLAX_SOURCE == 1
    vec3 V = normalize(v2f_viewVector);
#endif

#if _ALBEDO_SOURCE != 0 || _NORMAL_SOURCE != 0 || _SPECULAR_SOURCE != 0
    #if _PARALLAX_SOURCE == 1
        vec2 baseTc = offsetTexcoord(heightMap, v2f_tex, V, heightScale * 0.1);
    #else
        vec2 baseTc = v2f_tex;
    #endif
#endif

#if _ALBEDO_SOURCE == 0
    vec4 albedo = albedoColor;
#elif _ALBEDO_SOURCE == 1
    vec4 albedo = tex2D(albedoMap, baseTc);
#endif

#ifdef PERFORATED
    if (albedo.w < 0.5) {
        discard;
    }
#endif

#if AMBIENT_LIGHTING || DIRECT_LIGHTING
    #if _NORMAL_SOURCE == 0
        vec3 N = normalize(v2f_normal);
    #elif _NORMAL_SOURCE == 1
        vec3 N = normalize(getNormal(normalMap, baseTc));
    #elif _NORMAL_SOURCE == 2
        vec3 b1 = normalize(getNormal(normalMap, baseTc));
        vec3 b2 = vec3(tex2D(detailNormalMap, baseTc * detailRepeat).xy * 2.0 - 1.0, 0.0);
        vec3 N = normalize(b1 + b2);
    #endif

    #if _SPECULAR_SOURCE == 0
        vec4 specular = vec4(0.0);
    #elif _SPECULAR_SOURCE == 1
        vec4 specular = specularColor;
    #elif _SPECULAR_SOURCE == 2
        vec4 specular = tex2D(specularMap, baseTc);
    #endif

    #if _GLOSS_SOURCE == 0
        float glossiness = glossScale;
    #elif _GLOSS_SOURCE == 1
        float glossiness = albedo.a * glossScale;
    #elif _GLOSS_SOURCE == 2
        float glossiness = specular.a * glossScale;
    #elif _GLOSS_SOURCE == 3
        float glossiness = tex2D(glossMap, baseTc).r * glossScale;
    #endif

    #if _METALLIC_SOURCE == 0
        vec4 metallic = vec4(metallicScale, 0.0, 0.0, 0.0);
    #elif _METALLIC_SOURCE == 1
        vec4 metallic = tex2D(metallicMap, baseTc);
        metallic.r *= metallicScale;
    #endif

    #if _ROUGHNESS_SOURCE == 0
        float roughness = roughnessScale;
    #elif _ROUGHNESS_SOURCE == 1
        float roughness = metallic.g * roughnessScale;
    #elif _ROUGHNESS_SOURCE == 2
        float roughness = tex2D(roughnessMap, baseTc).r * roughnessScale;
    #endif

    #ifdef STANDARD_SPECULAR_LIGHTING
        roughness = 1.0 - glossiness;
    #endif

    #ifdef LEGACY_PHONG_LIGHTING
        float specularPower = glossinessToSpecularPower(glossiness);
    #endif
#endif

    vec3 C = vec3(0.0);

#if (AMBIENT_LIGHTING && DIRECT_LIGHTING) || DIRECT_LIGHTING == 0
    #if _EMISSION_SOURCE == 1
        C += emissionColor * emissionScale;
    #elif _EMISSION_SOURCE == 2
        C += tex2D(emissionMap, baseTc).rgb * emissionColor * emissionScale;
    #endif
#endif

#if AMBIENT_LIGHTING
    vec3 toWorldMatrixS = normalize(v2f_toWorldAndPackedWorldPosS.xyz);
    vec3 toWorldMatrixT = normalize(v2f_toWorldAndPackedWorldPosT.xyz);
    vec3 toWorldMatrixR = normalize(v2f_toWorldAndPackedWorldPosR.xyz);
    //vec3 toWorldMatrixR = normalize(cross(toWorldMatrixS, toWorldMatrixT) * v2f_toWorldT.w);

    #ifdef IBL
        vec3 worldN;
        // Convert coordinates from z-up to GL axis
        worldN.z = dot(toWorldMatrixS, N);
        worldN.x = dot(toWorldMatrixT, N);
        worldN.y = dot(toWorldMatrixR, N);

        vec3 worldV;
        // Convert coordinates from z-up to GL axis
        worldV.z = dot(toWorldMatrixS, V);
        worldV.x = dot(toWorldMatrixT, V);
        worldV.y = dot(toWorldMatrixR, V);

        #ifdef STANDARD_METALLIC_LIGHTING
            C += IBLDiffuseLambertWithSpecularGGX(envCubeMap, worldN, worldV, albedo.rgb * (1.0 - metallic.r), mix(vec3(0.04), albedo.rgb, metallic.r), roughness);
        #elif defined(STANDARD_SPECULAR_LIGHTING)
            C += IBLDiffuseLambertWithSpecularGGX(envCubeMap, worldN, worldV, albedo.rgb, specular.rgb, 1.0 - glossiness);
        #elif defined(LEGACY_PHONG_LIGHTING)
            C += IBLPhongWithFresnel(envCubeMap, worldN, worldV, albedo.rgb, specular.rgb, specularPower, 1.0 - glossiness);
        #endif
    #else
        vec3 worldN;
        // Convert coordinates from z-up to GL axis
        worldN.z = dot(toWorldMatrixS, N);
        worldN.x = dot(toWorldMatrixT, N);
        worldN.y = dot(toWorldMatrixR, N);

        vec3 d1 = texCUBE(diffuseIrradianceCubeMap0, worldN).rgb;
        //vec3 d2 = texCUBE(diffuseIrradianceCubeMap1, worldN).rgb;

        vec3 Cd = albedo.rgb * d1;//mix(d1, d2, ambientLerp);

        vec3 S = reflect(-V, N);
        vec3 worldS;
        // Convert coordinates from z-up to GL axis
        worldS.z = dot(toWorldMatrixS, S);
        worldS.x = dot(toWorldMatrixT, S);
        worldS.y = dot(toWorldMatrixR, S);

        #if PARALLAX_CORRECTED_AMBIENT_LIGHTING
            // Convert coordinates from z-up to GL axis
            vec3 worldPos;
            worldPos.z = v2f_toWorldAndPackedWorldPosS.w;
            worldPos.x = v2f_toWorldAndPackedWorldPosT.w;
            worldPos.y = v2f_toWorldAndPackedWorldPosR.w;

            vec4 sampleVec;
            sampleVec.xyz = boxProjectedCubemapDirection(worldS, worldPos, vec4(0, 50, 0, 1.0), vec3(-4000, -4000, -4000), vec3(4000, 4000, 4000)); // FIXME
        #else
            vec4 sampleVec;
            sampleVec.xyz = worldS;
        #endif

        float NdotV = max(dot(N, V), 0.0);

        #ifdef STANDARD_METALLIC_LIGHTING
            sampleVec.w = roughness * 7.0; // FIXME: 7.0 == maximum mip level

            vec3 s1 = texCUBElod(specularPrefilteredCubeMap0, sampleVec).rgb;
            //vec3 s2 = texCUBElod(specularPrefilteredCubeMap1, sampleVec).rgb;

            vec2 envBRDF = tex2D(integrationLUTMap, vec2(NdotV, roughness)).xy;

            vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic.r);
            vec3 F = F_SchlickRoughness(F0, roughness, NdotV);
            vec3 Cs = s1 * (F * envBRDF.x + envBRDF.y);

            vec3 diffuseScale = (vec3(1.0) - F) * (1.0 - metallic.r);
        #elif defined(STANDARD_SPECULAR_LIGHTING)
            sampleVec.w = roughness * 7.0; // max mip levels = 7

            vec3 s1 = texCUBElod(specularPrefilteredCubeMap0, sampleVec).rgb;
            //vec3 s2 = texCUBElod(specularPrefilteredCubeMap1, sampleVec).rgb;

            vec2 envBRDF = tex2D(integrationLUTMap, vec2(NdotV, roughness)).xy;

            vec3 F = F_SchlickRoughness(specular.rgb, roughness, NdotV);
            vec3 Cs = s1 * (F * envBRDF.x + envBRDF.y);

            vec3 diffuseScale = (vec3(1.0) - F);
        #elif defined(LEGACY_PHONG_LIGHTING)
            // (log2(specularPower) - log2(maxSpecularPower)) / log2(pow(maxSpecularPower, -1/numMipmaps))
            // (log2(specularPower) - 11) / (-11/8)
            float specularMipLevel = -(8.0 / 11.0) * log2(specularPower) + 8.0;

            sampleVec.w = specularMipLevel;

            vec3 s1 = texCUBElod(specularPrefilteredCubeMap0, sampleVec).rgb;
            //vec3 s2 = texCUBElod(specularPrefilteredCubeMap1, sampleVec).rgb;

            vec3 F = F_SchlickRoughness(specular.rgb, sqrt(1.0 - glossiness), NdotV);
            vec3 Cs = F * s1;

            vec3 diffuseScale = (vec3(1.0) - F);
        #endif 

        C += Cd * diffuseScale + Cs;
    #endif
#else
    C += albedo.rgb * ambientScale;
#endif

#if DIRECT_LIGHTING
    #ifdef USE_SHADOW_MAP
        vec3 shadowLighting = ShadowFunc();
    #else
        vec3 shadowLighting = vec3(1.0);
    #endif

    vec3 L = normalize(v2f_lightVector);

    #ifdef USE_LIGHT_CUBE_MAP
        if (useLightCube) {
            Cl *= texCUBE(lightCubeMap, -L);
        }
    #endif

    #ifdef STANDARD_METALLIC_LIGHTING
        vec3 lightingColor = litStandard(L, N, V, albedo.rgb * (1.0 - metallic.r), mix(vec3(0.04), albedo.rgb, metallic.r), roughness);
    #elif defined(STANDARD_SPECULAR_LIGHTING)
        vec3 lightingColor = litStandard(L, N, V, albedo.rgb, specular.rgb, roughness);
    #elif defined(LEGACY_PHONG_LIGHTING)
        vec3 lightingColor = litPhongWithFresnel(L, N, V, albedo.rgb, specular.rgb, specularPower);
    #endif

    #if defined(_SUB_SURFACE_SCATTERING)
        float subLamb = smoothstep(-subSurfaceRollOff, 1.0, NdotL) - smoothstep(0.0, 1.0, NdotL);
        subLamb = max(subLamb, 0.0);
        lightingColor += subLamb * tex2D(subSurfaceColorMap, baseTc).xyz * (shadowLighting * (1.0 - subSurfaceShadowDensity) + subSurfaceShadowDensity);
    #endif

    C += Cl * lightingColor * shadowLighting;
#endif

#if _OCCLUSION_SOURCE == 1
    float occ = tex2D(occlusionMap, baseTc).r;
    C *= 1.0 - occlusionScale + occ * occlusionScale;
#endif

    vec4 outputColor = v2f_color * vec4(C, albedo.w);

#ifdef LOGLUV_HDR
    o_fragColor = encodeLogLuv(outputColor.xyz);
#else
    o_fragColor = outputColor;
#endif
}
