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
vec3 ImportanceSampleBeckmann(vec2 xi, float linearRoughness) {
    float cosTheta2 = 1.0 / (1.0 - linearRoughness * linearRoughness * log(xi.x));
    float cosTheta = sqrt(cosTheta2);
    float sinTheta = sqrt(1.0 - cosTheta2);
    float phi = TWO_PI * xi.y;

    // Transform from spherical into cartesian
    vec3 sampleDir;
    sampleDir.x = sinTheta * cos(phi);
    sampleDir.y = sinTheta * sin(phi);
    sampleDir.z = cosTheta;

    return sampleDir;
}

// Returns importance sampled halfway direction for GGX specular NDF with respect to N
vec3 ImportanceSampleGGX(vec2 xi, float linearRoughness) {
    float cosTheta2 = xi.x / (linearRoughness * linearRoughness * (1.0 - xi.x) + xi.x);
    float cosTheta = sqrt(cosTheta2);
    float sinTheta = sqrt(1.0 - cosTheta2);
    float phi = TWO_PI * xi.y;

    // Transform from spherical into cartesian
    vec3 sampleDir;
    sampleDir.x = sinTheta * cos(phi);
    sampleDir.y = sinTheta * sin(phi);
    sampleDir.z = cosTheta;

    return sampleDir;
}

#ifdef INDIRECT_LIGHTING

$include "ShadingParms.glsl"

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

    vec3 F = F_SchlickRoughness(shading.specular.rgb, shading.roughness, shading.ndotv);

    specularLighting *= F * shading.specular.rgb * ((shading.specularPower + 2.0) / (shading.specularPower + 1.0));

    diffuseLighting *= (vec3(1.0) - F) * shading.diffuse.rgb;

    return (diffuseLighting + specularLighting) / float(numSamples);
}

vec3 IBLSpecularGGX(samplerCube radMap) {
    const int numSamples = 64;

    vec3 specularLighting = vec3(0.0);

    mat3 tangentToWorld = GetLocalFrame(shading.n);

    // k for IBL = roughness^2 / 2
    float k = shading.linearRoughness * 0.5;

    for (int i = 0; i < numSamples; i++) {
        vec2 xi = Hammersley(i, numSamples);

        vec3 H = tangentToWorld * ImportanceSampleGGX(xi, shading.linearRoughness);
        vec3 L = 2.0 * dot(shading.v, H) * H - shading.v;

        float NdotL = max(dot(shading.n, L), 0.0);

        if (NdotL > 0.0) {
            vec3 radiance = texCUBE(radMap, L).rgb;

            float NdotH = max(dot(shading.n, H), 0.0);
            float VdotH = max(dot(shading.v, H), 0.0);

            float G = G_SchlickGGX(shading.ndotv, NdotL, k);

            vec3 F = F_SchlickSG(shading.specular.rgb, VdotH);

            // BRDF = D * G * F (G term is divided by (4 * NdotL * NdotV))
            // PDF(H) = D * NdotH
            // PDF(L) = PDF(H) * dH / dL = D * NdotH / (4 * VdotH) (ref. PBRT 2nd Edition p698)
            //
            // Integrate { Li * BRDF * NdotL }
            // F_N = 1/N Sigma^N { Li * BRDF * NdotL / PDF(L)  }
            //     = 1/N Sigma^N { (Li * D * G * F * NdotL) / (D * NdotH / (4 * VdotH)) }
            //     = 1/N Sigma^N { 4.0 * Li * G * F * NdotL * VdotH / NdotH }
            specularLighting += 4.0 * radiance * G * F * NdotL * VdotH / NdotH;
        }
    }

    return specularLighting / float(numSamples);
}

vec3 IBLDiffuseLambertWithSpecularGGX(samplerCube radMap) {
    const int numSamples = 64;

    vec3 diffuseLighting = vec3(0.0);

    vec3 specularLighting = vec3(0.0);

    mat3 tangentToWorld = GetLocalFrame(shading.n);

    // k for IBL = roughness^2 / 2
    float k = shading.linearRoughness * 0.5;

    for (int i = 0; i < numSamples; i++) {
        vec2 xi = Hammersley(i, numSamples);

        vec3 Ld = tangentToWorld * ImportanceSampleLambert(xi);

        vec3 H = tangentToWorld * ImportanceSampleGGX(xi, shading.linearRoughness);

        float VdotH = max(dot(shading.v, H), 0.0);

        vec3 F = F_SchlickSG(shading.specular.rgb, VdotH);

        diffuseLighting += (vec3(1.0) - F) * texCUBE(radMap, Ld).rgb;

        vec3 Ls = 2.0 * dot(shading.v, H) * H - shading.v;

        float NdotL = max(dot(shading.n, Ls), 0.0);

        if (NdotL > 0.0) {
            float NdotH = max(dot(shading.n, H), 0.0);

            float G = G_SchlickGGX(shading.ndotv, NdotL, k);

            specularLighting += 4.0 * texCUBE(radMap, Ls).rgb * G * F * NdotL * VdotH / NdotH;
        }
    }

    diffuseLighting *= shading.diffuse.rgb;

    return (diffuseLighting + specularLighting) / float(numSamples);
}

#endif

// Compute LD term (first sum on Real Shading in Unreal Engine 4 p6)
vec3 IntegrateLD(vec3 N, float linearRoughness, samplerCube radianceCubeMap, float radianceCubeMapSize) {
    mat3 tangentToWorld = GetLocalFrame(N);

    // As we don't know beforehand the view direction when convoluting the environment map,
    // We make a further approximation by assuming the view direction is always equal to the surface normal.
    // So we don't get lengthy reflections at grazing angles.
    vec3 V = N;

    vec3 color = vec3(0.0);

    float totalWeights = 0.0;

    const float inc = 1.0 / 32.0;

#ifdef USE_MIPMAP_FILTERED_SAMPLING
    const float maxMipLevel = log2(radianceCubeMapSize);
    const float sampleCount = 32.0 * 32.0;

    // Solid angle associated to a pixel of the cubemap.
    float invOmegaP = (6.0 * radianceCubeMapSize * radianceCubeMapSize) / (4.0 * PI);
#endif

    for (float y = 0.0; y < 1.0; y += inc) {
        for (float x = 0.0; x < 1.0; x += inc) {
            vec3 H = tangentToWorld * ImportanceSampleGGX(vec2(x, y), linearRoughness);
            vec3 L = 2.0 * dot(V, H) * H - V;

            // Integrate { Li * NdotL }
            float NdotL = max(dot(N, L), 0.0);

            if (NdotL > 0.0) {
#ifdef USE_MIPMAP_FILTERED_SAMPLING
                // Use lower mipmap level for fetching sample with low probability in order to reduce the variance.
                float NdotH = saturate(dot(N, H));

                // N == V and then NdotH == VdotH.
                // PDF(H) = D * NdotH
                // PDF(L) = D * NdotH / (4 * VdotH) 
                //        = D / 4;
                float PDF = D_GGX(NdotH, linearRoughness) / 4.0;

                // Solid angle associated to a sample.
                float omegaS = 1.0 / (sampleCount * PDF);

                float mipLevel = 0.5 * log2(omegaS * invOmegaP) + 1.0;
#else
                float mipLevel = 0.0;
#endif

                color += texCUBElod(radianceCubeMap, vec4(L, mipLevel)).rgb * NdotL;

                // We have found weighting by cos(theta) achieves better results.
                totalWeights += NdotL;
            }
        }
    }

    return color / totalWeights;
}

// Compute DFG term (second sum on Real Shading in Unreal Engine 4 p6)
vec2 IntegrateDFG(float NdotV, float roughness) {
    vec3 N = vec3(0.0, 0.0, 1.0);

    mat3 tangentToWorld = GetLocalFrame(N);

    // theta = cos^-1(NdotV), phi = 0
    vec3 V;
    V.x = sqrt(1.0 - NdotV * NdotV); // sin(theta) cos(phi)
    V.y = 0.0; // sin(theta) sin(phi)
    V.z = NdotV; // cos(theta)

    float A = 0.0;
    float B = 0.0;

    float numSamples = 0.0;

    float linearRoughness = roughness * roughness;

    float k = linearRoughness * 0.5; // k for IBL

    for (float y = 0.0; y < 1.0; y += 0.01) {
        for (float x = 0.0; x < 1.0; x += 0.01) {
            vec3 H = tangentToWorld * ImportanceSampleGGX(vec2(x, y), linearRoughness);
            vec3 L = normalize(2.0 * dot(V, H) * H - V);

            float NdotL = max(L.z, 0.0);

            if (NdotL > 0.0) {
                float NdotH = max(H.z, 0.0);
                float VdotH = max(dot(V, H), 0.0);

                float G = G_SchlickGGX(NdotV, NdotL, k);
                // BRDF/F = D * G (G term is divided by (4 * NdotL * NdotV))
                //
                // PDF(H) = D * NdotH
                // PDF(L) = PDF(H) * dH / dL = D * NdotH / (4 * VdotH) (ref. PBRT 2nd Edition p698)
                //
                // Integrate { BRDF/F * NdotL }
                //
                // F_N = 1/N Sigma^N { BRDF/F * NdotL / PDF(L) }
                //     = 1/N Sigma^N { (D * G * NdotL) / (D * NdotH / (4 * VdotH)) }
                //     = 1/N Sigma^N { 4 * G * NdotL * VdotH / NdotH }
                float G_Vis = 4.0 * G * NdotL * VdotH / NdotH;

                float Fc = pow5(1.0 - VdotH);

                A += (1.0 - Fc) * G_Vis;
                B += Fc * G_Vis;
            }

            numSamples += 1.0;
        }
    }

    return vec2(A, B) / numSamples;
}

#endif
