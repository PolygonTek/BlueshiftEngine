in vec4 v2f_shadowVec;

const vec4 poissonDisk4[2] = vec4[](
    vec4(-0.94201624, -0.39906216, +0.94558609, -0.76890725),
    vec4(-0.09418410, -0.92938870, +0.34495938, +0.29387760)
);

const vec4 poissonDisk12[6] = vec4[](
    vec4(-0.326212, -0.405805, -0.840144, -0.073580),
    vec4(-0.695914, +0.457137, -0.203345, +0.620716),
    vec4(+0.962340, -0.194983, +0.473434, -0.480026),
    vec4(+0.519456, +0.767022, +0.185461, -0.893124),
    vec4(+0.507431, +0.064425, +0.896420, +0.412458),
    vec4(-0.321940, -0.932615, -0.791559, -0.597705)
);

const vec4 poissonDisk16[8] = vec4[](
    vec4(-0.94201624, -0.39906216, +0.94558609, -0.76890725),
    vec4(-0.09418410, -0.92938870, +0.34495938, +0.29387760),
    vec4(-0.91588581, +0.45771432, -0.81544232, -0.87912464),
    vec4(-0.38277543, +0.27676845, +0.97484398, +0.75648379),
    vec4(+0.44323325, -0.97511554, +0.53742981, -0.47373420),
    vec4(-0.26496911, -0.41893023, +0.79197514, +0.19090188),
    vec4(-0.24188840, +0.99706507, -0.81409955, +0.91437590),
    vec4(+0.19984126, +0.78641367, +0.14383161, -0.14100790)
);

uniform sampler2DShadow shadowMap;

uniform sampler2DArrayShadow shadowArrayMap;
uniform float shadowMapFilterSize[8];
uniform float cascadeBlendSize;

uniform sampler2D randomRotMatMap;
uniform vec4 shadowSplitFar;
uniform vec2 shadowMapTexelSize;

//-------------------------------------------------------------------------------------------------

uniform mat4 shadowCascadeProjMatrix[CSM_COUNT];
const vec3 cascadeDebugColor[8] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(1.0, 1.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 1.0, 1.0),
    vec3(1.0, 0.0, 0.0),
    vec3(1.0, 1.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 1.0, 1.0)
);

vec3 SampleShadowArrayPCF_Q3(vec4 shadowTC, vec2 filterOffset) {
    const float kernelRadius = 1.0;
    //vec2 jitter_texcoord = gl_FragCoord.xy / 64.0;
    vec2 jitter_texcoord = shadowTC.xy * 256.0;
    vec4 rotMat = tex2D(randomRotMatMap, jitter_texcoord).xyzw * 2.0 - 1.0;
    //rotSample.xy = normalize(rotMat.xy);
    rotMat *= kernelRadius;

    vec3 shadow = vec3(0.0);

    for (int i = 0; i < 8; i++) {
        vec4 kernel = poissonDisk16[i];
        vec4 rotated = rotMat.xyzw * kernel.xxww + rotMat.zwxy * kernel.yyzz;
        shadow += tex2Darray(shadowArrayMap, vec4(shadowTC.xy + filterOffset * rotated.xy, shadowTC.zw)).rgb;
        shadow += tex2Darray(shadowArrayMap, vec4(shadowTC.xy + filterOffset * rotated.zw, shadowTC.zw)).rgb;
    }

    shadow *= 0.0625;
    return shadow;
}

vec3 SampleShadowArrayPCF_Q2(vec4 shadowTC, vec2 filterOffset) {
    // 9 sample
    vec3 shadow = tex2Darray(shadowArrayMap, shadowTC).rgb;
    shadow += tex2Darray(shadowArrayMap, shadowTC + vec4(0, filterOffset.y, 0, 0)).rgb;
    shadow += tex2Darray(shadowArrayMap, shadowTC + vec4(0, -filterOffset.y, 0, 0)).rgb;
    shadow += tex2Darray(shadowArrayMap, shadowTC + vec4(filterOffset.x, 0, 0, 0)).rgb;
    shadow += tex2Darray(shadowArrayMap, shadowTC + vec4(-filterOffset.x, 0, 0, 0)).rgb;
    shadow += tex2Darray(shadowArrayMap, shadowTC + vec4(filterOffset.x, filterOffset.y, 0, 0)).rgb;
    shadow += tex2Darray(shadowArrayMap, shadowTC + vec4(filterOffset.x, -filterOffset.y, 0, 0)).rgb;
    shadow += tex2Darray(shadowArrayMap, shadowTC + vec4(-filterOffset.x, filterOffset.y, 0, 0)).rgb;
    shadow += tex2Darray(shadowArrayMap, shadowTC + vec4(-filterOffset.x, -filterOffset.y, 0, 0)).rgb;
    shadow /= 9.0;
    return shadow;
}

vec3 SampleShadowArrayPCF_Q1(vec4 shadowTC, vec2 filterOffset) {
    // 5 sample
    vec3 shadow = tex2Darray(shadowArrayMap, shadowTC).rgb;
    shadow += tex2Darray(shadowArrayMap, shadowTC + vec4(0, filterOffset.y, 0, 0)).rgb;
    shadow += tex2Darray(shadowArrayMap, shadowTC + vec4(0, -filterOffset.y, 0, 0)).rgb;
    shadow += tex2Darray(shadowArrayMap, shadowTC + vec4(filterOffset.x, 0, 0, 0)).rgb;
    shadow += tex2Darray(shadowArrayMap, shadowTC + vec4(-filterOffset.x, 0, 0, 0)).rgb;
    shadow *= 0.2;
    return shadow;
}

vec3 SampleShadowArray(vec4 shadowTC, vec2 filterOffset) {
#if SHADOW_MAP_QUALITY == 3
    return SampleShadowArrayPCF_Q3(shadowTC, filterOffset);
#elif SHADOW_MAP_QUALITY == 2
    return SampleShadowArrayPCF_Q2(shadowTC, shadowMapTexelSize);
#elif SHADOW_MAP_QUALITY == 1
    return SampleShadowArrayPCF_Q1(shadowTC, shadowMapTexelSize);
#else
    return tex2Darray(shadowArrayMap, shadowTC).rgb;
#endif
}

vec3 SampleSingleCascadedShadowMap() {
    vec4 shadowCascadedTC;
    shadowCascadedTC = shadowCascadeProjMatrix[0] * v2f_shadowVec;
    shadowCascadedTC = shadowCascadedTC.xyzz / shadowCascadedTC.w;
    shadowCascadedTC.z = 0.0;
    vec3 s1 = SampleShadowArray(shadowCascadedTC, shadowMapTexelSize * shadowMapFilterSize[0]);

#ifdef DEBUG_CASCADE_SHADOW_MAP
    s1 *= cascadeDebugColor[0];
#endif

    return s1;
}

vec3 SampleCascadedShadowMap() {
    vec4 shadowCascadedTC;
    int cascadeIndex = CSM_COUNT;

#if CASCADE_SELECTION_METHOD == 0
    // z-based selection
    vec3 comp = vec3(greaterThan(gl_FragCoord.zzz, shadowSplitFar.xyz));
    cascadeIndex = int(dot(comp, vec3(1.0, 1.0, 1.0)));
    
    shadowCascadedTC = shadowCascadeProjMatrix[cascadeIndex] * v2f_shadowVec;
    shadowCascadedTC = shadowCascadedTC.xyzz / shadowCascadedTC.w;
#elif CASCADE_SELECTION_METHOD == 1
    // map-based selection
    vec4 borderTC = shadowMapTexelSize.xyxy;
    #if SHADOW_MAP_QUALITY == 3
        borderTC *= shadowMapFilterSize[cascadeIndex];
    #endif
    borderTC.zw = vec2(1.0) - borderTC.zw;

    for (int i = 0; i < CSM_COUNT; i++) {
        shadowCascadedTC = shadowCascadeProjMatrix[i] * v2f_shadowVec;
        shadowCascadedTC = shadowCascadedTC.xyzz / shadowCascadedTC.w;

        if (all(greaterThan(shadowCascadedTC.xy, borderTC.xy)) && all(lessThan(shadowCascadedTC.xy, borderTC.zw))) {
        /*if (shadowCascadedTC.x > borderTC.x &&
            shadowCascadedTC.y > borderTC.y &&
            shadowCascadedTC.x < borderTC.z &&
            shadowCascadedTC.y < borderTC.w) {*/
            cascadeIndex = i;
            break;
        }
    }
#endif
    shadowCascadedTC.z = float(cascadeIndex);
    vec3 s1 = SampleShadowArray(shadowCascadedTC, shadowMapTexelSize * shadowMapFilterSize[cascadeIndex]);

#ifdef DEBUG_CASCADE_SHADOW_MAP
    s1 *= cascadeDebugColor[cascadeIndex];
#endif

#ifdef BLEND_CASCADE
    float blendBand = min(min(shadowCascadedTC.x, shadowCascadedTC.y), min(1.0 - shadowCascadedTC.x, 1.0 - shadowCascadedTC.y));

    if (blendBand < cascadeBlendSize) {
        float t = 1.0 - min(blendBand / cascadeBlendSize, 1.0);

        vec3 s2 = vec3(1.0);
        int cascadeIndex2 = cascadeIndex + 1;
        if (cascadeIndex2 < CSM_COUNT) {
            vec4 shadowCascadedTC2 = shadowCascadeProjMatrix[cascadeIndex2] * v2f_shadowVec;
            shadowCascadedTC2 = shadowCascadedTC2.xyzz / shadowCascadedTC2.w;
            shadowCascadedTC2.z = float(cascadeIndex2);
            s2 = SampleShadowArray(shadowCascadedTC2, shadowMapTexelSize * shadowMapFilterSize[cascadeIndex2]);

        #ifdef DEBUG_CASCADE_SHADOW_MAP
            s2 *= cascadeDebugColor[cascadeIndex2];
        #endif
        }

        s1 = mix(s1, s2, t);
    }
#endif // BLEND_CASCADE

    return s1;
}

//-------------------------------------------------------------------------------------------------

// for ATI
vec3 shadow2D4(in sampler2DShadow tex, in vec3 texcoord, in float width, in float height) {
    float alpha = fract(texcoord.x * width - 0.5);
    float beta = fract(texcoord.y * height - 0.5);

    vec3 st_00 = vec3(texcoord.x - 0.5/width, texcoord.y - 0.5/height, texcoord.z);
    vec3 st_10 = vec3(texcoord.x + 0.5/width, texcoord.y - 0.5/height, texcoord.z);
    vec3 st_01 = vec3(texcoord.x - 0.5/width, texcoord.y + 0.5/height, texcoord.z);
    vec3 st_11 = vec3(texcoord.x + 0.5/width, texcoord.y + 0.5/height, texcoord.z);

    vec3 s00 = (1.0 - alpha) * (1.0 - beta) * tex2D(tex, st_00).rgb;
    vec3 s10 = alpha * (1.0 - beta) * tex2D(tex, st_10).rgb;
    vec3 s01 = (1.0 - alpha)*beta * tex2D(tex, st_01).rgb;
    vec3 s11 = alpha * beta * tex2D(tex, st_11).rgb;

    return s00 + s10 + s01 + s11;
}

vec3 SampleShadowPCF_Q3(vec4 shadowTC, vec2 filterOffset) {
    const float kernelRadius = 1.0;
    //vec2 jitter_texcoord = gl_FragCoord.xy / 64.0;
    vec2 jitter_texcoord = shadowTC.xy * 512.0;
    vec4 rotMat = tex2D(randomRotMatMap, jitter_texcoord).xyzw * 2.0 - 1.0;
    //rotSample.xy = normalize(rotMat.xy);
    rotMat *= kernelRadius;

    vec3 shadow = vec3(0.0);

    for (int i = 0; i < 8; i++) {
        vec4 kernel = poissonDisk16[i];
        vec4 rotated = rotMat.xyzw * kernel.xxww + rotMat.zwxy * kernel.yyzz;
        shadow += tex2D(shadowMap, vec3(shadowTC.xy + filterOffset * rotated.xy, shadowTC.z)).rgb;
        shadow += tex2D(shadowMap, vec3(shadowTC.xy + filterOffset * rotated.zw, shadowTC.z)).rgb;
    }

    shadow *= 0.0625;
    return shadow;
}

vec3 SampleShadowPCF_Q2(vec4 shadowTC, vec2 filterOffset) {
    // 9 sample
    vec3 shadow = tex2D(shadowMap, shadowTC.xyz).rgb;
    shadow += tex2D(shadowMap, vec3(shadowTC.x, shadowTC.y + filterOffset.y, shadowTC.z)).rgb;
    shadow += tex2D(shadowMap, vec3(shadowTC.x, shadowTC.y - filterOffset.y, shadowTC.z)).rgb;
    shadow += tex2D(shadowMap, vec3(shadowTC.x + filterOffset.x, shadowTC.y, shadowTC.z)).rgb;
    shadow += tex2D(shadowMap, vec3(shadowTC.x - filterOffset.x, shadowTC.y, shadowTC.z)).rgb;
    shadow += tex2D(shadowMap, vec3(shadowTC.x + filterOffset.x, shadowTC.y + filterOffset.y, shadowTC.z)).rgb;
    shadow += tex2D(shadowMap, vec3(shadowTC.x + filterOffset.x, shadowTC.y - filterOffset.y, shadowTC.z)).rgb;
    shadow += tex2D(shadowMap, vec3(shadowTC.x - filterOffset.x, shadowTC.y + filterOffset.y, shadowTC.z)).rgb;
    shadow += tex2D(shadowMap, vec3(shadowTC.x - filterOffset.x, shadowTC.y - filterOffset.y, shadowTC.z)).rgb;
    shadow /= 9.0;
    return shadow;
}

vec3 SampleShadowPCF_Q1(vec4 shadowTC, vec2 filterOffset) {
    // 5 sample
    vec3 shadow = tex2D(shadowMap, shadowTC.xyz).rgb;
    shadow += tex2D(shadowMap, vec3(shadowTC.x, shadowTC.y + filterOffset.y, shadowTC.z)).rgb;
    shadow += tex2D(shadowMap, vec3(shadowTC.x, shadowTC.y - filterOffset.y, shadowTC.z)).rgb;
    shadow += tex2D(shadowMap, vec3(shadowTC.x + filterOffset.x, shadowTC.y, shadowTC.z)).rgb;
    shadow += tex2D(shadowMap, vec3(shadowTC.x - filterOffset.x, shadowTC.y, shadowTC.z)).rgb;
    shadow *= 0.2;
    return shadow;
}

vec3 SampleSpotShadowMap() {
    vec4 shadowTC = v2f_shadowVec.xyzz / v2f_shadowVec.w;
    shadowTC.z = 0.0;
#if SHADOW_MAP_QUALITY >= 1
    return SampleShadowArrayPCF_Q1(shadowTC, shadowMapTexelSize);
#else
    return tex2Darray(shadowArrayMap, shadowTC).rgb;
#endif
}

//-------------------------------------------------------------------------------------------------

uniform samplerCube cubicNormalCubeMap;
uniform samplerCube indirectionCubeMap;
uniform vec2 shadowProjectionDepth;
uniform float vscmBiasedScale;

/*vec3 ShadowCubePCF(vec4 shadowTC) {
#if SHADOW_MAP_QUALITY > 0
    float ma = max(abs(shadowTC.x), max(abs(shadowTC.y), abs(shadowTC.z)));
    vec4 projCoord = vec4((shadowTC.xyz / ma) * shadowMapSize, shadowTC.w);

    vec4 shadowCoord[8];
    shadowCoord[0] = projCoord;
    shadowCoord[1] = projCoord + vec4(0, 0, 1, 0);
    shadowCoord[2] = projCoord + vec4(0, 1, 0, 0);
    shadowCoord[3] = projCoord + vec4(0, 1, 1, 0);
    shadowCoord[4] = projCoord + vec4(1, 0, 0, 0);
    shadowCoord[5] = projCoord + vec4(1, 0, 1, 0);
    shadowCoord[6] = projCoord + vec4(1, 1, 0, 0);
    shadowCoord[7] = projCoord + vec4(1, 1, 1, 0);

    return (shadowCube(shadowCubeMap, shadowCoord[0].xyzw).rgb +
            shadowCube(shadowCubeMap, shadowCoord[1].xyzw).rgb +
            shadowCube(shadowCubeMap, shadowCoord[2].xyzw).rgb +
            shadowCube(shadowCubeMap, shadowCoord[3].xyzw).rgb +
            shadowCube(shadowCubeMap, shadowCoord[4].xyzw).rgb +
            shadowCube(shadowCubeMap, shadowCoord[5].xyzw).rgb +
            shadowCube(shadowCubeMap, shadowCoord[6].xyzw).rgb +
            shadowCube(shadowCubeMap, shadowCoord[7].xyzw).rgb) * 0.125;
#else
    return shadowCube(shadowCubeMap, shadowTC.xyzw).rgb;
#endif
}*/

// ShaderX3 - 5.4 Efficient Omnidirectional Shadow Maps
vec4 GetShadowIndirectCoord(const vec3 worldLightVec) {
    // Get the world cubic face normal
    vec3 faceNormal = texCUBE(cubicNormalCubeMap, worldLightVec).xyz;

    // Zeye in the face frustum
    //float Zeye = max(abs(worldLightVec.x), max(abs(worldLightVec.y), abs(worldLightVec.z)));
    float Zeye = dot(worldLightVec, faceNormal);
    
    // light direction in near distance 1.0
    vec3 dir = worldLightVec / Zeye; 

    // VSCM rendered with biased fov so that we should scale direction vector 
    /*vec3 biasedDir;
    biasedDir.x = faceNormal.x == 0.0 ? dir.x * vscmBiasedScale : dir.x;
    biasedDir.y = faceNormal.y == 0.0 ? dir.y * vscmBiasedScale : dir.y;
    biasedDir.z = faceNormal.z == 0.0 ? dir.z * vscmBiasedScale : dir.z;*/
    vec3 a = abs(faceNormal);
    vec3 b = vscmBiasedScale * (vec3(1.0, 1.0, 1.0) - a);
    vec3 biasedDir = a * dir + b * dir;
    
    vec4 shadowIndirectCoord;
    shadowIndirectCoord.xy = texCUBE(indirectionCubeMap, biasedDir).zw; // indirectionCubeMap has LA_16_16 format 
    shadowIndirectCoord.z = (1.0 / Zeye) * shadowProjectionDepth.x + shadowProjectionDepth.y;
    shadowIndirectCoord.w = 1.0;

    return shadowIndirectCoord;
}

vec3 SamplePointShadowMap() {
    vec4 shadowTC = GetShadowIndirectCoord(v2f_shadowVec.xyz);
#if SHADOW_MAP_QUALITY == 3
    return SampleShadowPCF_Q3(shadowTC, shadowMapTexelSize);
#elif SHADOW_MAP_QUALITY == 2
    return SampleShadowPCF_Q2(shadowTC, shadowMapTexelSize);
#elif SHADOW_MAP_QUALITY ==	1
    return SampleShadowPCF_Q1(shadowTC, shadowMapTexelSize);
#else
    return tex2D(shadowMap, shadowTC.xyz).rgb;
#endif
}

//-------------------------------------------------------------------------------------------------

vec3 ShadowFunc() {
#if defined(USE_SHADOW_SPOT)
    return SampleSpotShadowMap();
#elif defined(USE_SHADOW_POINT)
    return SamplePointShadowMap();
#elif defined(USE_SHADOW_CASCADE)
    #if CSM_COUNT > 1
        return SampleCascadedShadowMap();
    #else
        return SampleSingleCascadedShadowMap();
    #endif
#endif
    return vec3(1.0);
}
