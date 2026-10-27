#ifndef __TONE_MAP_OPERATORS_HLSLI__
#define __TONE_MAP_OPERATORS_HLSLI__

#include "../ShaderInterop.h"
#include "../Colors.hlsli"
#include "AgX.hlsli"

static const float ToneMapWhiteLevel = 5.0;
static const float ToneMapLuminanceSaturation = 1.0;

float3 ToneMapApplyLuminance(float3 color, float pixelLuminance, float toneMappedLuminance) {
    return toneMappedLuminance * pow(color / pixelLuminance, float3(ToneMapLuminanceSaturation, ToneMapLuminanceSaturation, ToneMapLuminanceSaturation));
}

// Linear tone mapping operator
float3 ToneMapLinear(float3 color) {
    return color;
}

// Exponential tone mapping operator
float3 ToneMapExponential(float3 color) {
    float pixelLuminance = max(GetLuma(color), 0.0001);
    float toneMappedLuminance = 1.0 - exp(-pixelLuminance / ToneMapWhiteLevel);
    return ToneMapApplyLuminance(color, pixelLuminance, toneMappedLuminance);
}

// Logarithmic mapping operator
float3 ToneMapLogarithmic(float3 color) {
    float pixelLuminance = max(GetLuma(color), 0.0001);
    float toneMappedLuminance = log10(1.0 + pixelLuminance) / log10(1.0 + ToneMapWhiteLevel);
    return ToneMapApplyLuminance(color, pixelLuminance, toneMappedLuminance);
}

// Drago's logarithmic mapping operator
float3 ToneMapDragoLogarithmic(float3 color, float bias) {
    float pixelLuminance = max(GetLuma(color), 0.0001);
    float toneMappedLuminance = log10(1.0 + pixelLuminance);
    toneMappedLuminance /= log10(1.0 + ToneMapWhiteLevel);
    toneMappedLuminance /= log10(2.0 + 8.0 * ((pixelLuminance / ToneMapWhiteLevel) * log10(bias) / log10(0.5)));
    return ToneMapApplyLuminance(color, pixelLuminance, toneMappedLuminance);
}

// Reinhard's basic tone mapping operator
float3 ToneMapReinhard(float3 color) {
    float pixelLuminance = max(GetLuma(color), 0.0001);
    float toneMappedLuminance = pixelLuminance / (pixelLuminance + 1.0);
    return ToneMapApplyLuminance(color, pixelLuminance, toneMappedLuminance);
}

// Reinhard's extended tone mapping operator
float3 ToneMapReinhardExtended(float3 color) {
    float pixelLuminance = max(GetLuma(color), 0.0001);
    float toneMappedLuminance = pixelLuminance * (1.0 + pixelLuminance / (ToneMapWhiteLevel * ToneMapWhiteLevel)) / (1.0 + pixelLuminance);
    return ToneMapApplyLuminance(color, pixelLuminance, toneMappedLuminance);
}

// Applies the filmic curve from John Hable's presentation
float3 ToneMapFilmicALU(float3 color) {
    color = max(float3(0.0, 0.0, 0.0), color - float3(0.004, 0.004, 0.004));
    color = (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);

    // result has 1/2.2 baked in
    return pow(color, float3(2.2, 2.2, 2.2));
}

// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float3 ToneMapFilmicACES(float3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;

    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

// https://knarkowicz.wordpress.com/2016/08/31/hdr-display-first-steps/
float3 ToneMapFilmicACESRec2020(float3 x) {
    float a = 15.8;
    float b = 2.12;
    float c = 1.2;
    float d = 5.92;
    float e = 1.9;

    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

// Unreal 3, Documentation: "Color Grading"
// Adapted to be close to ToneMapFilmicACES, with similar range
float3 ToneMapFilmicUnreal3(float3 x) {
    return pow(x / (x + 0.155) * 1.019, float3(2.2, 2.2, 2.2));
}

// Function used by the Uncharted2 tone mapping curve
float3 U2Func(float3 x) {
    const float a = 0.22; // Shoulder Strength
    const float b = 0.30; // Linear Strength
    const float c = 0.10; // Linear Angle
    const float d = 0.20; // Toe Strength
    const float e = 0.01; // Toe Numerator
    const float f = 0.30; // Toe Denominator

    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}

// Uncharted 2 filmic tone mapping operator
float3 ToneMapFilmicUncharted2(float3 color) {
    float linearWhite = 3.3466;
    float exposureBias = 2.0;
    float3 numerator = U2Func(exposureBias * color);
    float3 denominator = U2Func(float3(linearWhite, linearWhite, linearWhite));

    return numerator / denominator;
}

// Uchimura 2017, "HDR theory and practice"
// Math: https://www.desmos.com/calculator/gslcdxvipg
// Source: https://www.slideshare.net/nikuque/hdr-theory-and-practicce-jp
float3 ToneMapFilmicUchimura(float3 x, float P, float a, float m, float l, float c, float b) {
    float l0 = ((P - m) * l) / a;
    float L0 = m - m / a;
    float L1 = m + (1.0 - m) / a;
    float S1 = m + a * l0;
    float S0 = m + l0;
    float C2 = (a * P) / (P - S1);
    float CP = -C2 / P;

    float3 w0 = 1.0 - smoothstep(float3(0.0, 0.0, 0.0), float3(m, m, m), x);
    float3 w2 = step(m + l0, x);
    float3 w1 = 1.0 - w0 - w2;

    float3 T = m * pow(x / m, float3(c, c, c)) + b;
    float3 S = P - (P - S1) * exp(CP * (x - S0));
    float3 L = m + a * (x - m);

    return T * w0 + L * w1 + S * w2;
}

float3 ToneMapFilmicGT(float3 x, float maxDisplayLinearValue) {
    const float P = max(maxDisplayLinearValue, 1.0); // max display brightness in paper-white-relative linear units
    const float a = 1.0;    // contrast
    const float m = 0.22;   // linear section start
    const float l = 0.4;    // linear section length
    const float c = 1.33;   // black
    const float b = 0.0;    // pedestal

    return ToneMapFilmicUchimura(x, P, a, m, l, c, b);
}

float3 ToneMapPBRNeutral(float3 color) {
    const float startCompression = 0.8 - 0.04;
    const float desaturation = 0.15;

    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;

    float peak = max(color.r, max(color.g, color.b));
    if (peak < startCompression) {
        return color;
    }

    float d = 1.0 - startCompression;
    float newPeak = 1.0 - d * d / (peak + d - startCompression);
    color *= newPeak / peak;

    float g = 1.0 - 1.0 / (desaturation * (peak - newPeak) + 1.0);
    return lerp(color, float3(newPeak, newPeak, newPeak), g);
}

float3 ToneMap(float3 color, uint toneMappingOperator, float maxDisplayLinearValue) {
    switch (toneMappingOperator) {
    case ToneMappingOperatorLinear:
        return ToneMapLinear(color);
    case ToneMappingOperatorExponential:
        return ToneMapExponential(color);
    case ToneMappingOperatorLogarithmic:
        return ToneMapLogarithmic(color);
    case ToneMappingOperatorDragoLogarithmic:
        return ToneMapDragoLogarithmic(color, 0.85);
    case ToneMappingOperatorReinhard:
        return ToneMapReinhard(color);
    case ToneMappingOperatorReinhardExtended:
        return ToneMapReinhardExtended(color);
    case ToneMappingOperatorFilmicALU:
        return ToneMapFilmicALU(color);
    case ToneMappingOperatorFilmicUnreal3:
        return ToneMapFilmicUnreal3(color);
    case ToneMappingOperatorFilmicUncharted2:
        return ToneMapFilmicUncharted2(color);
    case ToneMappingOperatorFilmicGT:
        return ToneMapFilmicGT(color, maxDisplayLinearValue);
    case ToneMappingOperatorPBRNeutral:
        return ToneMapPBRNeutral(color);
    case ToneMappingOperatorAgX:
        return ToneMapAgX(color, AgXLookNone);
    case ToneMappingOperatorAgXPunch:
        return ToneMapAgX(color, AgXLookPunchy);
    case ToneMappingOperatorFilmicACES:
    default:
        return ToneMapFilmicACES(color);
    }
}

#endif // __TONE_MAP_OPERATORS_HLSLI__
