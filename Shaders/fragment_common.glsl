#ifndef FRAGMENT_COMMON_INCLUDED
#define FRAGMENT_COMMON_INCLUDED

vec4 Tex2Dbilinear(sampler2D tex, in vec2 uv, in float textureSize) {
    float texelSize = 1.0 / textureSize;
    vec2 f = fract(uv.xy * textureSize);
    vec4 t00 = tex2D(tex, uv);
    vec4 t10 = tex2D(tex, uv + vec2(texelSize, 0.0));
    vec4 tA = mix(t00, t10, f.x);
    vec4 t01 = tex2D(tex, uv + vec2(0.0, texelSize));
    vec4 t11 = tex2D(tex, uv + vec2(texelSize, texelSize));
    vec4 tB = mix(t01, t11, f.x);
    return mix(tA, tB, f.y);
}

float GetMipmapLevel(vec2 uv, vec2 textureSize) {
    vec2 dx = dFdx(uv * textureSize.x);
    vec2 dy = dFdy(uv * textureSize.y);
    float d = max(dot(dx, dx), dot(dy, dy));
    return 0.5 * log2(d);
}

vec3 GetNormal(sampler2D normalMap, in vec2 tc) {
#if defined(LATC_NORMAL)
    vec3 n = tex2D(normalMap, tc).xyy * 2.0 - 1.0;
    n.z = 0.0;
    n.z = sqrt(abs(1.0 - dot(n, n)));
#elif defined(ETC2_NORMAL)
    vec3 n = tex2D(normalMap, tc).xyy;
    n.z = 0.0;
    n.z = sqrt(abs(1.0 - dot(n, n)));
#elif defined(DXT5_XGBR_NORMAL)
    vec3 n = tex2D(normalMap, tc).wyz * 2.0 - 1.0;
#else
    vec3 n = tex2D(normalMap, tc).xyz * 2.0 - 1.0;
#endif
    return n;
}

vec2 EncodeViewNormal(vec3 n) {
    vec2 enc = n.xy / (n.z + 1.0);
    enc /= 1.7777;;
    return enc * 0.5 + 0.5;
}

// enc4 is vec4, with .rg containing encoded normal
vec3 DecodeViewNormal(vec4 enc4) {
    vec3 n = 1.7777 * (enc4.xyz * vec3(2.0, 2.0, 0.0) - vec3(1.0, 1.0, -1.0));
    float g = 2.0 / dot(n, n);
    n.xy *= g;
    n.z = g - 1.0;
    return n;
}

vec3 FaceToGLCubeMapCoords(int faceIndex, int x, int y, int cubeMapSize) {
    float invSize = 1.0 / float(cubeMapSize);

    float sc = (2.0 * (float(x) + 0.5) * invSize) - 1.0;
    float tc = (2.0 * (float(y) + 0.5) * invSize) - 1.0;
        
    vec3 vec;
    
    if (faceIndex == 0) {
        vec = vec3(1.0, -tc, -sc);
    } else if (faceIndex == 1) {
        vec = vec3(-1.0, -tc, sc);
    } else if (faceIndex == 2) {
        vec = vec3(sc, 1.0, tc);
    } else if (faceIndex == 3) {
        vec = vec3(sc, -1.0, -tc);
    } else if (faceIndex == 4) {
        vec = vec3(sc, -tc, 1.0);
    } else {
        vec = vec3(-sc, -tc, -1.0);
    }
    
    vec = normalize(vec);
    return vec;
}

float AreaElement(float x, float y) {
    return atan(x * y, sqrt(x * x + y * y + 1.0));
}

float CubeMapTexelSolidAngle(float x, float y, int size) {
    float invSize = 1.0 / float(size);

    float s = (2.0 * (float(x) + 0.5) * invSize) - 1.0;
    float t = (2.0 * (float(y) + 0.5) * invSize) - 1.0;

    // s and t are the -1..1 texture coordinate on the current face.
    // Get projected area for this texel
    float x0 = s - invSize;
    float y0 = t - invSize;
    float x1 = s + invSize;
    float y1 = t + invSize;
    return AreaElement(x0, y0) - AreaElement(x0, y1) - AreaElement(x1, y0) + AreaElement(x1, y1);
}

vec3 BoxProjectedCubemapDirection(vec3 worldS, vec3 worldPos, vec4 cubemapCenter, vec3 boxMin, vec3 boxMax) {
    if (cubemapCenter.w > 0.0) {
        worldS = normalize(worldS);

        vec3 rbmax = (boxMax - worldPos) / worldS;
        vec3 rbmin = (boxMin - worldPos) / worldS;
        vec3 rbminmax = mix(rbmin, rbmax, greaterThan(worldS, vec3(0.0)));

        float fa = min(min(rbminmax.x, rbminmax.y), rbminmax.z);

        worldPos -= cubemapCenter.xyz;
        worldS = worldPos + worldS * fa;
    }

    return worldS;
}

#endif
