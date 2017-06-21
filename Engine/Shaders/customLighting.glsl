uniform float specularExponent;
uniform float specularReflectance;
const float specularExponentMapScale = 64.0;

void litPhong(in vec3 N, in vec3 L, in vec3 V, in vec4 Kd, in vec4 Ks, out vec3 Cd, out vec3 Cs) {
#if defined(_HALF_LAMBERT_DIFFUSE)
    float NdotL = dot(N, L);
    float halfLambert = NdotL * 0.5 + 0.5;
    halfLambert *= halfLambert;
    Cd = Kd.rgb * halfLambert;
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
        float specExp = max(Ks.a * specularExponentMapScale, 1.0);
        Cs = Ks.rgb * pow((NdotL > 0.0 ? RdotV : 0.0), specExp);
    #else
        Cs = Ks.rgb * pow((NdotL > 0.0 ? RdotV : 0.0), specularExponent);
    #endif
#endif
}

// Phong lighting that satisfy energy conservation
void litPhongEC(in vec3 N, in vec3 L, in vec3 V, in vec4 Kd, in vec4 Ks, out vec3 Cd, out vec3 Cs) {
#if defined(_HALF_LAMBERT_DIFFUSE)
    float NdotL = dot(N, L);
    float halfLambert = NdotL * 0.5 + 0.5;
    halfLambert *= halfLambert;
    Cd = Kd.rgb * halfLambert;
#else // Lambertian
    float NdotL = dot(N, L);
    Cd = Kd.rgb * max(NdotL / 3.14159265, 0.0);
#endif

#if _SPECULAR_SOURCE == 0
    Cs = vec3(0.0, 0.0, 0.0);
#else
    vec3 R = reflect(-L, N);     
    float RdotV = max(dot(R, V), 0.0);

    #if _SPECULAR_SOURCE == 3
        float specExp = max(Ks.a * specularExponentMapScale, 1.0);
        float normFactor = (specExp + 2.0) / 6.2831853;
        Cs = Ks.rgb * normFactor * NdotL * pow((NdotL > 0.0 ? RdotV : 0.0), specExp);
    #else
        float normFactor = (specularExponent + 2.0) / 6.2831853;
        Cs = Ks.rgb * normFactor * NdotL * pow((NdotL > 0.0 ? RdotV : 0.0), specularExponent);
    #endif
#endif

#if _SPECULAR_SOURCE != 0
    Cs *= specularReflectance;
    Cd *= (1.0f - specularReflectance);
#endif
}

void litDiffuseLambertian(in vec3 N, in vec3 L, in vec4 Kd, out vec3 Cd) {
    float NdotL = dot(N, L);
    Cd = Kd.rgb * max(NdotL / 3.14159265, 0.0);
}

void litDiffuseOrenNayar(in vec3 N, in vec3 L, in vec3 V, in float roughness, in vec4 Kd, out vec3 Cd) { 
    float LdotV = dot(L, V);
    float NdotL = dot(N, L);
    float NdotV = dot(N, V);

    //float sigma2 = roughness * roughness;
    //float A = 1.0 - sigma2 * (0.5 / (sigma2 + 0.33) + 0.17 / (sigma2 + 0.13));
    //float B = 0.45 * sigma2 / (sigma2  + 0.09);

    // A tiny improvement of Oren-Nayar [Yasuhiro Fujii]
    // http://mimosa-pudica.net/improved-oren-nayar.html
    float A = 1.0 / (3.14159265 + 0.90412966 * roughness);
    float B = roughness / (3.14159265 + 0.90412966 * roughness);

    float s = LdotV - NdotL * NdotV;
    float t = mix(1.0, max(NdotL, NdotV), step(0.0, s));    
    
    Cd = Kd.rgb * max(NdotL, 0) * (A + B * s / t) / 3.14159265;
}

void litSpecularGGX(in vec3 N, in vec3 L, in vec3 V, in float roughness, in float F0, in vec4 Ks, out vec3 Cs) {
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0);
    float NdotH = max(dot(N, H), 0);
    float LdotH = max(dot(L, H), 0);

    // Normal distribution
    float alpha = roughness * roughness;
    float alphaSqr = alpha * alpha;
    float denom = NdotH * NdotH * (alphaSqr - 1.0) + 1.0;
    float D = alphaSqr / (3.14159265 * denom * denom);

    // Geometric (cheaper version of Schlick-Smith)
    float k = alpha * 0.5f;
    float k2 = k * k;
    float invK2 = 1.0 - k2;
    float G = 1.0 / (LdotH * LdotH * invK2 + k2);
    
    // Fresnel (Schlick's approximation)
    float LdotH5 = pow(1.0 - LdotH, 5.0);
    float F = F0 + (1.0 - F0) * LdotH5;

    Cs = Ks.rgb * NdotL * D * G * F;
}
