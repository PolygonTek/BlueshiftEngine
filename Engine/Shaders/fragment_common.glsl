#ifdef LOGLUV_HDR
$include "logluv.glsl"
#endif

#define PI 3.1415926535897932384626433832795
#define TWO_PI 6.283185307179586476925286766559
#define HALF_PI 1.5707963267948966192313216916398
#define INV_PI 0.31830988618379067153776752674503
#define INV_TWO_PI 0.15915494309189533576888376337251

vec3 toLinear(vec3 v) {
    return pow(v, vec3(2.2)); 
}

vec3 toSRGB(vec3 v) { 
    return pow(v, vec3(1.0 / 2.2)); 
}

vec4 tex2D_bilinear(sampler2D tex, in vec2 uv, in float textureSize) {
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

float getMipmapLevel(vec2 uv, vec2 textureSize) {
    vec2 dx = dFdx(uv * textureSize.x);
    vec2 dy = dFdy(uv * textureSize.y);
    float d = max(dot(dx, dx), dot(dy, dy));
    return 0.5 * log2(d);
}

float fresnel(vec3 V, vec3 N, float F0) {
    float kOneMinusVdotN = 1.0 - abs(dot(V, N));    // note: abs() makes 2-sided materials work
    // raise to 5th power
    float result = kOneMinusVdotN * kOneMinusVdotN;
    result = result * result;
    result = result * kOneMinusVdotN;
    result = F0 + (1.0 - F0) * result;

    return max(result, 0.0);
}

vec3 getNormal(sampler2D normalMap, in vec2 tc) {
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

vec2 offsetTexcoord(in float h, in vec2 st, in vec3 viewDir, in float heightScale) {
    return st + h * heightScale * (viewDir.xy / (viewDir.z));
}

vec4 encodeFloatRGBA(float v) {
    vec4 enc = vec4(1.0, 255.0, 65025.0, 160581375.0) * v;
    enc = fract(enc);
    enc -= enc.yzww * vec4(1.0/255.0, 1.0/255.0, 1.0/255.0, 0.0);
    return enc;
}

float decodeFloatRGBA(vec4 rgba) {
    return dot(rgba, vec4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/160581375.0));
}

vec2 encodeViewNormal(vec3 n) {
    vec2 enc = n.xy / (n.z + 1.0);
    enc /= 1.7777;;
    return enc * 0.5 + 0.5;
}

// enc4 is vec4, with .rg containing encoded normal
vec3 decodeViewNormal(vec4 enc4) {
    vec3 n = 1.7777 * (enc4.xyz * vec3(2.0, 2.0, 0.0) - vec3(1.0, 1.0, -1.0));
    float g = 2.0 / dot(n, n);
    n.xy *= g;
    n.z = g - 1.0;
    return n;
}
/*
void RGB2YCoCg(vec4 color, vec3 ycocg) {
    ycocg.x = dot(color, vec4(1.0, -1.0, 0.0, 1.0));
    ycocg.y = dot(color, vec4(0.0, 1.0, -0.5 * 256.0 / 255.0, 1.0));
    ycocg.z = dot(color, vec4(-1.0, -1.0, 1.0 * 256.0 / 255.0, 1.0));
}

void YCoCg2RGB(vec3 ycocg) {
    ycocg.z * (255.0 / 8.0) + 1.0;
}
*/

vec3 faceToGLCubeMapCoords(int faceIndex, int x, int y, int cubeMapSize) {
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

float areaElement(float x, float y) {
    return atan(x * y, sqrt(x * x + y * y + 1.0));
}

float cubeMapTexelSolidAngle(float x, float y, int size) {
    float invSize = 1.0 / float(size);

    float s = (2.0 * (float(x) + 0.5) * invSize) - 1.0;
    float t = (2.0 * (float(y) + 0.5) * invSize) - 1.0;

    // s and t are the -1..1 texture coordinate on the current face.
    // Get projected area for this texel
    float x0 = s - invSize;
    float y0 = t - invSize;
    float x1 = s + invSize;
    float y1 = t + invSize;
    return areaElement(x0, y0) - areaElement(x0, y1) - areaElement(x1, y0) + areaElement(x1, y1);
}

vec3 boxProjectedCubemapDirection(vec3 worldS, vec3 worldPos, vec4 cubemapCenter, vec3 boxMin, vec3 boxMax) {
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
