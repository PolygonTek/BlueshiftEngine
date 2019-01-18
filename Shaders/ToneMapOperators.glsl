$include "Colors.glsl"

#define TONE_MAPPING_LINEAR             0
#define TONE_MAPPING_EXPONENTIAL        1
#define TONE_MAPPING_LOGARITHMIC        2
#define TONE_MAPPING_DRAGO_LOGARITHMIC  3
#define TONE_MAPPING_REINHARD           4
#define TONE_MAPPING_REINHARD_EX        5
#define TONE_MAPPING_FILMIC_ALU         6
#define TONE_MAPPING_FILMIC_ACES        7
#define TONE_MAPPING_FILMIC_UNREAL3     8
#define TONE_MAPPING_FILMIC_UNCHARTED2  9
#define TONE_MAPPING_FILMIC_GT          10

const float whiteLevel = 5.0;

const float luminanceSaturation = 1.0;

// Linear tone mapping operator
vec3 ToneMapLinear(vec3 color) {
    return color;
}

// Exponential tone mapping operator
vec3 ToneMapExponential(vec3 color) {
    float pixelLuminance = max(GetLuma(color), 0.0001);
    float toneMappedLuminance = 1.0 - exp(-pixelLuminance / whiteLevel);
    return toneMappedLuminance * pow(color / pixelLuminance, vec3(luminanceSaturation));
}

// Logarithmic mapping operator
vec3 ToneMapLogarithmic(vec3 color) {
    float pixelLuminance = max(GetLuma(color), 0.0001);
    float toneMappedLuminance = log10(1.0 + pixelLuminance) / log10(1.0 + whiteLevel);
    return toneMappedLuminance * pow(color / pixelLuminance, vec3(luminanceSaturation));
}

// Drago's Logarithmic mapping operator
vec3 ToneMapDragoLogarithmic(vec3 color, float bias) {
    float pixelLuminance = max(GetLuma(color), 0.0001);
    float toneMappedLuminance = log10(1 + pixelLuminance);
    toneMappedLuminance /= log10(1.0 + whiteLevel);
    toneMappedLuminance /= log10(2.0 + 8.0 * ((pixelLuminance / whiteLevel) * log10(bias) / log10(0.5)));
    return toneMappedLuminance * pow(color / pixelLuminance, vec3(luminanceSaturation));
}

// Reinhard's basic tone mapping operator
vec3 ToneMapReinhard(vec3 color) {
    float pixelLuminance = max(GetLuma(color), 0.0001);
    float toneMappedLuminance = pixelLuminance / (pixelLuminance + 1.0);
    return toneMappedLuminance * pow(color / pixelLuminance, vec3(luminanceSaturation));
}

// Reinhard's extended tone mapping operator
vec3 ToneMapReinhardExtended(vec3 color) {
    float pixelLuminance = max(GetLuma(color), 0.0001);
    float toneMappedLuminance = pixelLuminance * (1.0 + pixelLuminance / (whiteLevel * whiteLevel)) / (1.0 + pixelLuminance);
    return toneMappedLuminance * pow(color / pixelLuminance, vec3(luminanceSaturation));
}

// Applies the filmic curve from John Hable's presentation
vec3 ToneMapFilmicALU(vec3 color) {
    color = max(vec3(0.0), color - vec3(0.004));
    color = (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);

    // result has 1/2.2 baked in
    return pow(color, vec3(2.2));
}

// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ToneMapFilmicACES(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;

    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

// Unreal 3, Documentation: "Color Grading"
// Adapted to be close to ToneMapFilmicACES, with similar range
vec3 ToneMapFilmicUnreal3(vec3 x) {
    return pow(x / (x + 0.155) * 1.019, vec3(2.2));
}

// Function used by the Uncharted2 tone mapping curve
vec3 U2Func(vec3 x) {
    const float a = 0.22; // Shoulder Strength
    const float b = 0.30; // Linear Strength
    const float c = 0.10; // Linear Angle
    const float d = 0.20; // Toe Strength
    const float e = 0.01; // Toe Numerator
    const float f = 0.30; // Toe Denominator

    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}

// Uncharted 2 filmic tone mapping operator
vec3 ToneMapFilmicUncharted2(vec3 color) {
    float linearWhite = 3.3466;
    float exposureBias = 2.0;
    vec3 numerator = U2Func(exposureBias * color);
    vec3 denominator = U2Func(vec3(linearWhite));

    return numerator / denominator;
}

// Uchimura 2017, "HDR theory and practice"
// Math: https://www.desmos.com/calculator/gslcdxvipg
// Source: https://www.slideshare.net/nikuque/hdr-theory-and-practicce-jp
vec3 ToneMapFilmicUchimura(vec3 x, float P, float a, float m, float l, float c, float b) {
    float l0 = ((P - m) * l) / a;
    float L0 = m - m / a;
    float L1 = m + (1.0 - m) / a;
    float S0 = m + l0;
    float S1 = m + a * l0;
    float C2 = (a * P) / (P - S1);
    float CP = -C2 / P;

    vec3 w0 = 1.0 - smoothstep(vec3(0.0), vec3(m), x);
    vec3 w2 = step(m + l0, x);
    vec3 w1 = 1.0 - w0 - w2;

    vec3 T = m * pow(x / m, vec3(c)) + b;
    vec3 S = P - (P - S1) * exp(CP * (x - S0));
    vec3 L = m + a * (x - m);

    return T * w0 + L * w1 + S * w2;
}

vec3 ToneMapFilmicGT(vec3 x) {
    const float P = 1.0;  // max display brightness
    const float a = 1.0;  // contrast
    const float m = 0.22; // linear section start
    const float l = 0.4;  // linear section length
    const float c = 1.33; // black
    const float b = 0.0;  // pedestal

    return ToneMapFilmicUchimura(x, P, a, m, l, c, b);
}
