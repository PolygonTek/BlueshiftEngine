#define TONE_MAPPING_LINEAR             0
#define TONE_MAPPING_EXPONENTIAL        1
#define TONE_MAPPING_LOGARITHMIC        2
#define TONE_MAPPING_DRAGO_LOGARITHMIC  3
#define TONE_MAPPING_REINHARD           4
#define TONE_MAPPING_REINHARD_EX        5
#define TONE_MAPPING_FILMIC_ALU         6
#define TONE_MAPPING_FILMIC_ACES        7
#define TONE_MAPPING_FILMIC_UNREAL      8
#define TONE_MAPPING_FILMIC_UNCHARTED2  9

const vec3 lumVector = vec3(0.2125, 0.7154, 0.0721);

const float whiteLevel = 5.0;

const float luminanceSaturation = 1.0;

float log10(float x) {
    // 1.0 / log(10) = 0.4342944819
    return 0.4342944819 * log(x);
}

// Linear tone mapping operator
vec3 ToneMapLinear(vec3 color) {
    return color;
}

// Exponential tone mapping operator
vec3 ToneMapExponential(vec3 color) {
    float pixelLuminance = max(dot(color, lumVector), 0.0001);
    float toneMappedLuminance = 1.0 - exp(-pixelLuminance / whiteLevel);
    return toneMappedLuminance * pow(color / pixelLuminance, vec3(luminanceSaturation));
}

// Logarithmic mapping operator
vec3 ToneMapLogarithmic(vec3 color) {
    float pixelLuminance = max(dot(color, lumVector), 0.0001);
    float toneMappedLuminance = log10(1.0 + pixelLuminance) / log10(1.0 + whiteLevel);
    return toneMappedLuminance * pow(color / pixelLuminance, vec3(luminanceSaturation));
}

// Drago's Logarithmic mapping operator
vec3 ToneMapDragoLogarithmic(vec3 color, float bias) {
    float pixelLuminance = max(dot(color, lumVector), 0.0001);
    float toneMappedLuminance = log10(1 + pixelLuminance);
    toneMappedLuminance /= log10(1.0 + whiteLevel);
    toneMappedLuminance /= log10(2.0 + 8.0 * ((pixelLuminance / whiteLevel) * log10(bias) / log10(0.5)));
    return toneMappedLuminance * pow(color / pixelLuminance, vec3(luminanceSaturation));
}

// Reinhard's basic tone mapping operator
vec3 ToneMapReinhard(vec3 color) {
    float pixelLuminance = max(dot(color, lumVector), 0.0001);
    float toneMappedLuminance = pixelLuminance / (pixelLuminance + 1.0);
    return toneMappedLuminance * pow(color / pixelLuminance, vec3(luminanceSaturation));
}

// Reinhard's extended tone mapping operator
vec3 ToneMapReinhardExtended(vec3 color) {
    float pixelLuminance = max(dot(color, lumVector), 0.0001);
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

// Unreal, Documentation: "Color Grading"
// Adapted to be close to ToneMapFilmicACES, with similar range
vec3 ToneMapFilmicUnreal(vec3 x) {
    return pow(x / (x + 0.155) * 1.019, vec3(2.2));
}

// Function used by the Uncharted2 tone mapping curve
vec3 U2Func(vec3 x) {
    const float A = 0.22; // Shoulder Strength
    const float B = 0.30; // Linear Strength
    const float C = 0.10; // Linear Angle
    const float D = 0.20; // Toe Strength
    const float E = 0.01; // Toe Numerator
    const float F = 0.30; // Toe Denominator

    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

// Uncharted 2 filmic tone mapping operator
vec3 ToneMapFilmicUncharted2(vec3 color) {
    float linearWhite = 3.3466;
    float exposureBias = 2.0;
    vec3 numerator = U2Func(exposureBias * color);
    vec3 denominator = U2Func(vec3(linearWhite));

    return numerator / denominator;
}
