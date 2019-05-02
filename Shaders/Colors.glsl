#ifndef COLORS_INCLUDED
#define COLORS_INCLUDED

// REC 709 (sRGB) primaries
const mat3 Rec709_2_XYZ = mat3(
    0.4124564, 0.3575761, 0.1804375,
    0.2126729, 0.7151522, 0.0721750,
    0.0193339, 0.1191920, 0.9503041
);

const mat3 XYZ_2_Rec709 = mat3(
    3.2404542,-1.5371385,-0.4985314,
    -0.9692660, 1.8760108, 0.0415560,
    0.0556434,-0.2040259, 1.0572252
);

// REC 2020 primaries
const mat3 XYZ_2_Rec2020 = mat3(
    1.7166084, -0.3556621, -0.2533601,
    -0.6666829, 1.6164776, 0.0157685,
    0.0176422, -0.0427763, 0.94222867
);

const mat3 Rec2020_2_XYZ = mat3(
    0.6369736, 0.1446172, 0.1688585,
    0.2627066, 0.6779996, 0.0592938,
    0.0000000, 0.0280728, 1.0608437
);

// P3, D65 primaries
const mat3 XYZ_2_P3D65 = mat3(
    2.4933963, -0.9313459, -0.4026945,
    -0.8294868, 1.7626597, 0.0236246,
    0.0358507, -0.0761827, 0.9570140
);

const mat3 P3D65_2_XYZ = mat3(
    0.4865906, 0.2656683, 0.1981905,
    0.2289838, 0.6917402, 0.0792762,
    0.0000000, 0.0451135, 1.0438031
);

vec3 GammaToLinear(vec3 sRGB) {
    // Approximate version from http://chilliant.blogspot.com.au/2012/08/srgb-approximations-for-hlsl.html?m=1
    return sRGB * (sRGB * (sRGB * 0.305306011 + 0.682171111) + 0.012522878);
}

vec3 LinearToGamma(vec3 linRGB) {
    linRGB = max(linRGB, vec3(0.0, 0.0, 0.0));
    // An almost-perfect approximation from http://chilliant.blogspot.com.au/2012/08/srgb-approximations-for-hlsl.html?m=1
    return max(1.055 * pow(linRGB, vec3(0.416666667, 0.416666667, 0.416666667)) - 0.055, 0.0);
}

// Compute the luminance of a linear rgb color.
float GetLuma(vec3 color) {
    return dot(color, vec3(0.212655, 0.715158, 0.072187));
}

// Converts a color from linear RGB to XYZ space.
vec3 RGB_to_XYZ(vec3 rgb) {
    return Rec709_2_XYZ * rgb;
}

// Converts a color from XYZ to linear RGB space.
vec3 XYZ_to_RGB(vec3 xyz) {
    return XYZ_2_Rec709 * xyz;
}

// Converts a color from XYZ to xyY space (Y is luminosity).
vec3 XYZ_to_xyY(vec3 xyz) {
    float Y = xyz.y;
    float x = xyz.x / (xyz.x + xyz.y + xyz.z);
    float y = xyz.y / (xyz.x + xyz.y + xyz.z);
    return vec3(x, y, Y);
}

// Converts a color from xyY space to XYZ space.
vec3 xyY_to_XYZ(vec3 xyY) {
    float Y = xyY.z;
    float x = Y * xyY.x / xyY.y;
    float z = Y * (1.0 - xyY.x - xyY.y) / xyY.y;
    return vec3(x, Y, z);
}

// Converts a color from linear RGB to xyY space.
vec3 RGB_to_xyY(vec3 rgb) {
    return XYZ_to_xyY(RGB_to_XYZ(rgb));
}

// Converts a color from xyY space to linear RGB.
vec3 xyY_to_RGB(vec3 xyY) {
    return XYZ_to_RGB(xyY_to_XYZ(xyY));
}

//
// Hue, Saturation, Value
// Ranges:
//  Hue [0.0, 1.0]
//  Sat [0.0, 1.0]
//  Lum [0.0, HALF_MAX]
//
vec3 RGB_to_HSV(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-4;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 HSV_to_RGB(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, saturate(p - K.xxx), c.y);
}

vec3 RGB_to_YUV(const vec3 rgb) {
    vec3 yuv;
    yuv.x = dot(rgb, vec3(0.299, 0.587, 0.114));
    yuv.y = (rgb.z - yuv.x) * 0.565;
    yuv.z = (rgb.x - yuv.x) * 0.713;
    return yuv;
}

vec3 YUV_to_RGB(const vec3 yuv) {
    vec3 rgb;
    rgb.r = yuv.x + 1.403 * yuv.z;
    rgb.g = yuv.x - 0.344 * yuv.y - 1.403 * yuv.z;
    rgb.b = yuv.x + 1.770 * yuv.y;
    return rgb;
}

vec4 RGB_to_CMYK(const vec3 rgb) {
    vec4 cmyk = vec4(vec3(1.0) - rgb, 0.0);
    cmyk.w = min(min(cmyk.x, cmyk.y), cmyk.z);
    cmyk.xyz = (cmyk.xyz - cmyk.w) / (1.0 - cmyk.w);
    return cmyk;
}

vec3 CMYK_to_RGB(const vec4 cmyk) {
    vec3 rgb = min(vec3(1.0), cmyk.xyz * (1.0 - cmyk.w) + vec3(cmyk.w));
    return vec3(1.0) - rgb;
}

//
// RGB / Full-range YCbCr conversions (ITU-R BT.601)
//
vec3 RGB_to_YCbCr(vec3 c) {
    float Y  =  0.299 * c.r + 0.587 * c.g + 0.114 * c.b;
    float Cb = -0.169 * c.r - 0.331 * c.g + 0.500 * c.b;
    float Cr =  0.500 * c.r - 0.419 * c.g - 0.081 * c.b;
    return vec3(Y, Cb, Cr);
}

vec3 YCbCr_to_RGB(vec3 c) {
    float R = c.x + 0.000 * c.y + 1.403 * c.z;
    float G = c.x - 0.344 * c.y - 0.714 * c.z;
    float B = c.x - 1.773 * c.y + 0.000 * c.z;
    return vec3(R, G, B);
}

vec4 EncodeRGBE(vec3 color) {
    float maxChannel = max(max(color.r, color.g), color.b);
    float exponent = ceil(log2(max(maxChannel, 1e-8)));
    vec3 mantissa = clamp(color / exp2(exponent), 0.0, 1.0);
    return vec4(mantissa, (exponent + 128.0) / 256.0);
}

vec3 DecodeRGBE(vec4 v) {
    float exponent = exp2(v.a * 256.0 - 128.0);
    return v.rgb * exponent;
}

//
// LOGLUV ENCODING FOR HDR
// https://mynameismjp.wordpress.com/2008/12/12/logluv-encoding-for-hdr/
//
vec4 EncodeLogLuv(vec3 vRGB) {
    vec3 Xp_Y_XYZp = vRGB * mat3(
        0.2209, 0.3390, 0.4184,
        0.1138, 0.6780, 0.7319,
        0.0102, 0.1130, 0.2969);
    Xp_Y_XYZp = max(Xp_Y_XYZp, vec3(1e-6, 1e-6, 1e-6));
    vec4 vResult;
    vResult.xy = Xp_Y_XYZp.xy / Xp_Y_XYZp.z;
    float Le = 2.0 * log2(Xp_Y_XYZp.y) + 127.0;
    vResult.w = fract(Le);
    vResult.z = (Le - (floor(vResult.w * 255.0)) / 255.0) / 255.0; 
    return vResult;
}

vec3 DecodeLogLuv(vec4 vLogLuv) {
    float Le = vLogLuv.z * 255.0 + vLogLuv.w;
    vec3 Xp_Y_XYZp;
    Xp_Y_XYZp.y = exp2((Le - 127.0) / 2.0);
    Xp_Y_XYZp.z = Xp_Y_XYZp.y / vLogLuv.y;
    Xp_Y_XYZp.x = vLogLuv.x * Xp_Y_XYZp.z;
    vec3 vRGB = Xp_Y_XYZp * mat3(
        6.0013, -2.700, -1.7995,
        -1.332, 3.1029, -5.7720,
        0.3007, -1.088, 5.6268);
    return max(vRGB, vec3(0.0, 0.0, 0.0));
}

vec4 EncodeFloatToRGBA(float v) {
    vec4 enc = vec4(1.0, 255.0, 65025.0, 160581375.0) * v;
    enc = fract(enc);
    enc -= enc.yzww * vec4(1.0/255.0, 1.0/255.0, 1.0/255.0, 0.0);
    return enc;
}

float DecodeFloatToRGBA(vec4 rgba) {
    return dot(rgba, vec4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/160581375.0));
}

#endif
