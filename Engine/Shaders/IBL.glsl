#ifndef IBL_INCLUDED
#define IBL_INCLUDED

$include "BRDFLibrary.glsl"

// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
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
    const int numSamples = 64;

    vec3 diffuseLighting = vec3(0.0);

    for (int i = 0; i < numSamples; i++) {
        vec2 xi = hammersley(i, numSamples);

        vec3 L = importanceSampleLambert(xi, N);

        // BRDF = 1 / PI
        // PDF(L) = NdotL / PI
        //
        // Integrate { Li * BRDF * NdotL }
        // F_N = 1/N * Sigma^N { Li * BRDF * NdotL / PDF(L) }
        // = 1/N * Sigma^N { Li }
        diffuseLighting += texCUBE(radMap, L).rgb;
    }

    return diffuseLighting * diffuseColor / float(numSamples);
}

vec3 IBLSpecularPhong(samplerCube radMap, vec3 N, vec3 V, vec3 specularColor, float specularPower) {
    const int numSamples = 64;

    vec3 S = reflect(-V, N);

    vec3 specularLighting = vec3(0.0);

    for (int i = 0; i < numSamples; i++) {
        vec2 xi = hammersley(i, numSamples);

        vec3 L = importanceSamplePhongSpecular(xi, specularPower, S);

        // BRDF = (power + 2) * pow(LdotS, power) / (NdotL * TWO_PI)
        // PDF(L) = (power + 1) * pow(LdotS, power) / TWO_PI
        //
        // Integrate { Li * BRDF * NdotL }
        // F_N = 1/N * Sigma^N { Li * BRDF * NdotL / PDF(L) }
        // = 1/N * Sigma^N { Li * (power + 2) / (power + 1) }
        specularLighting += texCUBE(radMap, L).rgb;
    }

    return specularLighting * specularColor * (specularPower + 2.0) / (specularPower + 1.0) / float(numSamples);
}

vec3 IBLPhongWithFresnel(samplerCube radMap, vec3 N, vec3 V, vec3 diffuseColor, vec3 specularColor, float specularPower, float roughness) {
    const int numSamples = 64;

    vec3 diffuseLighting = vec3(0.0);

    vec3 specularLighting = vec3(0.0);

    vec3 S = reflect(-V, N);

    for (int i = 0; i < numSamples; i++) {
        vec2 xi = hammersley(i, numSamples);

        vec3 Ld = importanceSampleLambert(xi, N);

        vec3 Ls = importanceSamplePhongSpecular(xi, specularPower, S);
        
        diffuseLighting += texCUBE(radMap, Ld).rgb;

        specularLighting += texCUBE(radMap, Ls).rgb;
    }

    float NdotV = max(dot(N, V), 0.0);

    vec3 F = F_SchlickRoughness(specularColor, roughness, NdotV);

    specularLighting *= F * specularColor * ((specularPower + 2.0) / (specularPower + 1.0));

    diffuseLighting *= (vec3(1.0) - F) * diffuseColor;

    return (diffuseLighting + specularLighting) / float(numSamples);
}

vec3 IBLSpecularGGX(samplerCube radMap, vec3 N, vec3 V, vec3 specularColor, float roughness) {
    const int numSamples = 64;

    vec3 specularLighting = vec3(0.0);

    float k = roughness * roughness * 0.5; // k for IBL

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

            float G = G_SchlickGGX(NdotV, NdotL, k);

            vec3 F = F_SchlickSG(specularColor, VdotH); 

            // BRDF = D * G * F / 4 (G term is divided by (NdotL * NdotV))
            // PDF(H) = D * NdotH
            // PDF(L) = PDF(H) * dH / dL = D * NdotH / (4 * VdotH) (ref. PBRT 2nd Edition p698)
            //
            // Integrate { Li * BRDF * NdotL }
            // F_N = 1/N Sigma^N { Li * BRDF * NdotL / PDF(L)  }
            // = 1/N Sigma^N { (Li * D * G * F * NdotL / 4) / (D * NdotH / (4 * VdotH)) }
            // = 1/N Sigma^N { Li * G * F * NdotL * VdotH / NdotH }
            specularLighting += radiance * G * F * NdotL * VdotH / NdotH;
        }
    }

    return specularLighting / float(numSamples);
}

vec3 IBLDiffuseLambertWithSpecularGGX(samplerCube radMap, vec3 N, vec3 V, vec3 albedo, vec3 F0, float roughness) {
    const int numSamples = 64;

    vec3 diffuseLighting = vec3(0.0);

    vec3 specularLighting = vec3(0.0);

    for (int i = 0; i < numSamples; i++) {
        vec2 xi = hammersley(i, numSamples);

        vec3 Ld = importanceSampleLambert(xi, N);

        vec3 H = importanceSampleGGX(xi, roughness, N); 

        float VdotH = max(dot(V, H), 0.0);

        vec3 F = F_SchlickSG(F0, VdotH);

        diffuseLighting += (vec3(1.0) - F) * texCUBE(radMap, Ld).rgb;

        vec3 Ls = 2.0 * dot(V, H) * H - V;

        float NdotL = max(dot(N, Ls), 0.0);

        if (NdotL > 0.0) {
            float NdotH = max(dot(N, H), 0.0);
            float NdotV = max(dot(N, V), 0.0);

            float k = roughness * roughness * 0.5; // k for IBL
            float G = G_SchlickGGX(NdotV, NdotL, k);

            specularLighting += texCUBE(radMap, Ls).rgb * G * F * NdotL * VdotH / NdotH;
        }
    }

    diffuseLighting *= albedo;

    return (diffuseLighting + specularLighting) / float(numSamples);
}

#endif
