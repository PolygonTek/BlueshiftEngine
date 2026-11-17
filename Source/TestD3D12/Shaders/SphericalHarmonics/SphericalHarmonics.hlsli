#ifndef __SPHERICAL_HARMONICS_HLSLI__
#define __SPHERICAL_HARMONICS_HLSLI__

#ifndef SH_BAND_COUNT
#error SH_BAND_COUNT must be defined.
#endif

#if SH_BAND_COUNT != 2 && SH_BAND_COUNT != 3
#error SH_BAND_COUNT must be 2 or 3.
#endif

static const uint SHCoefficientCount = SH_BAND_COUNT * SH_BAND_COUNT;

// Evaluates the real SH basis through the selected band count using the legacy engine-axis convention.
// The x/y sign change preserves the coefficient layout used by BlueshiftRuntime.
void EvaluateSHBasis(float3 direction, out float basisValues[SHCoefficientCount]) {
    const float SHConst0 = 0.282095f;
    const float SHConst1 = 0.488603f;

    const float x = -direction.x;
    const float y = -direction.y;
    const float z = direction.z;

    basisValues[0] = SHConst0;
    basisValues[1] = SHConst1 * y;
    basisValues[2] = SHConst1 * z;
    basisValues[3] = SHConst1 * x;

#if SH_BAND_COUNT >= 3
    const float SHConst2 = 1.092548f;
    const float SHConst3 = 0.315392f;
    const float SHConst4 = 0.546274f;

    basisValues[4] = SHConst2 * x * y;
    basisValues[5] = SHConst2 * y * z;
    basisValues[6] = SHConst3 * (3.0f * z * z - 1.0f);
    basisValues[7] = SHConst2 * x * z;
    basisValues[8] = SHConst4 * (x * x - y * y);
#endif
}

#endif // __SPHERICAL_HARMONICS_HLSLI__
