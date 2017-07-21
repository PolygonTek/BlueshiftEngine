#ifndef LIGHTING_PHONG_INCLUDED
#define LIGHTING_PHONG_INCLUDED

uniform float wrapped;
uniform float glossiness;
uniform float specularReflectance;

float glossinessToSpecularPower(float glossiness) {
    return exp2(10.0 * glossiness + 1.0); 
}

vec3 litPhong(vec3 L, vec3 N, vec3 V, vec4 Kd, vec4 Ks) {
#if defined(_WRAPPED_DIFFUSE)
    float NdotL = dot(N, L);
    float w2 = 1.0 + wrapped;
    vec3 Cd = Kd.rgb * (NdotL + wrapped) / (w2 * w2);
#else // Lambertian
    float NdotL = dot(N, L);
    vec3 Cd = Kd.rgb * max(NdotL, 0.0);
#endif

#if _SPECULAR_SOURCE != 0
    vec3 R = reflect(-L, N);
    float RdotV = max(dot(R, V), 0.0);

    #if _SPECULAR_SOURCE == 3
        vec3 Cs = Ks.rgb * pow((NdotL > 0.0 ? RdotV : 0.0), glossinessToSpecularPower(Ks.a));
    #else
        vec3 Cs = Ks.rgb * pow((NdotL > 0.0 ? RdotV : 0.0), glossinessToSpecularPower(glossiness));
    #endif

    return Cd + Cs;
#else
    return Cd;
#endif
}

// Blinn-Phong lighting
vec3 litBlinnPhong(vec3 L, vec3 N, vec3 V, vec4 Kd, vec4 Ks) {
#if defined(_WRAPPED_DIFFUSE)
    float NdotL = dot(N, L);
    float w2 = 1.0 + wrapped;
    vec3 Cd = Kd.rgb * (NdotL + wrapped) / (w2 * w2);
#else // Lambertian
    float NdotL = dot(N, L);
    vec3 Cd = Kd.rgb * max(NdotL, 0.0);
#endif

#if _SPECULAR_SOURCE != 0
    vec3 H = normalize(L + V);
    float NdotH = max(dot(N, H), 0.0);

    #if _SPECULAR_SOURCE == 3
        vec3 Cs = Ks.rgb * pow((NdotL > 0.0 ? NdotH : 0.0), glossinessToSpecularPower(Ks.a));
    #else
        vec3 Cs = Ks.rgb * pow((NdotL > 0.0 ? NdotH : 0.0), glossinessToSpecularPower(glossiness));
    #endif

    return Cd + Cs;
#else
    return Cd;
#endif
}

// Phong lighting that satisfy energy conservation
vec3 litPhongEC(vec3 L, vec3 N, vec3 V, vec4 Kd, vec4 Ks) {
#if defined(_WRAPPED_DIFFUSE)
    float NdotL = dot(N, L);
    float w2 = 1.0 + wrapped;
    vec3 Cd = Kd.rgb * (vec3(1.0) - Ks.rgb) * (NdotL + wrapped) / (w2 * w2);
#else // Lambertian
    float NdotL = dot(N, L);
    vec3 Cd = Kd.rgb * (vec3(1.0) - Ks.rgb) * max(NdotL, 0.0);
#endif

#if _SPECULAR_SOURCE != 0
    vec3 R = reflect(-L, N);
    float RdotV = max(dot(R, V), 0.0);

    #if _SPECULAR_SOURCE == 3
        float specularPower = glossinessToSpecularPower(Ks.a);
    #else
        float specularPower = glossinessToSpecularPower(glossiness);
    #endif

    float normFactor = specularPower * 0.5 + 1.0;
    vec3 Cs = Ks.rgb * normFactor * pow((NdotL > 0.0 ? RdotV : 0.0), specularPower);

    return Cd + Cs;
#else
    return Cd;
#endif
}

// Blinn-Phong lighting that satisfy energy conservation
vec3 litBlinnPhongEC(vec3 L, vec3 N, vec3 V, vec4 Kd, vec4 Ks) {
#if defined(_WRAPPED_DIFFUSE)
    float NdotL = dot(N, L);
    float w2 = 1.0 + wrapped;
    vec3 Cd = Kd.rgb * (vec3(1.0) - Ks.rgb) * (NdotL + wrapped) / (w2 * w2);
#else // Lambertian
    float NdotL = dot(N, L);
    vec3 Cd = Kd.rgb * (vec3(1.0) - Ks.rgb) * max(NdotL, 0.0);
#endif

#if _SPECULAR_SOURCE != 0
    vec3 H = normalize(L + V);
    float NdotH = max(dot(N, H), 0.0);

    #if _SPECULAR_SOURCE == 3
        float specularPower = glossinessToSpecularPower(Ks.a);
    #else
        float specularPower = glossinessToSpecularPower(glossiness);
    #endif

    float normFactor = specularPower * 0.125 + 1.0;
    vec3 Cs = Ks.rgb * normFactor * pow((NdotL > 0.0 ? NdotH : 0.0), specularPower);

    return Cd + Cs;
#else
    return Cd;
#endif
}

#endif