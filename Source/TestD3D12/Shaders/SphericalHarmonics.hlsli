#ifndef __SPHERICAL_HARMONICS_HLSLI__
#define __SPHERICAL_HARMONICS_HLSLI__

static const uint SH3CoefficientCount = 9u;

// Evaluates the real SH basis through band 2 using the legacy engine-axis convention.
// The x/y sign change preserves the coefficient layout used by BlueshiftRuntime.
void EvaluateSH3Basis(float3 direction, out float coefficients[SH3CoefficientCount]) {
    const float SHConst0 = 0.282095f;
    const float SHConst1 = 0.488603f;
    const float SHConst2 = 1.092548f;
    const float SHConst3 = 0.315392f;
    const float SHConst4 = 0.546274f;

    const float x = -direction.x;
    const float y = -direction.y;
    const float z = direction.z;

    coefficients[0] = SHConst0;
    coefficients[1] = SHConst1 * y;
    coefficients[2] = SHConst1 * z;
    coefficients[3] = SHConst1 * x;
    coefficients[4] = SHConst2 * x * y;
    coefficients[5] = SHConst2 * y * z;
    coefficients[6] = SHConst3 * (3.0f * z * z - 1.0f);
    coefficients[7] = SHConst2 * x * z;
    coefficients[8] = SHConst4 * (x * x - y * y);
}

// Returns sqrt(4 PI / (2 l + 1)) times the projected clamped-cosine / PI ZH coefficient.
float LambertSHConvolutionFactor(uint coefficientIndex) {
    if (coefficientIndex == 0u) {
        return 1.0f;
    }
    if (coefficientIndex < 4u) {
        return 2.0f / 3.0f;
    }
    return 1.0f / 4.0f;
}

#endif // __SPHERICAL_HARMONICS_HLSLI__
