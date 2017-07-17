$include "fragment_common.glsl"

in vec4 v2f_color;
in vec2 v2f_tcDiffuseBump;
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
uniform sampler2D normalMap;
uniform sampler2D detailNormalMap;
uniform sampler2D heightMap;
uniform sampler2D selfIllumMap;

uniform vec4 diffuseColor;
uniform vec4 lightColor;
uniform float detailRepeat;
uniform bool useShadowMap;

uniform float ambientLerp;
uniform samplerCube ambientCubeMap0;
uniform samplerCube ambientCubeMap1;

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
#if defined(_PARALLAX) || defined(_RIM_LIGHT)
	vec3 V = normalize(v2f_viewVector);
#endif

#ifdef _PARALLAX
	vec2 tc = offsetTexcoord(heightMap, v2f_tcDiffuseBump, V.xy, 0.0078125);
#else
	vec2 tc = v2f_tcDiffuseBump;
#endif

#if _DIFFUSE_SOURCE == 0
	vec4 diffuse = diffuseColor;
#elif _DIFFUSE_SOURCE == 1
	vec4 diffuse = tex2D(diffuseMap, tc);
#endif

#ifdef PERFORATED
	if (diffuse.w < 0.5) {
		discard;
	}
#endif

	vec3 ambient = vec3(1.0, 1.0, 1.0);

#if _NORMAL_SOURCE == 0
	vec3 N = normalize(v2f_normal);
#elif _NORMAL_SOURCE == 1
	vec3 N = normalize(getNormal(normalMap, tc));
#elif _NORMAL_SOURCE == 2
	vec3 b1 = normalize(getNormal(normalMap, tc));
	vec3 b2 = vec3(tex2D(detailNormalMap, tc * detailRepeat).xy * 2.0 - 1.0, 0.0);
	vec3 N = normalize(b1 + b2);
#endif

#ifndef NO_AMBIENT_CUBE_MAP
	vec3 tangentToWorldMatrixS = normalize(v2f_tangentToWorldMatrixS.xyz);
	vec3 tangentToWorldMatrixT = normalize(v2f_tangentToWorldMatrixT.xyz);
	vec3 tangentToWorldMatrixR = normalize(v2f_tangentToWorldMatrixR.xyz);
	//vec3 tangentToWorldMatrixR = normalize(cross(tangentToWorldMatrixS, tangentToWorldMatrixT) * v2f_tangentToWorldMatrixT.w);

	vec3 worldN;
    // Convert coordinates from z-up to GL axis
	worldN.z = dot(tangentToWorldMatrixS, N);
	worldN.x = dot(tangentToWorldMatrixT, N);
	worldN.y = dot(tangentToWorldMatrixR, N);

	vec3 a1 = texCUBE(ambientCubeMap0, worldN).xyz;
	vec3 a2 = texCUBE(ambientCubeMap1, worldN).xyz;

	ambient = mix(a1, a2, ambientLerp);
#endif

	ambient *= diffuse.xyz;

#ifdef SELF_ILLUM
	ambient += tex2D(selfIllumMap, tc);
#endif

	vec4 outputColor = v2f_color * vec4(ambient, diffuse.w);

#ifdef LOGLUV_HDR
	o_fragColor = encodeLogLuv(outputColor.xyz);
#else
	o_fragColor = outputColor;
#endif
}
