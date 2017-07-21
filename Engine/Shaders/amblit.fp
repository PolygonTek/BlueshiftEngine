$include "fragment_common.glsl"
$include "LightingPhong.glsl"
$include "LightingStandard.glsl"

in vec4 v2f_color;
in vec2 v2f_tcDiffuseBump;
in vec2 v2f_tcSpecular;
in vec2 v2f_tcLightmap;
in vec3 v2f_viewVector;
in vec3 v2f_lightVector;
in vec4 v2f_lightProjection;
in vec3 v2f_normal;
in vec3 v2f_tangentToWorldMatrixS;
in vec3 v2f_tangentToWorldMatrixT;
in vec3 v2f_tangentToWorldMatrixR;

out vec4 o_fragColor : FRAG_COLOR;

uniform sampler2D diffuseMap;
uniform sampler2D specularMap;
uniform sampler2D normalMap;
uniform sampler2D detailNormalMap;
uniform sampler2D heightMap;
uniform sampler2D selfIllumMap;

uniform vec4 diffuseColor;
uniform vec4 specularColor;
uniform vec4 lightColor;
uniform float detailRepeat;
uniform bool useShadowMap;

uniform float ambientLerp;
uniform samplerCube diffuseIrradianceCubeMap0;
uniform samplerCube diffuseIrradianceCubeMap1;
uniform samplerCube specularIrradianceCubeMap0;
uniform samplerCube specularIrradianceCubeMap1;

uniform float fresnelConstant;// = 0.144;
uniform float reflectness;// = 1.0;
uniform samplerCube envCubeMap;
uniform sampler2D envMaskMap;

uniform float rimLightExponent;
uniform float rimLightShadowDensity;// = 0.5;
uniform float subSurfaceRollOff;
uniform float subSurfaceShadowDensity;// = 0.5;
uniform sampler2D subSurfaceColorMap;

void main() {
#if defined(_PARALLAX) || defined(_RIM_LIGHT) || !defined(NO_AMBIENT_CUBE_MAP)
	vec3 V = normalize(v2f_viewVector);
#endif

#ifdef _PARALLAX
	vec2 tc = offsetTexcoord(heightMap, v2f_tcDiffuseBump, V.xy, 0.0078125);
#else
	vec2 tc = v2f_tcDiffuseBump;
#endif

#if _ALBEDO_SOURCE == 0
	vec4 Kd = diffuseColor;
#elif _ALBEDO_SOURCE == 1
	vec4 Kd = tex2D(diffuseMap, tc);
#endif

#ifdef PERFORATED
	if (Kd.w < 0.5) {
		discard;
	}
#endif

#if _NORMAL_SOURCE == 0
	vec3 N = normalize(v2f_normal);
#elif _NORMAL_SOURCE == 1
	vec3 N = normalize(getNormal(normalMap, tc));
#elif _NORMAL_SOURCE == 2
	vec3 b1 = normalize(getNormal(normalMap, tc));
	vec3 b2 = vec3(tex2D(detailNormalMap, tc * detailRepeat).xy * 2.0 - 1.0, 0.0);
	vec3 N = normalize(b1 + b2);
#endif

#if _SPECULAR_SOURCE == 0
    vec4 Ks = vec4(0.0, 0.0, 0.0, 0.0);
#elif _SPECULAR_SOURCE == 1
    vec4 Ks = specularColor;
#elif (_SPECULAR_SOURCE == 2 || _SPECULAR_SOURCE == 3)
    vec4 Ks = tex2D(specularMap, v2f_tcSpecular);
#elif _SPECULAR_SOURCE == 4
    vec4 Ks = tex2D(diffuseMap, v2f_tcSpecular).aaaa;
#endif

	vec3 ambient;

#ifdef NO_AMBIENT_CUBE_MAP
    ambient = Kd.rgb;
#else
	vec3 tangentToWorldMatrixS = normalize(v2f_tangentToWorldMatrixS.xyz);
	vec3 tangentToWorldMatrixT = normalize(v2f_tangentToWorldMatrixT.xyz);
	vec3 tangentToWorldMatrixR = normalize(v2f_tangentToWorldMatrixR.xyz);
	//vec3 tangentToWorldMatrixR = normalize(cross(tangentToWorldMatrixS, tangentToWorldMatrixT) * v2f_tangentToWorldMatrixT.w);

	vec3 worldN;
    // Convert coordinates from z-up to GL axis
	worldN.z = dot(tangentToWorldMatrixS, N);
	worldN.x = dot(tangentToWorldMatrixT, N);
	worldN.y = dot(tangentToWorldMatrixR, N);

	vec3 d1 = texCUBE(diffuseIrradianceCubeMap0, worldN).rgb;
	vec3 d2 = texCUBE(diffuseIrradianceCubeMap1, worldN).rgb;

	vec3 Cd = Kd.rgb * (vec3(1.0) - Ks.rgb) * mix(d1, d2, ambientLerp);

    #if _SPECULAR_SOURCE != 0
        #if _SPECULAR_SOURCE == 3
            float specularPower = glossinessToSpecularPower(Ks.a);
        #else
            float specularPower = glossinessToSpecularPower(glossiness);
        #endif
        float specularMipLevel = -(7.0 / 11.0) * log2(specularPower) + 7.0;

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

        vec3 F = F_SchlickSG(Ks.rgb, NdotV); 
        vec3 Cs = F * s1;

        ambient = Cd + Cs;
    #else
        ambient = Cd;
    #endif
#endif

#ifdef SELF_ILLUM
	ambient += tex2D(selfIllumMap, tc);
#endif

	vec4 outputColor = v2f_color * vec4(ambient, Kd.w);

#ifdef LOGLUV_HDR
	o_fragColor = encodeLogLuv(outputColor.xyz);
#else
	o_fragColor = outputColor;
#endif
}
