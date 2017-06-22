#define PBR_DIFFUSE 0
#define PBR_SPEC_D 2
#define PBR_SPEC_G 3

float pow5(float f) {
    float f2 = f * f;
    return f2 * f2 * f;
}

void litDiffuseLambert(in float NdotL, in vec4 Kd, out vec3 Cd) {
    Cd = Kd.rgb * NdotL / 3.14159265;
}

void litDiffuseBurley(in float NdotL, in float NdotV, in float VdotH, in float roughness, in vec4 Kd, out vec3 Cd) {
    float Fd90 = 0.5 + 2.0 * VdotH * VdotH * roughness;
    float FdL = 1.0 + (Fd90 - 1.0) * pow5(1.0 - NdotL);
    float FdV = 1.0 + (Fd90 - 1.0) * pow5(1.0 - NdotV);
    Cd = Kd.rgb * FdL * FdV / 3.14159265;
}

void litDiffuseOrenNayar(in float NdotL, in float NdotV, in float LdotV, in float roughness, in vec4 Kd, out vec3 Cd) { 
    //float sigma2 = roughness * roughness;
    //float A = 1.0 - sigma2 * (0.5 / (sigma2 + 0.33) + 0.17 / (sigma2 + 0.13));
    //float B = 0.45 * sigma2 / (sigma2  + 0.09);

    // A tiny improvement of Oren-Nayar [Yasuhiro Fujii]
    // http://mimosa-pudica.net/improved-oren-nayar.html
    float A = 1.0 / (3.14159265 + 0.90412966 * roughness);
    float B = roughness / (3.14159265 + 0.90412966 * roughness);

    float s = LdotV - NdotL * NdotV;
    float t = mix(1.0, max(NdotL, NdotV), step(0.0, s));    
    
    Cd = Kd.rgb * NdotL * (A + B * s / t) / 3.14159265;
}

float D_Blinn(float NdotH, float m) {
    float m2 = m * m;
    float n = 2.0 / m2 - 2.0;
    return pow(NdotH, n) * (n + 2.0) / 6.2831853;
}

float D_Beckmann(float NdotH, float m) {
    float m2 = m * m;
    float NdotH2 = NdotH * NdotH;
    return exp((NdotH2 - 1.0) / (m2 * NdotH2)) / (3.14159265 * m2 * NdotH2 * NdotH2);
}

float D_GGX(float NdotH, float m) {
    float m2 = m * m;
    float denom = NdotH * NdotH * (m2 - 1.0) + 1.0;
    return m2 / (3.14159265 * denom * denom);
}

float D_GGXAniso(float NdotH, float XdotH, float YdotH, float mx, float my) {
    float denom = XdotH * XdotH / (mx * mx) + YdotH * YdotH / (my * my) + NdotH * NdotH;
    return 1.0 / (3.14159265 * mx * my * denom * denom);
}

float G_Neumann(float NdotV, float NdotL) {
    return 1.0 / (4.0 * max(NdotL, NdotV));
}

float G_Kelemen(float VdotH) {
    return 1.0 / (4.0 * VdotH * VdotH);
}

float G_CookTorrance(float NdotV, float NdotL, float NdotH, float VdotH) {
    float G = min(1.0, min(2.0 * NdotH * NdotV / VdotH, (2.0 * NdotH * NdotL) / VdotH));
    return G / (4.0 * NdotL * NdotV);
}

float G_SchlickGGX(float NdotV, float NdotL, float m) {
    float k = m * 0.5f;
    float invK = 1.0 - k;
    float GV = NdotV * invK + k;
    float GL = NdotL * invK + k;
    return 0.25 / (GL * GV);
}

// Schlick's approximation
vec3 F_Schlick(vec3 F0, float LdotH) {
    return F0 + (vec3(1.0, 1.0, 1.0) - F0) * pow5(1.0 - LdotH);
}

void litStandard(in vec3 N, in vec3 L, in vec3 V, in float roughness, in vec4 Kd, in vec4 Ks, out vec3 Cd, out vec3 Cs) {
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0);
    float NdotH = max(dot(N, H), 0);
    float LdotH = max(dot(L, H), 0);

#if PBR_DIFFUSE == 0
    litDiffuseLambert(NdotL, Kd, Cd);
#elif PBR_DIFFUSE == 1
    float NdotV = max(dot(N, V), 0);
    float VdotH = max(dot(V, H), 0);

    litDiffuseBurley(NdotL, NdotV, VdotH, roughness, Kd, Cd);
#else PBR_DIFFUSE == 2
    float NdotV = max(dot(N, V), 0);
    float LdotV = max(dot(L, V), 0);

    litDiffuseOrenNayar(NdotL, NdotV, LdotV, roughness, Kd, Cd);
#endif

    float m = roughness * roughness;

    // Normal distribution term
#if PBR_SPEC_D == 0
    float D = D_Blinn(NdotH, m);
#elif PBR_SPEC_D == 1
    float D = D_Beckmann(NdotH, m);
#elif PBR_SPEC_D == 2
    float D = D_GGX(NdotH, m);
#endif

    // Geometric visibility term
#if PBR_SPEC_G == 0
    float NdotV = max(dot(N, V), 0);

    float G = G_Neumann(NdotV, NdotL);
#elif PBR_SPEC_G == 1
    float VdotH = max(dot(V, H), 0);

    float G = G_Kelemen(VdotH);
#elif PBR_SPEC_G == 2
    float NdotV = max(dot(N, V), 0);
    float VdotH = max(dot(V, H), 0);

    float G = G_CookTorrance(NdotV, NdotL, NdotH, VdotH);
#elif PBR_SPEC_G == 3
    float NdotV = max(dot(N, V), 0);

    float G = G_SchlickGGX(NdotV, NdotL, m);
#endif

    // Fresnel term (Schlick's approximation)
    vec3 F = F_Schlick(Ks.rgb, LdotH); 

    // Microfacets BRDF = D * G * F
    // G term is divided by (4 * NdotL * NdotV)
    Cs = NdotL * D * G * F;

    Cd *= (vec3(1.0, 1.0, 1.0) - F_Schlick(Ks.rgb, NdotL)); // is it correct ??
}