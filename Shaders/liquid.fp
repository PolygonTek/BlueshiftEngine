$include "fragment_common.glsl"

in vec4 v2f_color;
in vec2 v2f_texCoord;
in vec3 v2f_eyeVector;
in vec2 v2f_distortion;
in vec3 v2f_toWorldS;
in vec3 v2f_toWorldT;
in vec3 v2f_toWorldR;

out vec4 o_fragColor : FRAG_COLOR;

uniform sampler2D normalMap;
uniform sampler2D normalMapNext;
uniform sampler2D albedoMap;
uniform samplerCube envCubeMap;
uniform sampler2D sunSpecularMap;
uniform sampler2DRect currentRenderMap;
uniform sampler2D depthMap;

uniform vec2 invScreenSize;
uniform float lerpFactor;
uniform float fresnelConstant;
uniform float reflectness;
uniform vec3 liquidDistortion;
uniform vec3 liquidTint;
uniform vec3 sunDirection;
uniform float sunSpecularExponent;
uniform vec3 sunColor;

void main() {
#ifdef INTERPOLATE
	vec3 b1 = getNormal(normalMap, v2f_texCoord);
	vec3 b2 = getNormal(normalMapNext, v2f_texCoord);
	vec3 N = mix(b1, b2, lerpFactor);
	//N = smoothstep(b1, b2, N);
	//N = N * b1 + (1.0 - N) * b2;
#else
	vec3 N = getNormal(normalMap, v2f_texCoord);
#endif

	N.xy *= liquidDistortion.z;
	N = normalize(N);

	vec3 E = normalize(v2f_eyeVector.xyz);
	vec3 R = reflect(E, N);
	vec3 worldR;
	worldR.x = dot(v2f_toWorldS, R);
	worldR.y = dot(v2f_toWorldT, R);
	worldR.z = dot(v2f_toWorldR, R);
	worldR = normalize(worldR);

#ifdef SUN
	float RdotS = max(dot(worldR, normalize(sunDirection)), 0.0);
	vec2 spec = vec2(pow(RdotS, sunSpecularExponent));
	vec3 Cs = sunColor * tex2D(sunSpecularMap, spec).rgb;
#else
	vec3 Cs = vec3(0, 0, 0);
#endif

/*	vec2 screenST = gl_FragCoord.xy * vec2(1.0/1024.0, 1.0/768.0);//invScreenSize.xy;
	float screenDepth = tex2D(depthMap, screenST).r;
	float shoreFrac = clamp((gl_FragCoord.z - screenDepth) * 10000000.0, 0.0, 1.0);*/

	vec2 screenST = gl_FragCoord.xy;
	screenST += N.xy * (v2f_distortion.x / v2f_distortion.y) * liquidDistortion.xy * vec2(1024, 768);

	vec3 refraction = texRect(currentRenderMap, screenST).xyz * liquidTint;
	vec3 reflection = texCUBE(envCubeMap, worldR).xyz;
	vec4 diffuseColor = tex2D(albedoMap, v2f_texCoord);

	float RF = pow(fresnel(-E, N, fresnelConstant) * reflectness, 2.2);
	vec3 Cd = mix(refraction, reflection, RF);

	vec4 outputColor = vec4(Cd, RF);//v2f_color * vec4(Cd + Cs, diffuseColor.w);
#ifdef LOGLUV_HDR
	o_fragColor = encodeLogLuv(outputColor.xyz);
#else
	o_fragColor = outputColor;
#endif
}
