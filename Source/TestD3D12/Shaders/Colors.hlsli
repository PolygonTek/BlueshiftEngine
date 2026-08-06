#ifndef __COLORS_HLSLI__
#define __COLORS_HLSLI__

// Converts sRGB-encoded values to linear RGB.
inline float3 GammaToLinear(float3 sRGB) {
    return select(sRGB < 0.04045, sRGB / 12.92, pow((sRGB + 0.055) / 1.055, 2.4));
}

// Converts linear RGB values to sRGB encoding.
inline float3 LinearToGamma(float3 linRGB) {
    return select(linRGB < 0.0031308, 12.92 * linRGB, 1.055 * pow(linRGB, 1.0 / 2.4) - 0.055);
}

// Approximate version from http://chilliant.blogspot.com.au/2012/08/srgb-approximations-for-hlsl.html?m=1
inline float3 GammaToLinearApprox(float3 sRGB) {
    return sRGB * (sRGB * (sRGB * 0.305306011 + 0.682171111) + 0.012522878);
}

// Fast sRGB approximation from MiniEngine's ColorSpaceUtility.hlsli.
inline float3 LinearToGammaApprox(float3 linRGB) {
    return select(linRGB < 0.0031308, 12.92 * linRGB, 1.13005 * sqrt(linRGB - 0.00228) - 0.13448 * linRGB + 0.005719);
}

// Compute the luminance of a linear float rgb color.
inline float GetLuma(float3 color) {
    return dot(color, float3(0.212655, 0.715158, 0.072187));
}

// Compute the luminance of a linear half rgb color.
inline half GetLuma(half3 color) {
    return dot(color, half3(0.2127h, 0.7152h, 0.0722h));
}

// Converts a color from linear RGB to XYZ space.
inline float3 LinearRGBToXYZ(float3 rgb) {
    return float3(
        0.4124564 * rgb.r + 0.3575761 * rgb.g + 0.1804375 * rgb.b,
        0.2126729 * rgb.r + 0.7151522 * rgb.g + 0.0721750 * rgb.b,
        0.0193339 * rgb.r + 0.1191920 * rgb.g + 0.9503041 * rgb.b);
}

// Converts a color from XYZ to linear RGB space.
inline float3 XYZToLinearRGB(float3 XYZ) {
    return float3(
        3.2404542 * XYZ.x - 1.5371385 * XYZ.y - 0.4985314 * XYZ.z,
       -0.9692660 * XYZ.x + 1.8760108 * XYZ.y + 0.0415560 * XYZ.z,
        0.0556434 * XYZ.x - 0.2040259 * XYZ.y + 1.0572252 * XYZ.z);
}

// Converts a color from XYZ to xyY space (Y is luminosity).
inline float3 XYZToxyY(float3 xyz) {
    float Y = xyz.y;
    float sum = xyz.x + xyz.y + xyz.z;
    if (sum <= 0.0) {
        return float3(0.0, 0.0, 0.0);
    }

    float sumRcp = rcp(sum);
    float x = xyz.x * sumRcp;
    float y = xyz.y * sumRcp;
    return float3(x, y, Y);
}

// Converts a color from xyY space to XYZ space.
inline float3 xyYToXYZ(float3 xyY) {
    float Y = xyY.z;
    if (xyY.y <= 0.0) {
        return float3(0.0, 0.0, 0.0);
    }

    float scale = Y / xyY.y;
    float x = scale * xyY.x;
    float z = scale * (1.0 - xyY.x - xyY.y);
    return float3(x, Y, z);
}

// Converts a color from linear RGB to xyY space.
inline float3 LinearRGBToxyY(float3 rgb) {
    return XYZToxyY(LinearRGBToXYZ(rgb));
}

// Converts a color from xyY space to linear RGB.
inline float3 xyYToLinearRGB(float3 xyY) {
    return XYZToLinearRGB(xyYToXYZ(xyY));
}

//
// Hue, Saturation, Value
// Ranges:
//  Hue [0.0, 1.0]
//  Sat [0.0, 1.0]
//  Lum [0.0, HALF_MAX]
//
inline float3 RGBToHSV(float3 c) {
    float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    float4 p = lerp(float4(c.bg, K.wz), float4(c.gb, K.xy), step(c.b, c.g));
    float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-4;
    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

inline float3 HSVToRGB(float3 c) {
    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, saturate(p - K.xxx), c.y);
}

//
// RGB / CMYK conversions
// RGB and CMYK components use [0, 1].
//
inline float4 RGBToCMYK(float3 rgb) {
    float4 cmyk = float4(float3(1.0, 1.0, 1.0) - rgb, 0.0);
    cmyk.w = min(min(cmyk.x, cmyk.y), cmyk.z);
    if (cmyk.w >= 1.0) {
        return float4(0.0, 0.0, 0.0, 1.0);
    }

    cmyk.xyz = (cmyk.xyz - cmyk.w) / (1.0 - cmyk.w);
    return cmyk;
}

inline float3 CMYKToRGB(float4 cmyk) {
    float3 rgb = min(float3(1.0, 1.0, 1.0), cmyk.xyz * (1.0 - cmyk.w) + float3(cmyk.w, cmyk.w, cmyk.w));
    return float3(1.0, 1.0, 1.0) - rgb;
}

//
// RGB / Full-range YCbCr conversions (ITU-R BT.601)
// RGB and Y use [0, 1], while Cb and Cr are zero-centered in [-0.5, 0.5].
//
inline float3 RGBToYCbCr(float3 c) {
    float Y  =  0.299000 * c.r + 0.587000 * c.g + 0.114000 * c.b;
    float Cb = -0.168736 * c.r - 0.331264 * c.g + 0.500000 * c.b;
    float Cr =  0.500000 * c.r - 0.418688 * c.g - 0.081312 * c.b;
    return float3(Y, Cb, Cr);
}

inline float3 YCbCrToRGB(float3 c) {
    float R = c.x + 1.402000 * c.z;
    float G = c.x - 0.344136 * c.y - 0.714136 * c.z;
    float B = c.x + 1.772000 * c.y;
    return float3(R, G, B);
}

//
// RGB / YCoCg conversions
// RGB and Y use [0, 1], while Co and Cg are zero-centered in [-0.5, 0.5].
//
inline float3 RGBToYCoCg(float3 color) {
    float Y  =  0.25 * color.r + 0.50 * color.g + 0.25 * color.b;
    float Co =  0.50 * color.r - 0.50 * color.b;
    float Cg = -0.25 * color.r + 0.50 * color.g - 0.25 * color.b;
    return float3(Y, Co, Cg);
}

inline float3 YCoCgToRGB(float3 color) {
    float R = color.x + color.y - color.z;
    float G = color.x + color.z;
    float B = color.x - color.y - color.z;
    return float3(R, G, B);
}

//
// RGBE encoding for HDR
// Non-negative RGB values share a base-2 exponent stored in the alpha channel.
// RGB stores the normalized mantissa, while A stores the exponent biased by 128 and scaled by 1/256.
//
inline float4 EncodeRGBE(float3 color) {
    float maxChannel = max(max(color.r, color.g), color.b);
    float exponent = ceil(log2(max(maxChannel, 1e-8)));
    float3 mantissa = saturate(color / exp2(exponent));
    return float4(mantissa, (exponent + 128.0) / 256.0);
}

inline float3 DecodeRGBE(float4 v) {
    float exponent = exp2(v.a * 256.0 - 128.0);
    return v.rgb * exponent;
}

//
// LOGLUV ENCODING FOR HDR
// https://mynameismjp.wordpress.com/2008/12/12/logluv-encoding-for-hdr/
//
inline float4 EncodeLogLuv(float3 vRGB) {
    float3 Xp_Y_XYZp = mul(vRGB, float3x3(
        0.2209, 0.3390, 0.4184,
        0.1138, 0.6780, 0.7319,
        0.0102, 0.1130, 0.2969));
    Xp_Y_XYZp = max(Xp_Y_XYZp, float3(1e-6, 1e-6, 1e-6));
    float4 vResult;
    vResult.xy = Xp_Y_XYZp.xy / Xp_Y_XYZp.z;
    float Le = 2.0 * log2(Xp_Y_XYZp.y) + 127.0;
    vResult.w = frac(Le);
    vResult.z = (Le - (floor(vResult.w * 255.0)) / 255.0) / 255.0; 
    return vResult;
}

inline float3 DecodeLogLuv(float4 vLogLuv) {
    float Le = vLogLuv.z * 255.0 + vLogLuv.w;
    float3 Xp_Y_XYZp;
    Xp_Y_XYZp.y = exp2((Le - 127.0) / 2.0);
    Xp_Y_XYZp.z = Xp_Y_XYZp.y / vLogLuv.y;
    Xp_Y_XYZp.x = vLogLuv.x * Xp_Y_XYZp.z;
    float3 vRGB = mul(Xp_Y_XYZp, float3x3(
        6.0013, -2.700, -1.7995,
        -1.332, 3.1029, -5.7720,
        0.3007, -1.088, 5.6268));
    return max(vRGB, float3(0.0, 0.0, 0.0));
}

// Encodes a float in the [0, 1) range into four normalized channels.
inline float4 EncodeFloatToRGBA(float v) {
    float4 enc = float4(1.0, 255.0, 65025.0, 16581375.0) * v;
    enc = frac(enc);
    enc -= enc.yzww * float4(1.0/255.0, 1.0/255.0, 1.0/255.0, 0.0);
    return enc;
}

inline float DecodeFloatFromRGBA(float4 rgba) {
    return dot(rgba, float4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/16581375.0));
}

inline float CIE1931Gaussian(float t) {
    return exp(-0.5 * t * t);
}

//
// CIE 1931 2-degree color matching function
// Returns an approximate XYZ response for a wavelength in nanometers, or zero outside [360, 830] nm.
//
inline float3 SampleCIE1931CMF(float wavelengthNm) {
    if (wavelengthNm < 360.0 || wavelengthNm > 830.0) {
        return float3(0.0, 0.0, 0.0);
    }

    float t1 = (wavelengthNm - 442.0) * (wavelengthNm < 442.0 ? 0.0624 : 0.0374);
    float t2 = (wavelengthNm - 599.8) * (wavelengthNm < 599.8 ? 0.0264 : 0.0323);
    float t3 = (wavelengthNm - 501.1) * (wavelengthNm < 501.1 ? 0.0490 : 0.0382);
    float x = 0.362 * CIE1931Gaussian(t1) + 1.056 * CIE1931Gaussian(t2) - 0.065 * CIE1931Gaussian(t3);

    t1 = (wavelengthNm - 568.8) * (wavelengthNm < 568.8 ? 0.0213 : 0.0247);
    t2 = (wavelengthNm - 530.9) * (wavelengthNm < 530.9 ? 0.0613 : 0.0322);
    float y = 0.821 * CIE1931Gaussian(t1) + 0.286 * CIE1931Gaussian(t2);

    t1 = (wavelengthNm - 437.0) * (wavelengthNm < 437.0 ? 0.0845 : 0.0278);
    t2 = (wavelengthNm - 459.0) * (wavelengthNm < 459.0 ? 0.0385 : 0.0725);
    float z = 1.217 * CIE1931Gaussian(t1) + 0.681 * CIE1931Gaussian(t2);

    return float3(x, y, z);
}

//
// HDR10 color space conversions
// Rec.709 / Rec.2020 conversions assume linear RGB values.
// ST.2084 (PQ) uses normalized absolute luminance, where 1.0 represents 10,000 nits.
//
inline float3 ApplyREC2084Curve(float3 L) {
    const float m1 = 2610.0 / 4096.0 / 4.0;
    const float m2 = 2523.0 / 4096.0 * 128.0;
    const float c1 = 3424.0 / 4096.0;
    const float c2 = 2413.0 / 4096.0 * 32.0;
    const float c3 = 2392.0 / 4096.0 * 32.0;
    float3 Lp = pow(L, m1);
    return pow((c1 + c2 * Lp) / (1.0 + c3 * Lp), m2);
}

inline float3 RemoveREC2084Curve(float3 N) {
    const float m1 = 2610.0 / 4096.0 / 4.0;
    const float m2 = 2523.0 / 4096.0 * 128.0;
    const float c1 = 3424.0 / 4096.0;
    const float c2 = 2413.0 / 4096.0 * 32.0;
    const float c3 = 2392.0 / 4096.0 * 32.0;
    float3 Np = pow(N, 1.0 / m2);
    return pow(max(Np - c1, 0.0) / (c2 - c3 * Np), 1.0 / m1);
}

inline float3 REC709toREC2020(float3 RGB709) {
    static const float3x3 conversionMatrix = {
        0.627402, 0.329292, 0.043306,
        0.069095, 0.919544, 0.011360,
        0.016394, 0.088028, 0.895578
    };
    return mul(conversionMatrix, RGB709);
}

inline float3 REC2020toREC709(float3 RGB2020) {
    static const float3x3 conversionMatrix = {
         1.660496, -0.587656, -0.072840,
        -0.124547,  1.132895, -0.008348,
        -0.018154, -0.100597,  1.118751
    };
    return mul(conversionMatrix, RGB2020);
}

#endif // __COLORS_HLSLI__
