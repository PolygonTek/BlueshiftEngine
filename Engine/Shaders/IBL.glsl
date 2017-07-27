#ifndef IBL_INCLUDED
#define IBL_INCLUDED

$include "Lighting.glsl"

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

// Transform tangent space direction vector to local space direction vector
vec3 rotateWithUpVector(vec3 tangentDir, vec3 tangentZ) {
    vec3 tangentY = abs(tangentZ.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangentX = normalize(cross(tangentY, tangentZ));
    tangentY = cross(tangentZ, tangentX);
    return mat3(tangentX, tangentY, tangentZ) * tangentDir;
}

// Returns importance sampled incident direction for Lambert diffuse reflectance with respect to N
vec3 importanceSampleLambert(vec2 xi, vec3 N) {
    float cosTheta = sqrt(1.0 - xi.x);
    float sinTheta = sqrt(xi.x);
    float phi = TWO_PI * xi.y;

    // Transform from spherical into cartesian
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
    
    // Transform from spherical into cartesian
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

    // Transform from spherical into cartesian
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

    // Transform from spherical into cartesian
    vec3 sampleDir;
    sampleDir.x = sinTheta * cos(phi);
    sampleDir.y = sinTheta * sin(phi);
    sampleDir.z = cosTheta;

    return rotateWithUpVector(sampleDir, N);
}

vec3 IBLDiffuseLambert(samplerCube radMap, vec3 N, vec3 diffuseColor) {
    const int numSamples = 100;

    vec3 diffuseLighting = vec3(0.0);

    for (int i = 0; i < numSamples; i++) {
        vec2 xi = hammersley(i, numSamples);

        vec3 L = importanceSampleLambert(xi, N);

        // Integrate { Li * BRDF * NdotL }
        // F_N = 1/N * Sigma^N { Li * BRDF * NdotL / PDF }
        // BRDF = 1 / PI
        // PDF = NdotL / PI
        // F_N = 1/N * Sigma^N { Li }
        diffuseLighting += texCUBE(radMap, L).rgb;
    }

    return diffuseLighting * diffuseColor / numSamples;
}

vec3 IBLSpecularPhong(samplerCube radMap, vec3 N, vec3 V, vec3 specularColor, float specularPower) {
    const int numSamples = 100;

    vec3 S = reflect(-V, N);

    vec3 specularLighting = vec3(0.0);

    for (int i = 0; i < numSamples; i++) {
        vec2 xi = hammersley(i, numSamples);

        vec3 L = importanceSamplePhongSpecular(xi, specularPower, S);

        // Integrate { Li * BRDF * NdotL }
        // F_N = 1/N * Sigma^N { Li * BRDF * NdotL / PDF }
        // BRDF = (power + 2) * pow(LdotS, power) / (NdotL * TWO_PI)
        // PDF = (power + 1) * pow(LdotS, power) / TWO_PI
        // F_N = 1/N * Sigma^N { Li * (power + 2) / (power + 1) }
        specularLighting += texCUBE(radMap, L).rgb;
    }

    return specularLighting * specularColor * (specularPower + 2.0) / (specularPower + 1.0) / numSamples;
}

vec3 IBLPhongWithFresnel(samplerCube radMap, vec3 N, vec3 V, vec3 diffuseColor, vec3 specularColor, float glossiness) {
    float specularPower = glossinessToSpecularPower(glossiness);

    float NdotV = max(dot(N, V), 0.0);

    vec3 F = F_SchlickRoughness(specularColor, sqrt(1.0 - glossiness), NdotV);

    vec3 Cs = F * IBLSpecularPhong(radMap, N, V, specularColor, specularPower);
    vec3 Cd = (vec3(1.0) - F) * IBLDiffuseLambert(radMap, N, diffuseColor);

    return Cd + Cs;
}

vec3 IBLSpecularGGX(samplerCube radMap, vec3 N, vec3 V, vec3 specularColor, float roughness) {
    const int numSamples = 100;

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

            vec3 F = F_SchlickSG(specularColor, VdotH); 

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

#endif
