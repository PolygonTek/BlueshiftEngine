#ifndef __RANDOM_HLSLI__
#define __RANDOM_HLSLI__

#include "Common.hlsli"

// 32-bit integer hash.
uint Hash(uint x) {
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

// Maps a 2D coordinate and seed to an independent-ish pair of 32-bit hashes.
uint2 Hash(uint2 p, uint seed) {
    const uint h0 = Hash(p.x ^ Hash(p.y ^ seed));
    const uint h1 = Hash(p.y ^ Hash(p.x ^ (seed + 0x9e3779b9u)));
    return uint2(h0, h1);
}

// Maps the upper 23 bits of a uint to a uniform random number strictly within (0, 1).
// The half-step offset keeps both endpoints excluded at float precision.
float UintToUniform01(uint x) {
    return (float(x >> 9) + 0.5f) * (1.0f / 8388608.0f);
}

// Maps a grid coordinate and seed to two uniform random numbers in (0, 1).
float2 UniformRandom(uint2 coord, uint seed) {
    const uint2 h = Hash(coord, seed);
    return float2(UintToUniform01(h.x), UintToUniform01(h.y));
}

// Maps two independent U(0, 1) values to two independent N(0, 1) values.
float2 BoxMuller(float2 u) {
    const float r = sqrt(-2.0f * log(u.x));
    const float theta = TWO_PI * u.y;

    float s;
    float c;
    sincos(theta, s, c);
    return r * float2(c, s);
}

// Maps a grid coordinate and seed to a Gaussian pair with distribution N(0, 1).
float2 GaussianRandom(uint2 coord, uint seed) {
    return BoxMuller(UniformRandom(coord, seed));
}

#endif // __RANDOM_HLSLI__
