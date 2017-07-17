uniform float wrapped;
uniform float glossiness;
uniform float specularReflectance;

float glossinessToSpecularPower(float glossiness) {
    return exp2(10.0 * glossiness + 1.0); 
}

void litPhong(in vec3 L, in vec3 N, in vec3 V, in vec4 Kd, in vec4 Ks, out vec3 Cd, out vec3 Cs) {
#if defined(_WRAPPED_DIFFUSE)
    float NdotL = dot(N, L);
    float w2 = 1.0 + wrapped;
    Cd = Kd.rgb * (NdotL + wrapped) / (w2 * w2);
#else // Lambertian
    float NdotL = dot(N, L);
    Cd = Kd.rgb * max(NdotL, 0.0);
#endif

#if _SPECULAR_SOURCE == 0
    Cs = vec3(0.0, 0.0, 0.0);
#else
    vec3 R = reflect(-L, N);
    float RdotV = max(dot(R, V), 0.0);

    #if _SPECULAR_SOURCE == 3
        Cs = Ks.rgb * pow((NdotL > 0.0 ? RdotV : 0.0), glossinessToSpecularPower(Ks.a));
    #else
        Cs = Ks.rgb * pow((NdotL > 0.0 ? RdotV : 0.0), glossinessToSpecularPower(glossiness));
    #endif
#endif
}

// Blinn-Phong lighting
void litBlinnPhong(in vec3 L, in vec3 N, in vec3 V, in vec4 Kd, in vec4 Ks, out vec3 Cd, out vec3 Cs) {
#if defined(_WRAPPED_DIFFUSE)
    float NdotL = dot(N, L);
    float w2 = 1.0 + wrapped;
    Cd = Kd.rgb * (NdotL + wrapped) / (w2 * w2);
#else // Lambertian
    float NdotL = dot(N, L);
    Cd = Kd.rgb * max(NdotL, 0.0);
#endif

#if _SPECULAR_SOURCE == 0
    Cs = vec3(0.0, 0.0, 0.0);
#else
    vec3 H = normalize(L + V);
    float NdotH = max(dot(N, H), 0.0);

    #if _SPECULAR_SOURCE == 3
        Cs = Ks.rgb * pow((NdotL > 0.0 ? NdotH : 0.0), glossinessToSpecularPower(Ks.a));
    #else
        Cs = Ks.rgb * pow((NdotL > 0.0 ? NdotH : 0.0), glossinessToSpecularPower(glossiness));
    #endif
#endif
}

// Phong lighting that satisfy energy conservation
void litPhongEC(in vec3 L, in vec3 N, in vec3 V, in vec4 Kd, in vec4 Ks, out vec3 Cd, out vec3 Cs) {
#if defined(_WRAPPED_DIFFUSE)
    float NdotL = dot(N, L);
    float w2 = 1.0 + wrapped;
    Cd = Kd.rgb * (NdotL + wrapped) / (w2 * w2);
#else // Lambertian
    float NdotL = dot(N, L);
    Cd = Kd.rgb * max(NdotL, 0.0);
#endif

#if _SPECULAR_SOURCE == 0
    Cs = vec3(0.0, 0.0, 0.0);
#else
    vec3 R = reflect(-L, N);
    float RdotV = max(dot(R, V), 0.0);

    #if _SPECULAR_SOURCE == 3
        float normFactor = Ks.a * 0.5 + 1.0;
        Cs = Ks.rgb * normFactor * pow((NdotL > 0.0 ? RdotV : 0.0), glossinessToSpecularPower(Ks.a));
    #else
        float specularPower = glossinessToSpecularPower(glossiness);
        float normFactor = specularPower * 0.5 + 1.0;
        Cs = Ks.rgb * normFactor * pow((NdotL > 0.0 ? RdotV : 0.0), specularPower);
    #endif
#endif

#if _SPECULAR_SOURCE != 0
    Cs *= specularReflectance;
    Cd *= (1.0f - specularReflectance);
#endif
}

// Blinn-Phong lighting that satisfy energy conservation
void litBlinnPhongEC(in vec3 L, in vec3 N, in vec3 V, in vec4 Kd, in vec4 Ks, out vec3 Cd, out vec3 Cs) {
#if defined(_WRAPPED_DIFFUSE)
    float NdotL = dot(N, L);
    float w2 = 1.0 + wrapped;
    Cd = Kd.rgb * (NdotL + wrapped) / (w2 * w2);
#else // Lambertian
    float NdotL = dot(N, L);
    Cd = Kd.rgb * max(NdotL, 0.0);
#endif

#if _SPECULAR_SOURCE == 0
    Cs = vec3(0.0, 0.0, 0.0);
#else
    vec3 H = normalize(L + V);
    float NdotH = max(dot(N, H), 0.0);

    #if _SPECULAR_SOURCE == 3
        float normFactor = Ks.a * 0.125 + 1.0;
        Cs = Ks.rgb * normFactor * pow((NdotL > 0.0 ? NdotH : 0.0), glossinessToSpecularPower(Ks.a));
    #else
        float specularPower = glossinessToSpecularPower(glossiness);
        float normFactor = specularPower * 0.125 + 1.0;
        Cs = Ks.rgb * normFactor * pow((NdotL > 0.0 ? NdotH : 0.0), specularPower);
    #endif
#endif

#if _SPECULAR_SOURCE != 0
    Cs *= specularReflectance;
    Cd *= (1.0f - specularReflectance);
#endif
}
