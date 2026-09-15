#pragma once

#include "BlueshiftEngine.h"

class Sampler {
public:
    Sampler() = default;
    Sampler(const Sampler &) = delete;
    Sampler &operator=(const Sampler &) = delete;
    ~Sampler();

    void                            Init(int numSamples, uint64_t seed);
    void                            Clear();
    BE1::Vec2                       GetSample(int index, BE1::RNG &rng);

private:
    static constexpr int            NumSets = 256;
    int                             numSamples = 0;
    int                             capacity = 0;
    uint64_t                        seed = 0;
    BE1::Vec2 *                     samples = nullptr;
    int                             jump = 0;
};

extern BE1::Vec3 RotateWithUpVector(const BE1::Vec3 &dir, const BE1::Vec3 &up);
