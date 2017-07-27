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

uniform sampler2D diffuseMap;
uniform vec4 diffuseColor;

uniform sampler2D normalMap;
uniform sampler2D detailNormalMap;
uniform float detailRepeat;

uniform sampler2D specularMap;
uniform vec4 specularColor;

uniform sampler2D glossMap;
uniform float glossiness;

uniform float roughness;
uniform float metalness;

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
uniform samplerCube specularIrradianceCubeMap0;
uniform samplerCube specularIrradianceCubeMap1;
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
    vec4 Kd = diffuseColor;
#elif _ALBEDO_SOURCE == 1
    vec4 Kd = tex2D(diffuseMap, baseTc);
#endif

#ifdef PERFORATED
    if (Kd.w < 0.5) {
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
        vec4 Ks = vec4(0.0);
    #elif _SPECULAR_SOURCE == 1
        vec4 Ks = specularColor;
    #elif _SPECULAR_SOURCE == 2
        vec4 Ks = tex2D(specularMap, baseTc);
    #endif

    #if _GLOSS_SOURCE == 0
        float Kg = glossiness;
    #elif _GLOSS_SOURCE == 1
        float Kg = Kd.a * glossiness;
    #elif _GLOSS_SOURCE == 2
        float Kg = Ks.a * glossiness;
    #elif _GLOSS_SOURCE == 3
        float Kg = tex2D(glossMap, baseTc).r * glossiness;
    #endif
#endif

    vec3 C = vec3(0.0);

#if AMBIENT_LIGHTING || DIRECT_LIGHTING
    float specularPower = glossinessToSpecularPower(Kg);
#endif

#if AMBIENT_LIGHTING
    #if _EMISSION_SOURCE == 1
        C += emissionColor * emissionScale;
    #elif _EMISSION_SOURCE == 2
        C += tex2D(emissionMap, baseTc).rgb * emissionColor * emissionScale;
    #endif

    #if IBL
        vec3 tangentToWorldMatrixS = normalize(v2f_toWorldAndPackedWorldPosS.xyz);
        vec3 tangentToWorldMatrixT = normalize(v2f_toWorldAndPackedWorldPosT.xyz);
        vec3 tangentToWorldMatrixR = normalize(v2f_toWorldAndPackedWorldPosR.xyz);
        //vec3 tangentToWorldMatrixR = normalize(cross(tangentToWorldMatrixS, tangentToWorldMatrixT) * v2f_toWorldT.w);

        vec3 worldN;
        // Convert coordinates from z-up to GL axis
        worldN.z = dot(tangentToWorldMatrixS, N);
        worldN.x = dot(tangentToWorldMatrixT, N);
        worldN.y = dot(tangentToWorldMatrixR, N);

        vec3 worldV;
        // Convert coordinates from z-up to GL axis
        worldV.z = dot(tangentToWorldMatrixS, V);
        worldV.x = dot(tangentToWorldMatrixT, V);
        worldV.y = dot(tangentToWorldMatrixR, V);

        C += IBLPhongWithFresnel(envCubeMap, worldN, worldV, Kd.rgb, Ks.rgb, Kg);
    #else
        vec3 tangentToWorldMatrixS = normalize(v2f_toWorldAndPackedWorldPosS.xyz);
        vec3 tangentToWorldMatrixT = normalize(v2f_toWorldAndPackedWorldPosT.xyz);
        vec3 tangentToWorldMatrixR = normalize(v2f_toWorldAndPackedWorldPosR.xyz);
        //vec3 tangentToWorldMatrixR = normalize(cross(tangentToWorldMatrixS, tangentToWorldMatrixT) * v2f_toWorldT.w);

        vec3 worldN;
        // Convert coordinates from z-up to GL axis
        worldN.z = dot(tangentToWorldMatrixS, N);
        worldN.x = dot(tangentToWorldMatrixT, N);
        worldN.y = dot(tangentToWorldMatrixR, N);

        vec3 d1 = texCUBE(diffuseIrradianceCubeMap0, worldN).rgb;
        vec3 d2 = texCUBE(diffuseIrradianceCubeMap1, worldN).rgb;

        vec3 Cd = Kd.rgb * mix(d1, d2, ambientLerp);    

        vec3 S = reflect(-V, N);
        vec3 worldS;
        // Convert coordinates from z-up to GL axis
        worldS.z = dot(tangentToWorldMatrixS, S);
        worldS.x = dot(tangentToWorldMatrixT, S);
        worldS.y = dot(tangentToWorldMatrixR, S);

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

        // (log2(specularPower) - log2(maxSpecularPower)) / log2(pow(maxSpecularPower, -1/numMipmaps))
        // (log2(specularPower) - 11) / (-11/8)
        float specularMipLevel = -(8.0 / 11.0) * log2(specularPower) + 8.0;

        sampleVec.w = specularMipLevel;

        vec3 s1 = texCUBElod(specularIrradianceCubeMap0, sampleVec).rgb;
        vec3 s2 = texCUBElod(specularIrradianceCubeMap1, sampleVec).rgb;

        float NdotV = max(dot(N, V), 0.0);

        vec3 F = F_SchlickRoughness(Ks.rgb, sqrt(1.0 - Kg), NdotV);
        vec3 Cs = F * s1;

        C += Cd * (vec3(1.0) - F) + Cs;
    #endif
#else
    C += Kd.rgb * ambientScale;
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

    vec3 lightingColor = litPhongEC(L, N, V, Kd.rgb, Ks.rgb, specularPower);    
    //vec3 lightingColor = litStandard(L, N, V, Kd.rgb, roughness, metalness);

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

    vec4 outputColor = v2f_color * vec4(C, Kd.w);

#ifdef LOGLUV_HDR
    o_fragColor = encodeLogLuv(outputColor.xyz);
#else
    o_fragColor = outputColor;
#endif
}
