$include "fragment_common.glsl"

in vec4 v2f_color;
in vec2 v2f_texCoord;
in vec3 v2f_viewVector;
in vec3 v2f_tangentToWorldMatrixS;
in vec3 v2f_tangentToWorldMatrixT;
in vec3 v2f_tangentToWorldMatrixR;
in vec3 v2f_normal;

out vec4 o_fragColor : FRAG_COLOR;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D detailNormalMap;
uniform samplerCube envCubeMap;
uniform sampler2D envMaskMap;
uniform sampler2D heightMap;
uniform sampler2D selfIllumMap;
uniform samplerCube diffuseIrradianceCubeMap0;
uniform samplerCube diffuseIrradianceCubeMap1;

uniform vec4 diffuseColor;
uniform float ambientLerp;
uniform float detailRepeat;
uniform float fresnelConstant;// = 0.144;
uniform float reflectness;// = 1.0;

void main() {
#ifdef _PARALLAX
	vec2 tc = offsetTexcoord(heightMap, v2f_texCoord, normalize(v2f_viewVector).xy, 0.0078125);
#else
	vec2 tc = v2f_texCoord;
#endif

#if _ALBEDO_SOURCE == 0
	vec4 diffuse = diffuseColor;
#elif _ALBEDO_SOURCE == 1
	vec4 diffuse = tex2D(diffuseMap, tc);
#endif

#ifdef PERFORATED
	if (diffuse.w < 0.5) {
		discard;
	}
#endif

#if !defined(NO_LIT)
	#if _NORMAL_SOURCE != 0
		#if _NORMAL_SOURCE == 2
			vec3 b1 = normalize(getNormal(normalMap, tc));
			vec3 b2 = vec3(tex2D(detailNormalMap, tc * detailRepeat).xy * 2.0 - 1.0, 0.0);
			vec3 N = normalize(b1 + b2);
		#else
			vec3 N = normalize(getNormal(normalMap, tc));
		#endif

		vec3 tangentToWorldMatrixS = normalize(v2f_tangentToWorldMatrixS);
		vec3 tangentToWorldMatrixT = normalize(v2f_tangentToWorldMatrixT);
		vec3 tangentToWorldMatrixR = normalize(v2f_tangentToWorldMatrixR);

		vec3 worldN;
		worldN.x = dot(tangentToWorldMatrixS, N);
		worldN.y = dot(tangentToWorldMatrixT, N);
		worldN.z = dot(tangentToWorldMatrixR, N);
	#else
		vec3 worldN = normalize(v2f_normal);
	#endif

	vec3 a1 = texCUBE(diffuseIrradianceCubeMap0, worldN).xyz;
	vec3 a2 = texCUBE(diffuseIrradianceCubeMap1, worldN).xyz;

	vec3 ambient = mix(a1, a2, ambientLerp);
#else
	vec3 ambient = vec3(1.0, 1.0, 1.0);
#endif

#ifdef SELF_ILLUM
	ambient += tex2D(selfIllumMap, tc);
#endif

	ambient *= diffuse.xyz;

	vec4 outputColor = v2f_color * vec4(ambient, diffuse.w);

#ifdef LOGLUV_HDR
	o_fragColor = encodeLogLuv(outputColor.xyz);
#else
	o_fragColor = outputColor;
#endif
}
