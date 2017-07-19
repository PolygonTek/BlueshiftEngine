#define PBR_DIFFUSE 1
#define PBR_SPEC_D 2
#define PBR_SPEC_G 3

#define PI 3.1415926535897932384626433832795
#define TWO_PI 6.283185307179586476925286766559
#define HALF_PI 1.5707963267948966192313216916398
#define INV_PI 0.31830988618379067153776752674503
#define INV_TWO_PI 0.15915494309189533576888376337251

float radicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & uint(0x55555555)) << 1u) | ((bits & uint(0xAAAAAAAA)) >> 1u);
    bits = ((bits & uint(0x33333333)) << 2u) | ((bits & uint(0xCCCCCCCC)) >> 2u);
    bits = ((bits & uint(0x0F0F0F0F)) << 4u) | ((bits & uint(0xF0F0F0F0)) >> 4u);
    bits = ((bits & uint(0x00FF00FF)) << 8u) | ((bits & uint(0xFF00FF00)) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 hammersley(int i, int N) {
    return vec2(float(i) / float(N), radicalInverse_VdC(uint(i)));
}  

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

float litDiffuseLambert(in float NdotL) {
    return NdotL;
}

float litDiffuseBurley(in float NdotL, in float NdotV, in float VdotH, in float roughness) {
    float Fd90 = 0.5 + 2.0 * VdotH * VdotH * roughness;
    float FdL = 1.0 + (Fd90 - 1.0) * pow5(1.0 - NdotL);
    float FdV = 1.0 + (Fd90 - 1.0) * pow5(1.0 - NdotV);
    return NdotL * FdL * FdV;
}

float litDiffuseOrenNayar(in float NdotL, in float NdotV, in float LdotV, in float roughness) { 
    float sigma2 = roughness * roughness;
    float A = 1.0 - sigma2 * (0.5 / (sigma2 + 0.33) + 0.17 / (sigma2 + 0.13));
    float B = 0.45 * sigma2 / (sigma2  + 0.09);

    // A tiny improvement of Oren-Nayar [Yasuhiro Fujii]
    // http://mimosa-pudica.net/improved-oren-nayar.html
    //float A = 1.0 / (PI + 0.90412966 * roughness);
    //float B = roughness / (PI + 0.90412966 * roughness);

    float s = LdotV - NdotL * NdotV;
    float t = mix(1.0, max(NdotL, NdotV), step(0.0, s));
    
    return NdotL * (A + B * s / t);
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
    return 1.0 / max(NdotL, NdotV);
}

float G_Kelemen(float VdotH) {
    return 1.0 / (VdotH * VdotH);
}

float G_CookTorrance(float NdotV, float NdotL, float NdotH, float VdotH) {
    float G = min(1.0, min(2.0 * NdotH * NdotV / VdotH, (2.0 * NdotH * NdotL) / VdotH));
    return G / (NdotL * NdotV);
}

float G_SchlickGGX(float NdotV, float NdotL, float m) {
    float k = m * 0.5f;
    float invK = 1.0 - k;
    float GV = NdotV * invK + k;
    float GL = NdotL * invK + k;
    return 1.0 / (GL * GV);
}

//---------------------------------------------------
// Fresnel functions
//---------------------------------------------------

// Fresnel using Schlick's approximation
vec3 F_Schlick(vec3 F0, float VdotH) {
    return F0 + (vec3(1.0) - F0) * pow5(1.0 - VdotH);
}

// Fresnel using Schlick's approximation with spherical Gaussian approximation
vec3 F_SchlickSG(vec3 F0, float VdotH) {
    return F0 + (vec3(1.0) - F0) * exp2(-5.55473 * VdotH - 6.98316) * VdotH;
}

// Fresnel injected roughness term
vec3 F_SchlickRoughness(vec3 F0, float roughness, float VdotH) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow5(1.0 - VdotH);
}

//---------------------------------------------------

void litStandard(in vec3 L, in vec3 N, in vec3 V, in vec3 albedo, in float roughness, in float metalness, out vec3 Cd, out vec3 Cs) {
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    //----------------------------------
    // Diffuse BRDF
    //----------------------------------
#if PBR_DIFFUSE == 0
    Cd = albedo * litDiffuseLambert(NdotL);
#elif PBR_DIFFUSE == 1
    Cd = albedo * litDiffuseBurley(NdotL, NdotV, VdotH, roughness);
#elif PBR_DIFFUSE == 2
    float LdotV = max(dot(L, V), 0);
    Cd = albedo * litDiffuseOrenNayar(NdotL, NdotV, LdotV, roughness);
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

    // Specular Fresnel term
    vec3 F0 = mix(vec3(0.04), albedo, metalness);
    vec3 F = F_SchlickSG(F0, VdotH); 

    // Microfacets specular BRDF = D * G * F / 4 (G term is divided by (NdotL * NdotV))
    vec3 BRDFspec = D * G * F * 0.25;

    // Final specular lighting
    Cs = PI * NdotL * BRDFspec;

    // Final diffuse lighting with fresnel
    Cd *= (vec3(1.0) - F) * (1.0 - metalness);
    //Cd *= (vec3(1.0) - F_SchlickSG(F0, NdotL));
}

//---------------------------------------------------
// Importance sampling functions
//---------------------------------------------------

// Returns importance sampled incident direction for Lambert diffuse reflectance with respect to N
vec3 importanceSampleLambert(vec2 xi, vec3 N) {
    float cosTheta = sqrt(1.0 - xi.x);
    float sinTheta = sqrt(xi.x);
    float phi = TWO_PI * xi.y;

    vec3 sampleDir;
    sampleDir.x = sinTheta * cos(phi);
    sampleDir.y = sinTheta * sin(phi);
    sampleDir.z = cosTheta;

    return rotateWithUpVector(sampleDir, N);
}

// Returns importance sampled incident direction for Phong specular reflectance with respect to S
vec3 importanceSamplePhongSpecular(vec2 xi, float power, vec3 S) {
    float cosTheta = pow(xi.x, 1.0 / (power + 1.0));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float phi = TWO_PI * xi.y;
    
    vec3 sampleDir;
    sampleDir.x = sinTheta * cos(phi);
    sampleDir.y = sinTheta * sin(phi);
    sampleDir.z = cosTheta;

    return rotateWithUpVector(sampleDir, S);
}

// Returns importance sampled halfway direction for Beckmann specular NDF with respect to N
vec3 importanceSampleBeckmann(vec2 xi, float roughness, vec3 N) {
    float alpha = roughness * roughness;
    float cosTheta = sqrt(1.0 / (1.0 - alpha * alpha * log(xi.x)));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float phi = TWO_PI * xi.y;

    vec3 sampleDir;
    sampleDir.x = sinTheta * cos(phi);
    sampleDir.y = sinTheta * sin(phi);
    sampleDir.z = cosTheta;

    return rotateWithUpVector(sampleDir, N);
}

// Returns importance sampled halfway direction for GGX specular NDF with respect to N
vec3 importanceSampleGGX(vec2 xi, float roughness, vec3 N) {
    float alpha = roughness * roughness;
    float cosTheta = sqrt(xi.x / (alpha * alpha * (1.0 - xi.x) + xi.x));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float phi = TWO_PI * xi.y;

    vec3 sampleDir;
    sampleDir.x = sinTheta * cos(phi);
    sampleDir.y = sinTheta * sin(phi);
    sampleDir.z = cosTheta;

    return rotateWithUpVector(sampleDir, N);
}

vec3 IBLspecularPhong(samplerCube radMap, vec3 N, vec3 V, float specularPower, vec4 specularColor) {
    const int numSamples = 64;

    vec3 specularLighting = vec3(0.0);

    for (int i = 0; i < numSamples; i++) {
        vec2 xi = hammersley(i, numSamples);

        vec3 S = reflect(-V, N);

        vec3 L = importanceSamplePhongSpecular(xi, specularPower, S);

        float NdotL = max(dot(N, L), 0.0);

        if (NdotL > 0) {
            vec3 radiance = texCUBE(radMap, L).rgb;

            vec3 R = reflect(-L, N);
            
            float RdotV = max(dot(R, V), 0.0);

            float normFactor = specularPower * 0.5 + 1.0;

            specularLighting += radiance * specularColor.rgb * normFactor * pow(RdotV, specularPower);
        }
    }

    return specularLighting / numSamples;
}

vec3 IBLspecularGGX(samplerCube radMap, vec3 N, vec3 V, float roughness, vec4 specularColor) {
    const int numSamples = 64;

    vec3 specularLighting = vec3(0.0);

    float m = roughness * roughness;

    for (int i = 0; i < numSamples; i++) {
        vec2 xi = hammersley(i, numSamples);

        vec3 H = importanceSampleGGX(xi, roughness, N); 
        vec3 L = 2.0 * dot(V, H) * H - V;

        float NdotL = max(dot(N, L), 0.0);

        if (NdotL > 0.0) {
            vec3 radiance = texCUBE(radMap, L).rgb;

            float NdotH = max(dot(N, H), 0.0);
            float NdotV = max(dot(N, V), 0.0);
            float VdotH = max(dot(V, H), 0.0);

            float D = D_GGX(NdotH, m);

            float G = G_SchlickGGX(NdotV, NdotL, m);

            vec3 F = F_SchlickSG(specularColor.rgb, VdotH); 

            // Microfacets specular BRDF = D * G * F / 4 (G term is divided by (NdotL * NdotV))
            //
            // PDF(H) = D * NdotH
            // PDF(L) = PDF(H) * dH / dL = D * NdotH / (4 * VdotH) (ref. PBRT 2nd Edition p698)
            //
            // Monte Carlo summation term = radiance * BRDF * NdotL / PDF(L) 
            // = (radiance * D * G * F * NdotL / 4) / (D * NdotH / (4 * VdotH))
            // = radiance * G * F * NdotL * VdotH / NdotH
            specularLighting += radiance * G * F * NdotL * VdotH / NdotH;
        }
    }

    return specularLighting / numSamples;
}
