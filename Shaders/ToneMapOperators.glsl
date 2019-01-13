float log10(float x) {
    // 1.0 / log(10) = 0.4342944819
    return 0.4342944819 * log(x);
}

// Exponential tone mapping operator
vec3 ToneMapExponential(vec3 color, float whiteLevel, float luminanceSaturation) {
    float pixelLuminance = max(dot(color, lumVector), 0.0001);
    float toneMappedLuminance = 1.0 - exp(-pixelLuminance / whiteLevel);
    return toneMappedLuminance * pow(color / pixelLuminance, vec3(luminanceSaturation));
}

// Logarithmic mapping operator
vec3 ToneMapLogarithmic(vec3 color, float whiteLevel, float luminanceSaturation) {
    float pixelLuminance = max(dot(color, lumVector), 0.0001);
    float toneMappedLuminance = log10(1.0 + pixelLuminance) / log10(1.0 + whiteLevel);
    return toneMappedLuminance * pow(color / pixelLuminance, vec3(luminanceSaturation));
}

// Drago's Logarithmic mapping operator
vec3 ToneMapDragoLogarithmic(vec3 color, float whiteLevel, float luminanceSaturation, float bias) {
    float pixelLuminance = max(dot(color, lumVector), 0.0001);
    float toneMappedLuminance = log10(1 + pixelLuminance);
    toneMappedLuminance /= log10(1.0 + whiteLevel);
    toneMappedLuminance /= log10(2.0 + 8.0 * ((pixelLuminance / whiteLevel) * log10(bias) / log10(0.5)));
    return toneMappedLuminance * pow(color / pixelLuminance, vec3(luminanceSaturation));
}

// Reinhard's basic tone mapping operator
vec3 ToneMapReinhard(vec3 color, float luminanceSaturation) {
    float pixelLuminance = max(dot(color, lumVector), 0.0001);
    float toneMappedLuminance = pixelLuminance / (pixelLuminance + 1.0);
    return toneMappedLuminance * pow(color / pixelLuminance, vec3(luminanceSaturation));
}

// Reinhard's extended tone mapping operator
vec3 ToneMapReinhardExtended(vec3 color, float whiteLevel, float luminanceSaturation) {
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

// Function used by the Uncharted2 tone mapping curve
vec3 U2Func(vec3 x) {
    float A = 0.15; // ShoulderStrength;
    float B = 0.50; // LinearStrength;
    float C = 0.10; // LinearAngle;
    float D = 0.20; // ToeStrength;
    float E = 0.02; // ToeNumerator;
    float F = 0.30; // ToeDenominator;

    return ((x*(A*x + C * B) + D * E) / (x*(A*x + B) + D * F)) - E / F;
}

// Uncharted 2 filmic tone mapping operator
vec3 ToneMapFilmicU2(vec3 color) {
    vec3 linearWhite = vec3(11.2);
    vec3 numerator = U2Func(2.0 * color);
    vec3 denominator = U2Func(linearWhite);

    return numerator / denominator;
}
