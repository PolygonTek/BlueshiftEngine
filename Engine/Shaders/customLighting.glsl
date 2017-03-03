uniform float specularExponent;
const float specularExponentMapScale = 64.0;

void customLighting(in vec3 N, in vec3 L, in vec3 V, in vec4 S, out vec3 Cd, out vec3 Cs) {
#if !defined(_HALF_LAMBERT_DIFFUSE)
	float NdotL = dot(N, L);
	Cd = vec3(1.0, 1.0, 1.0) * max(NdotL, 0.0);
#else
	float NdotL = dot(N, L);
	float halfLambert = NdotL * 0.5 + 0.5;
	halfLambert *= halfLambert;
	Cd = vec3(halfLambert, halfLambert, halfLambert);
#endif

#if _SPECULAR_SOURCE == 0
	Cs = vec3(0.0, 0.0, 0.0);
#else
	vec3 R = reflect(-L, N);
	float RdotV = max(dot(R, V), 0.0);

	#if _SPECULAR_SOURCE == 3
		Cs = S.xyz * pow((NdotL > 0.0 ? RdotV : 0.0), max(S.a * specularExponentMapScale, 1.0));
	#else
		Cs = S.xyz * pow((NdotL > 0.0 ? RdotV : 0.0), specularExponent);
	#endif
#endif
}
