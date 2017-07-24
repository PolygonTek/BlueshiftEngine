$include "fragment_common.glsl"
$include "LightingPhong.glsl"
$include "LightingStandard.glsl"

#ifdef USE_SHADOW_MAP
$include "shadow.fp"
#endif

in vec4 v2f_color;
in vec2 v2f_tcDiffuseNormal;
in vec2 v2f_tcSpecular;
//in vec2 v2f_tcLightmap;
in vec3 v2f_viewVector;
in vec3 v2f_lightVector;
in vec3 v2f_lightFallOff;
in vec4 v2f_lightProjection;
// local vertex normal
in vec3 v2f_normal;
in vec3 v2f_toWorldS;
in vec3 v2f_toWorldT;
in vec3 v2f_toWorldR;

out vec4 o_fragColor : FRAG_COLOR;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D detailNormalMap;
uniform sampler2D specularMap;
uniform sampler2D heightMap;
uniform sampler2D lightProjectionMap;
uniform sampler2D selfIllumMap;
uniform samplerCube lightCubeMap;

uniform vec4 diffuseColor;
uniform vec4 specularColor;
uniform vec4 lightColor;
uniform float detailRepeat;
uniform float lightFallOffExponent;
uniform float ambientScale;
uniform bool useLightCube;
uniform bool useShadowMap;
uniform float glossiness;
uniform float roughness;
uniform float metalness;
uniform float rimLightExponent;
uniform float rimLightShadowDensity;// = 0.5;
uniform float subSurfaceRollOff;
uniform float subSurfaceShadowDensity;// = 0.5;
uniform sampler2D subSurfaceColorMap;

uniform float ambientLerp;
uniform samplerCube diffuseIrradianceCubeMap0;
uniform samplerCube diffuseIrradianceCubeMap1;
uniform samplerCube specularIrradianceCubeMap0;
uniform samplerCube specularIrradianceCubeMap1;

void main() {
#if DIRECT_LIGHTING
    float A = 1.0 - min(dot(v2f_lightFallOff, v2f_lightFallOff), 1.0);
    A = pow(A, lightFallOffExponent);

    vec3 Cl = tex2Dproj(lightProjectionMap, v2f_lightProjection).xyz * lightColor.xyz * A;
    
    if (Cl == vec3(0.0)) {
        discard;
    }
#endif

#if AMBIENT_LIGHTING || DIRECT_LIGHTING || _PARALLAX
	vec3 V = normalize(v2f_viewVector);
#endif

#if _ALBEDO_SOURCE != 0 || _NORMAL_SOURCE != 0
    #if _PARALLAX
	    vec2 baseTc = offsetTexcoord(heightMap, v2f_tcDiffuseNormal, V.xy, 0.0078125);
    #else
	    vec2 baseTc = v2f_tcDiffuseNormal;
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
        vec4 Ks = vec4(0.0, 0.0, 0.0, 0.0);
    #elif _SPECULAR_SOURCE == 1
        vec4 Ks = specularColor;
    #elif (_SPECULAR_SOURCE == 2 || _SPECULAR_SOURCE == 3)
        vec4 Ks = tex2D(specularMap, v2f_tcSpecular);
    #elif _SPECULAR_SOURCE == 4
        vec4 Ks = vec4(specularColor.rgb, tex2D(specularMap, v2f_tcSpecular).r);
    #endif
#endif

	vec3 C;

#if AMBIENT_LIGHTING
	vec3 tangentToWorldMatrixS = normalize(v2f_toWorldS.xyz);
	vec3 tangentToWorldMatrixT = normalize(v2f_toWorldT.xyz);
	vec3 tangentToWorldMatrixR = normalize(v2f_toWorldR.xyz);
	//vec3 tangentToWorldMatrixR = normalize(cross(tangentToWorldMatrixS, tangentToWorldMatrixT) * v2f_toWorldT.w);

	vec3 worldN;
    // Convert coordinates from z-up to GL axis
	worldN.z = dot(tangentToWorldMatrixS, N);
	worldN.x = dot(tangentToWorldMatrixT, N);
	worldN.y = dot(tangentToWorldMatrixR, N);

	vec3 d1 = texCUBE(diffuseIrradianceCubeMap0, worldN).rgb;
	vec3 d2 = texCUBE(diffuseIrradianceCubeMap1, worldN).rgb;

	vec3 Cd = Kd.rgb * mix(d1, d2, ambientLerp);

    #if _SPECULAR_SOURCE != 0
        #if _SPECULAR_SOURCE == 3 || _SPECULAR_SOURCE == 4
            float Kg = Ks.a;
        #else
            float Kg = glossiness;
        #endif

        float specularPower = glossinessToSpecularPower(Kg);

        // (log2(specularPower) - log2(maxSpecularPower)) / log2(pow(maxSpecularPower, -1/numMipmaps))
        // (log2(specularPower) - 11) / (-11/8)
        float specularMipLevel = -(8.0 / 11.0) * log2(specularPower) + 8.0;

        vec3 S = reflect(-V, N);
	    vec4 worldS;
        // Convert coordinates from z-up to GL axis
	    worldS.z = dot(tangentToWorldMatrixS, S);
	    worldS.x = dot(tangentToWorldMatrixT, S);
	    worldS.y = dot(tangentToWorldMatrixR, S);
        worldS.w = specularMipLevel;

	    vec3 s1 = texCUBElod(specularIrradianceCubeMap0, worldS).rgb;
	    vec3 s2 = texCUBElod(specularIrradianceCubeMap1, worldS).rgb;

        float NdotV = max(dot(N, V), 0.0);

        vec3 Fs = F_SchlickRoughness(Ks.rgb, 1.0 - Kg, NdotV); 
        vec3 Cs = Fs * s1;

        C = Cd * (vec3(1.0) - Fs) + Cs;
    #else
        C = Cd;
    #endif
#else
    C = Kd.rgb;
#endif

    C *= ambientScale;

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

    vec3 lightingColor = litBlinnPhongEC(L, N, V, Kd, Ks, glossiness);
    //vec3 lightingColor = litStandard(L, N, V, Kd.rgb, roughness, metalness);

    #if defined(_SUB_SURFACE_SCATTERING)
        float subLamb = smoothstep(-subSurfaceRollOff, 1.0, NdotL) - smoothstep(0.0, 1.0, NdotL);
        subLamb = max(subLamb, 0.0);
        lightingColor += subLamb * tex2D(subSurfaceColorMap, baseTc).xyz * (shadowLighting * (1.0 - subSurfaceShadowDensity) + subSurfaceShadowDensity);
    #endif

    C += Cl * lightingColor * shadowLighting;
#endif

#ifdef SELF_ILLUM
	C += tex2D(selfIllumMap, baseTc).rgb;
#endif

	vec4 outputColor = v2f_color * vec4(C, Kd.w);

#ifdef LOGLUV_HDR
	o_fragColor = encodeLogLuv(outputColor.xyz);
#else
	o_fragColor = outputColor;
#endif
}
