#include "Precompiled.h"
#include "Render/Sampler.h"
#include "Render/Options.h"

BE1::Vec3 RotateWithUpVector(const BE1::Vec3 &dir, const BE1::Vec3 &up) {
    BE1::Vec3 axis[2];
    if (BE1::Math::Abs(up.x) < 0.5f) {
        axis[1] = up.Cross(BE1::Vec3(1.0f, 0.0f, 0.0f));
    } else {
        axis[1] = BE1::Vec3(0.0f, 0.0f, 1.0f).Cross(up);
    }
    axis[1].Normalize();
    axis[0] = axis[1].Cross(up);
    axis[0].Normalize();
    return axis[0] * dir.x + axis[1] * dir.y + up * dir.z;
}

Sampler::~Sampler() {
    Clear();
}

void Sampler::Clear() {
    delete[] samples;
    samples = nullptr;
    numSamples = capacity = jump = 0;
    seed = 0;
}

void Sampler::Init(int numSamples, uint64_t seed) {
    assert(numSamples > 0 && numSamples <= Options::MaxSampleCount);
    assert(seed != 0);

    jump = 0;
    if (this->numSamples == numSamples && this->seed == seed) {
        return;
    }

    const int requiredCapacity = numSamples * NumSets;
    if (capacity < requiredCapacity) {
        delete[] samples;
        samples = new BE1::Vec2[requiredCapacity];
        capacity = requiredCapacity;
    }
    this->numSamples = numSamples;
    this->seed = seed;
    BE1::RNG rng(seed);

#if 1
    // Reuse the Hammersley-Halton radical inverse sequence without consuming random numbers.
    static const BE1::Hammersley<Options::MaxSampleCount> baseSamples(2, 7, BE1::Vec2::zero);

    for (int s = 0; s < NumSets; s++) {
        float rx = rng.GetNextFloat();
        float ry = rng.GetNextFloat();
        for (int i = 0; i < numSamples; i++) {
            int offset = numSamples * s + i;
            const BE1::Vec2 &baseSample = baseSamples.GetSample(i);
            samples[offset].x = baseSample.x + rx;
            samples[offset].x -= floorf(samples[offset].x);
            samples[offset].y = baseSample.y + ry;
            samples[offset].y -= floorf(samples[offset].y);
        }
    }
#else
    int n = (int)BE1::Math::Sqrt((float)numSamples);
    float subcell_size = 1.0f / numSamples;

    // Distribute points in the initial patterns
    for (int s = 0; s < NumSets; s++) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                int offset = numSamples * s + n * i + j;
                samples[offset].x = (n * j + i) * subcell_size + rng.GetNextFloat(0.0f, subcell_size);
                samples[offset].y = (n * i + j) * subcell_size + rng.GetNextFloat(0.0f, subcell_size);
            }
        }
    }

    // Shuffle y coordinates
    for (int s = 0; s < NumSets; s++) {
        BE1::Vec2 *ss = &samples[numSamples * s];
        for (int i = 0; i < numSamples; i++) {
            int k = rng.GetNextInt(0, numSamples - 1);
            Swap(ss[i].y, ss[k].y);
        }
    }

    // Shuffle x coordinates
    for (int s = 0; s < NumSets; s++) {
        BE1::Vec2 *ss = &samples[numSamples * s];
        for (int i = 0; i < numSamples - 1; i++) {
            int k = rng.GetNextInt(0, numSamples - 1);
            Swap(ss[i + 1].x, ss[k].x);
        }
    }
/*
    // Shuffle coordinates
    for (int s = 0; s < NumSets; s++) {
        BE1::Vec2 *ss = &samples[numSamples * s];
        for (int i = 0; i < numSamples; i++) {
            int j = rng.GetNextInt(0, numSamples - 1);
            Swap(ss[i], ss[j]);
        }
    }*/
#endif
}

BE1::Vec2 Sampler::GetSample(int index, BE1::RNG &rng) {
#if 0
    BE1::Vec2 sample;
    sample.x = rng.GetNextFloat();
    sample.y = rng.GetNextFloat();
    return sample;
#else
    if (index % numSamples == 0) {
        jump = rng.GetNextInt(0, NumSets - 1) * numSamples;
    }
    return samples[jump + index % numSamples];
#endif
}
