#define PBR_DIFFUSE 2
#define PBR_SPEC_D 2
#define PBR_SPEC_G 3

#define PI 3.1415926535897932384626433832795
#define TWO_PI 6.283185307179586476925286766559
#define HALF_PI 1.5707963267948966192313216916398
#define INV_PI 0.31830988618379067153776752674503
#define INV_TWO_PI 0.15915494309189533576888376337251

float pow5(float f) {
    float f2 = f * f;
    return f2 * f2 * f;
}

// Transform tangent space direction vector to local space direction vector
vec3 rotateWithUpVector(vec3 tangentDir, vec3 tangentZ) {
    vec3 tangentY = abs(tangentZ.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangentX = normalize(cross(tangentY, tangentZ));
    tangentY = cross(tangentZ, tangentX);
    return mat3(tangentX, tangentY, tangentZ) * tangentDir;
}

void litDiffuseLambert(in float NdotL, in vec4 Kd, out vec3 Cd) {
    Cd = Kd.rgb * NdotL * INV_PI;
}

void litDiffuseBurley(in float NdotL, in float NdotV, in float VdotH, in float roughness, in vec4 Kd, out vec3 Cd) {
    float Fd90 = 0.5 + 2.0 * VdotH * VdotH * roughness;
    float FdL = 1.0 + (Fd90 - 1.0) * pow5(1.0 - NdotL);
    float FdV = 1.0 + (Fd90 - 1.0) * pow5(1.0 - NdotV);
    Cd = Kd.rgb * FdL * FdV * INV_PI;
}

void litDiffuseOrenNayar(in float NdotL, in float NdotV, in float LdotV, in float roughness, in vec4 Kd, out vec3 Cd) { 
    float sigma2 = roughness * roughness;
    float A = 1.0 - sigma2 * (0.5 / (sigma2 + 0.33) + 0.17 / (sigma2 + 0.13));
    float B = 0.45 * sigma2 / (sigma2  + 0.09);

    // A tiny improvement of Oren-Nayar [Yasuhiro Fujii]
    // http://mimosa-pudica.net/improved-oren-nayar.html
    //float A = 1.0 / (PI + 0.90412966 * roughness);
    //float B = roughness / (PI + 0.90412966 * roughness);

    float s = LdotV - NdotL * NdotV;
    float t = mix(1.0, max(NdotL, NdotV), step(0.0, s));
    
    Cd = Kd.rgb * NdotL * (A + B * s / t) * INV_PI;
}

//---------------------------------------------------
// Normal distribution functions
//---------------------------------------------------

float D_Blinn(float NdotH, float m) {
    float m2 = m * m;
    float n = 2.0 / m2 - 2.0;
    return pow(NdotH, n) * (n + 2.0) * INV_TWO_PI;
}

float D_Beckmann(float NdotH, float m) {
    float m2 = m * m;
    float NdotH2 = NdotH * NdotH;
    return exp((NdotH2 - 1.0) / (m2 * NdotH2)) / (PI * m2 * NdotH2 * NdotH2);
}

// Trowbridge-Reitz aka GGX
float D_GGX(float NdotH, float m) {
    float m2 = m * m;
    float denom = NdotH * NdotH * (m2 - 1.0) + 1.0;
    return m2 / (PI * denom * denom);
}

float D_GGXAniso(float NdotH, float XdotH, float YdotH, float mx, float my) {
    float denom = XdotH * XdotH / (mx * mx) + YdotH * YdotH / (my * my) + NdotH * NdotH;
    return 1.0 / (PI * mx * my * denom * denom);
}

//---------------------------------------------------
// Geometric visibility functions
//---------------------------------------------------

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

//---------------------------------------------------
// Fresnel functions
//---------------------------------------------------

// Schlick's approximation
vec3 F_Schlick(vec3 F0, float VdotH) {
    return F0 + (vec3(1.0, 1.0, 1.0) - F0) * pow5(1.0 - VdotH);
}

// Schlick's approximation with spherical Gaussian approximation
vec3 F_SchlickSG(vec3 F0, float VdotH) {
    return F0 + (vec3(1.0, 1.0, 1.0) - F0) * exp2(-5.55473 * VdotH - 6.98316) * VdotH;
}

//---------------------------------------------------

void litStandard(in vec3 N, in vec3 L, in vec3 V, in float roughness, in vec4 Kd, in vec4 Ks, out vec3 Cd, out vec3 Cs) {
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0);
    float NdotH = max(dot(N, H), 0);
    float NdotV = max(dot(N, V), 0);
    float VdotH = max(dot(V, H), 0);

    //----------------------------------
    // Diffuse BRDF
    //----------------------------------
#if PBR_DIFFUSE == 0
    litDiffuseLambert(NdotL, Kd, Cd);
#elif PBR_DIFFUSE == 1
    litDiffuseBurley(NdotL, NdotV, VdotH, roughness, Kd, Cd);
#elif PBR_DIFFUSE == 2
    float LdotV = max(dot(L, V), 0);
    litDiffuseOrenNayar(NdotL, NdotV, LdotV, roughness, Kd, Cd);
#endif

    //----------------------------------
    // Specular BRDF
    //----------------------------------
    float m = roughness * roughness;

    // Normal distribution term
#if PBR_SPEC_D == 0
    float D = D_Blinn(NdotH, m);
#elif PBR_SPEC_D == 1
    float D = D_Beckmann(NdotH, m);
#elif PBR_SPEC_D == 2
    float D = D_GGX(NdotH, m);
#elif PBR_SPEC_D == 3
    float D = D_GGXAniso(NdotH, XdotH, YdotH, mx, my);
#endif

    // Geometric visibility term
#if PBR_SPEC_G == 0
    float G = G_Neumann(NdotV, NdotL);
#elif PBR_SPEC_G == 1
    float G = G_Kelemen(VdotH);
#elif PBR_SPEC_G == 2
    float G = G_CookTorrance(NdotV, NdotL, NdotH, VdotH);
#elif PBR_SPEC_G == 3
    float G = G_SchlickGGX(NdotV, NdotL, m);
#endif

    // Fresnel term
    vec3 F = F_SchlickSG(Ks.rgb, VdotH); 

    // Microfacets BRDF = D * G * F (G term is divided by (4 * NdotL * NdotV))
    Cs = PI * NdotL * D * G * F;

    Cd *= PI * (vec3(1.0, 1.0, 1.0) - F_SchlickSG(Ks.rgb, NdotL)); // is it correct ??
}

//---------------------------------------------------
// Importance sampling functions
//---------------------------------------------------

// Lambertian importance sample direction vector with respect to N
vec3 importanceSampleCosine(vec2 xi, vec3 N) {
    float sinTheta = sqrt(xi.x);
    float phi = TWO_PI * xi.y;

    vec3 sampleDir;
    sampleDir.x = sinTheta * cos(phi);
    sampleDir.y = sinTheta * sin(phi);
    sampleDir.z = sqrt(1.0 - xi.x);

    return rotateWithUpVector(sampleDir, N);
}

// Phong specular importance sample direction vector with respect to N
vec3 importanceSamplePowerCosine(vec2 xi, float power, vec3 N) {
    float cosTheta = pow(1.0 - xi.x, 1.0 / (power + 1));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float phi = TWO_PI * xi.y;
    
    vec3 sampleDir;
    sampleDir.x = sinTheta * cos(phi);
    sampleDir.y = sinTheta * sin(phi);
    sampleDir.z = cosTheta;

    return rotateWithUpVector(sampleDir, N);
}

// Importance sample direction vector for GGX specular with respect to N
vec3 importanceSampleGGX(vec2 xi, float roughness, vec3 N) {
    float alpha = roughness * roughness;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (alpha * alpha - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float phi = TWO_PI * xi.y;

    vec3 sampleDir;
    sampleDir.x = sinTheta * cos(phi);
    sampleDir.y = sinTheta * sin(phi);
    sampleDir.z = cosTheta;

    return rotateWithUpVector(sampleDir, N);
}