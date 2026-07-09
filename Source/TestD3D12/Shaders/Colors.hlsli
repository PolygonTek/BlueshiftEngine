#ifndef __COLORS_HLSLI__
#define __COLORS_HLSLI__

float3 GammaToLinear(float3 sRGB) {
    // Approximate version from http://chilliant.blogspot.com.au/2012/08/srgb-approximations-for-hlsl.html?m=1
    return sRGB * (sRGB * (sRGB * 0.305306011 + 0.682171111) + 0.012522878);
}

float3 LinearToGamma(float3 linRGB) {
    linRGB = max(linRGB, float3(0.0, 0.0, 0.0));
    // An almost-perfect approximation from http://chilliant.blogspot.com.au/2012/08/srgb-approximations-for-hlsl.html?m=1
    return max(1.055 * pow(linRGB, float3(0.416666667, 0.416666667, 0.416666667)) - 0.055, float3(0.0, 0.0, 0.0));
}

// Compute the luminance of a linear rgb color.
float GetLuma(float3 color) {
    return dot(color, float3(0.212655, 0.715158, 0.072187));
}

// Converts a color from linear RGB to XYZ space.
float3 LinearRGBToXYZ(float3 rgb) {
    return float3(
        0.4124564 * rgb.r + 0.3575761 * rgb.g + 0.1804375 * rgb.b,
        0.2126729 * rgb.r + 0.7151522 * rgb.g + 0.0721750 * rgb.b,
        0.0193339 * rgb.r + 0.1191920 * rgb.g + 0.9503041 * rgb.b);
}

// Converts a color from XYZ to linear RGB space.
float3 XYZToLinearRGB(float3 XYZ) {
    return float3(
        3.2404542 * XYZ.x - 1.5371385 * XYZ.y - 0.4985314 * XYZ.z,
       -0.9692660 * XYZ.x + 1.8760108 * XYZ.y + 0.0415560 * XYZ.z,
        0.0556434 * XYZ.x - 0.2040259 * XYZ.y + 1.0572252 * XYZ.z);
}

// Converts a color from XYZ to xyY space (Y is luminosity).
float3 XYZToxyY(float3 xyz) {
    float Y = xyz.y;
    float x = xyz.x / (xyz.x + xyz.y + xyz.z);
    float y = xyz.y / (xyz.x + xyz.y + xyz.z);
    return float3(x, y, Y);
}

// Converts a color from xyY space to XYZ space.
float3 xyYToXYZ(float3 xyY) {
    float Y = xyY.z;
    float x = Y * xyY.x / xyY.y;
    float z = Y * (1.0 - xyY.x - xyY.y) / xyY.y;
    return float3(x, Y, z);
}

// Converts a color from linear RGB to xyY space.
float3 LinearRGBToxyY(float3 rgb) {
    return XYZToxyY(LinearRGBToXYZ(rgb));
}

// Converts a color from xyY space to linear RGB.
float3 xyYToLinearRGB(float3 xyY) {
    return XYZToLinearRGB(xyYToXYZ(xyY));
}

//
// Hue, Saturation, Value
// Ranges:
//  Hue [0.0, 1.0]
//  Sat [0.0, 1.0]
//  Lum [0.0, HALF_MAX]
//
float3 RGBToHSV(float3 c) {
    float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    float4 p = lerp(float4(c.bg, K.wz), float4(c.gb, K.xy), step(c.b, c.g));
    float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-4;
    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

float3 HSVToRGB(float3 c) {
    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, saturate(p - K.xxx), c.y);
}

float3 RGBToYUV(float3 rgb) {
    float3 yuv;
    yuv.x = dot(rgb, float3(0.299, 0.587, 0.114));
    yuv.y = (rgb.z - yuv.x) * 0.565;
    yuv.z = (rgb.x - yuv.x) * 0.713;
    return yuv;
}

float3 YUVToRGB(float3 yuv) {
    float3 rgb;
    rgb.r = yuv.x + 1.403 * yuv.z;
    rgb.g = yuv.x - 0.344 * yuv.y - 1.403 * yuv.z;
    rgb.b = yuv.x + 1.770 * yuv.y;
    return rgb;
}

float4 RGBToCMYK(float3 rgb) {
    float4 cmyk = float4(float3(1.0, 1.0, 1.0) - rgb, 0.0);
    cmyk.w = min(min(cmyk.x, cmyk.y), cmyk.z);
    cmyk.xyz = (cmyk.xyz - cmyk.w) / (1.0 - cmyk.w);
    return cmyk;
}

float3 CMYKToRGB(float4 cmyk) {
    float3 rgb = min(float3(1.0, 1.0, 1.0), cmyk.xyz * (1.0 - cmyk.w) + float3(cmyk.w, cmyk.w, cmyk.w));
    return float3(1.0, 1.0, 1.0) - rgb;
}

//
// RGB / Full-range YCbCr conversions (ITU-R BT.601)
//
float3 RGBToYCbCr(float3 c) {
    float Y  =  0.299 * c.r + 0.587 * c.g + 0.114 * c.b;
    float Cb = -0.169 * c.r - 0.331 * c.g + 0.500 * c.b;
    float Cr =  0.500 * c.r - 0.419 * c.g - 0.081 * c.b;
    return float3(Y, Cb, Cr);
}

float3 YCbCrToRGB(float3 c) {
    float R = c.x + 0.000 * c.y + 1.403 * c.z;
    float G = c.x - 0.344 * c.y - 0.714 * c.z;
    float B = c.x - 1.773 * c.y + 0.000 * c.z;
    return float3(R, G, B);
}

float3 RGBToYCoCg(float3 color) {
    float Y  =  0.25 * color.r + 0.50 * color.g + 0.25 * color.b;
    float Co =  0.50 * color.r + 0.00 * color.g - 0.50 * color.b;
    float Cg = -0.25 * color.r + 0.50 * color.g - 0.25 * color.b;
    return float3(Y, Co, Cg);
}

float3 YCoCgToRGB(float3 color) {
    float R = color.x + color.y - color.z;
    float G = color.x + color.z;
    float B = color.x - color.y - color.z;
    return float3(R, G, B);
}

float4 EncodeRGBE(float3 color) {
    float maxChannel = max(max(color.r, color.g), color.b);
    float exponent = ceil(log2(max(maxChannel, 1e-8)));
    float3 mantissa = saturate(color / exp2(exponent));
    return float4(mantissa, (exponent + 128.0) / 256.0);
}

float3 DecodeRGBE(float4 v) {
    float exponent = exp2(v.a * 256.0 - 128.0);
    return v.rgb * exponent;
}

//
// LOGLUV ENCODING FOR HDR
// https://mynameismjp.wordpress.com/2008/12/12/logluv-encoding-for-hdr/
//
float4 EncodeLogLuv(float3 vRGB) {
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

float3 DecodeLogLuv(float4 vLogLuv) {
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

float4 EncodeFloatToRGBA(float v) {
    float4 enc = float4(1.0, 255.0, 65025.0, 160581375.0) * v;
    enc = frac(enc);
    enc -= enc.yzww * float4(1.0/255.0, 1.0/255.0, 1.0/255.0, 0.0);
    return enc;
}

float DecodeFloatToRGBA(float4 rgba) {
    return dot(rgba, float4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/160581375.0));
}

float CIE1931Gaussian(float t) {
    return exp(-0.5 * t * t);
}

float3 SampleCIE1931CMF(float wavelengthNm) {
    if (wavelengthNm < 360.0 || wavelengthNm > 830.0)
    {
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

#endif
