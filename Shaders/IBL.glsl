#ifndef IBL_INCLUDED
#define IBL_INCLUDED

$include "BRDFLibrary.glsl"

// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & uint(0x55555555)) << 1u) | ((bits & uint(0xAAAAAAAA)) >> 1u);
    bits = ((bits & uint(0x33333333)) << 2u) | ((bits & uint(0xCCCCCCCC)) >> 2u);
    bits = ((bits & uint(0x0F0F0F0F)) << 4u) | ((bits & uint(0xF0F0F0F0)) >> 4u);
    bits = ((bits & uint(0x00FF00FF)) << 8u) | ((bits & uint(0xFF00FF00)) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(int i, int N) {
    return vec2(float(i) / float(N), RadicalInverse_VdC(uint(i)));
}

// Build local frame matrix to transform tangent space direction vector to local space direction vector
mat3 GetLocalFrame(vec3 tangentZ) {
    vec3 tangentY = abs(tangentZ.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangentX = normalize(cross(tangentY, tangentZ));
    tangentY = cross(tangentZ, tangentX);
    return mat3(tangentX, tangentY, tangentZ);
}

// Returns importance sampled incident light direction for Lambert diffuse reflectance with respect to N
vec3 ImportanceSampleLambert(vec2 xi) {
    float cosTheta = sqrt(1.0 - xi.x);
    float sinTheta = sqrt(xi.x);
    float phi = TWO_PI * xi.y;

    // Transform from spherical into cartesian
    vec3 sampleDir;
    sampleDir.x = sinTheta * cos(phi);
    sampleDir.y = sinTheta * sin(phi);
    sampleDir.z = cosTheta;

    return sampleDir;
}

// Returns importance sampled incident light direction for Phong specular reflectance with respect to S
vec3 ImportanceSamplePhongSpecular(vec2 xi, float power) {
    float cosTheta = pow(xi.x, 1.0 / (power + 1.0));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float phi = TWO_PI * xi.y;
    
    // Transform from spherical into cartesian
    vec3 sampleDir;
    sampleDir.x = sinTheta * cos(phi);
    sampleDir.y = sinTheta * sin(phi);
    sampleDir.z = cosTheta;

    return sampleDir;
}

// Returns importance sampled halfway direction for Beckmann specular NDF with respect to N
vec3 ImportanceSampleBeckmann(vec2 xi, float roughness) {
    float alpha = roughness * roughness;
    float cosTheta = sqrt(1.0 / (1.0 - alpha * alpha * log(xi.x)));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float phi = TWO_PI * xi.y;

    // Transform from spherical into cartesian
    vec3 sampleDir;
    sampleDir.x = sinTheta * cos(phi);
    sampleDir.y = sinTheta * sin(phi);
    sampleDir.z = cosTheta;

    return sampleDir;
}

// Returns importance sampled halfway direction for GGX specular NDF with respect to N
vec3 ImportanceSampleGGX(vec2 xi, float roughness) {
    float alpha = roughness * roughness;
    float cosTheta = sqrt(xi.x / (alpha * alpha * (1.0 - xi.x) + xi.x));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float phi = TWO_PI * xi.y;

    // Transform from spherical into cartesian
    vec3 sampleDir;
    sampleDir.x = sinTheta * cos(phi);
    sampleDir.y = sinTheta * sin(phi);
    sampleDir.z = cosTheta;

    return sampleDir;
}

vec3 IBLDiffuseLambert(samplerCube radMap) {
    const int numSamples = 64;

    vec3 diffuseLighting = vec3(0.0);

    mat3 tangentToWorld = GetLocalFrame(shading.n);

    for (int i = 0; i < numSamples; i++) {
        vec2 xi = Hammersley(i, numSamples);

        vec3 L = tangentToWorld * ImportanceSampleLambert(xi);

        // BRDF = 1 / PI
        // PDF(L) = NdotL / PI
        //
        // Integrate { Li * BRDF * NdotL }
        // F_N = 1/N * Sigma^N { Li * BRDF * NdotL / PDF(L) }
        // = 1/N * Sigma^N { Li }
        diffuseLighting += texCUBE(radMap, L).rgb;
    }

    return diffuseLighting * shading.diffuse.rgb / float(numSamples);
}

vec3 IBLSpecularPhong(samplerCube radMap) {
    const int numSamples = 64;

    vec3 specularLighting = vec3(0.0);

    vec3 S = reflect(-shading.v, shading.n);

    mat3 tangentToWorld = GetLocalFrame(S);

    for (int i = 0; i < numSamples; i++) {
        vec2 xi = Hammersley(i, numSamples);

        vec3 L = tangentToWorld * ImportanceSamplePhongSpecular(xi, shading.specularPower);

        // BRDF = (power + 2) * pow(LdotS, power) / (NdotL * TWO_PI)
        // PDF(L) = (power + 1) * pow(LdotS, power) / TWO_PI
        //
        // Integrate { Li * BRDF * NdotL }
        // F_N = 1/N * Sigma^N { Li * BRDF * NdotL / PDF(L) }
        // = 1/N * Sigma^N { Li * (power + 2) / (power + 1) }
        specularLighting += texCUBE(radMap, L).rgb;
    }

    return specularLighting * shading.specular.rgb * (shading.specularPower + 2.0) / (shading.specularPower + 1.0) / float(numSamples);
}

vec3 IBLPhongWithFresnel(samplerCube radMap) {
    const int numSamples = 64;

    vec3 diffuseLighting = vec3(0.0);

    vec3 specularLighting = vec3(0.0);

    vec3 S = reflect(-shading.v, shading.n);

    mat3 tangentToWorld1 = GetLocalFrame(shading.n);

    mat3 tangentToWorld2 = GetLocalFrame(S);

    for (int i = 0; i < numSamples; i++) {
        vec2 xi = Hammersley(i, numSamples);

        vec3 Ld = tangentToWorld1 * ImportanceSampleLambert(xi);

        vec3 Ls = tangentToWorld2 * ImportanceSamplePhongSpecular(xi, shading.specularPower);
        
        diffuseLighting += texCUBE(radMap, Ld).rgb;

        specularLighting += texCUBE(radMap, Ls).rgb;
    }

    float NdotV = max(dot(shading.n, shading.v), 0.0);

    vec3 F = F_SchlickRoughness(shading.specular.rgb, shading.roughness, NdotV);

    specularLighting *= F * shading.specular.rgb * ((shading.specularPower + 2.0) / (shading.specularPower + 1.0));

    diffuseLighting *= (vec3(1.0) - F) * shading.diffuse.rgb;

    return (diffuseLighting + specularLighting) / float(numSamples);
}

vec3 IBLSpecularGGX(samplerCube radMap) {
    const int numSamples = 64;

    vec3 specularLighting = vec3(0.0);

    mat3 tangentToWorld = GetLocalFrame(shading.n);

    // k for IBL
    float k = shading.roughness * shading.roughness * 0.5;

    for (int i = 0; i < numSamples; i++) {
        vec2 xi = Hammersley(i, numSamples);

        vec3 H = tangentToWorld * ImportanceSampleGGX(xi, shading.roughness); 
        vec3 L = 2.0 * dot(shading.v, H) * H - shading.v;

        float NdotL = max(dot(shading.n, L), 0.0);

        if (NdotL > 0.0) {
            vec3 radiance = texCUBE(radMap, L).rgb;

            float NdotH = max(dot(shading.n, H), 0.0);
            float NdotV = max(dot(shading.n, shading.v), 0.0);
            float VdotH = max(dot(shading.v, H), 0.0);

            float G = G_SchlickGGX(NdotV, NdotL, k);

            vec3 F = F_SchlickSG(shading.specular.rgb, VdotH);

            // BRDF = D * G * F / 4 (G term is divided by (NdotL * NdotV))
            // PDF(H) = D * NdotH
            // PDF(L) = PDF(H) * dH / dL = D * NdotH / (4 * VdotH) (ref. PBRT 2nd Edition p698)
            //
            // Integrate { Li * BRDF * NdotL }
            // F_N = 1/N Sigma^N { Li * BRDF * NdotL / PDF(L)  }
            //     = 1/N Sigma^N { (Li * D * G * F * NdotL / 4) / (D * NdotH / (4 * VdotH)) }
            //     = 1/N Sigma^N { Li * G * F * NdotL * VdotH / NdotH }
            specularLighting += radiance * G * F * NdotL * VdotH / NdotH;
        }
    }

    return specularLighting / float(numSamples);
}

vec3 IBLDiffuseLambertWithSpecularGGX(samplerCube radMap) {
    const int numSamples = 64;

    vec3 diffuseLighting = vec3(0.0);

    vec3 specularLighting = vec3(0.0);

    mat3 tangentToWorld = GetLocalFrame(shading.n);

    // k for IBL
    float k = shading.roughness * shading.roughness * 0.5;

    for (int i = 0; i < numSamples; i++) {
        vec2 xi = Hammersley(i, numSamples);

        vec3 Ld = tangentToWorld * ImportanceSampleLambert(xi);

        vec3 H = tangentToWorld * ImportanceSampleGGX(xi, shading.roughness);

        float VdotH = max(dot(shading.v, H), 0.0);

        vec3 F = F_SchlickSG(shading.specular.rgb, VdotH);

        diffuseLighting += (vec3(1.0) - F) * texCUBE(radMap, Ld).rgb;

        vec3 Ls = 2.0 * dot(shading.v, H) * H - shading.v;

        float NdotL = max(dot(shading.n, Ls), 0.0);

        if (NdotL > 0.0) {
            float NdotH = max(dot(shading.n, H), 0.0);
            float NdotV = max(dot(shading.n, shading.v), 0.0);

            float G = G_SchlickGGX(NdotV, NdotL, k);

            specularLighting += texCUBE(radMap, Ls).rgb * G * F * NdotL * VdotH / NdotH;
        }
    }

    diffuseLighting *= shading.diffuse.rgb;

    return (diffuseLighting + specularLighting) / float(numSamples);
}

#endif
