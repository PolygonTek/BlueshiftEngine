#ifndef LIGHTING_PHONG_INCLUDED
#define LIGHTING_PHONG_INCLUDED

uniform float wrapped;

float glossinessToSpecularPower(float glossiness) {
    return exp2(10.0 * glossiness + 1.0); 
}

vec3 litPhong(vec3 L, vec3 N, vec3 V, vec3 albedo, vec3 specular, float specularPower) {
#if defined(_WRAPPED_DIFFUSE)
    float NdotL = dot(N, L);
    float w2 = 1.0 + wrapped;
    vec3 Cd = albedo.rgb * (NdotL + wrapped) / (w2 * w2);
#else // Lambertian
    float NdotL = dot(N, L);
    vec3 Cd = albedo.rgb * max(NdotL, 0.0);
#endif

#if _SPECULAR_SOURCE != 0
    vec3 R = reflect(-L, N);
    float RdotV = max(dot(R, V), 0.0);
    vec3 Cs = specular.rgb * pow(RdotV, specularPower);
    
    return Cd + Cs;
#else
    return Cd;
#endif
}

// Blinn-Phong lighting
vec3 litBlinnPhong(vec3 L, vec3 N, vec3 V, vec3 albedo, vec3 specular, float specularPower) {
#if defined(_WRAPPED_DIFFUSE)
    float NdotL = dot(N, L);
    float w2 = 1.0 + wrapped;
    vec3 Cd = albedo.rgb * (NdotL + wrapped) / (w2 * w2);
#else // Lambertian
    float NdotL = dot(N, L);
    vec3 Cd = albedo.rgb * max(NdotL, 0.0);
#endif

#if _SPECULAR_SOURCE != 0
    vec3 H = normalize(L + V);
    float NdotH = max(dot(N, H), 0.0);
    vec3 Cs = specular.rgb * pow(NdotH, specularPower);
    
    return Cd + Cs;
#else
    return Cd;
#endif
}

// Phong lighting that satisfy energy conservation
vec3 litPhongEC(vec3 L, vec3 N, vec3 V, vec3 albedo, vec3 specular, float specularPower) {
#if defined(_WRAPPED_DIFFUSE)
    float NdotL = dot(N, L);
    float w2 = 1.0 + wrapped;
    vec3 Cd = albedo.rgb * (vec3(1.0) - specular.rgb) * (NdotL + wrapped) / (w2 * w2);
#else // Lambertian
    float NdotL = dot(N, L);
    vec3 Cd = albedo.rgb * (vec3(1.0) - specular.rgb) * max(NdotL, 0.0);
#endif

#if _SPECULAR_SOURCE != 0
    vec3 R = reflect(-L, N);
    float RdotV = max(dot(R, V), 0.0);
    float normFactor = specularPower * 0.5 + 1.0;
    vec3 Cs = specular.rgb * normFactor * pow(RdotV, specularPower);

    return Cd + Cs;
#else
    return Cd;
#endif
}

// Blinn-Phong lighting that satisfy energy conservation
vec3 litBlinnPhongEC(vec3 L, vec3 N, vec3 V, vec3 albedo, vec3 specular, float specularPower) {
#if defined(_WRAPPED_DIFFUSE)
    float NdotL = dot(N, L);
    float w2 = 1.0 + wrapped;
    vec3 Cd = albedo.rgb * (vec3(1.0) - specular.rgb) * (NdotL + wrapped) / (w2 * w2);
#else // Lambertian
    float NdotL = dot(N, L);
    vec3 Cd = albedo.rgb * (vec3(1.0) - specular.rgb) * max(NdotL, 0.0);
#endif

#if _SPECULAR_SOURCE != 0
    vec3 H = normalize(L + V);
    float NdotH = max(dot(N, H), 0.0);
    float normFactor = specularPower * 0.125 + 1.0;
    vec3 Cs = specular.rgb * normFactor * pow(NdotH, specularPower);

    return Cd + Cs;
#else
    return Cd;
#endif
}

#endif