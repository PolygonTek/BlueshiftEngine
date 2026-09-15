#pragma once

#include <cstdint>

struct Options {
    static constexpr int            MaxSampleCount = 4096;

    uint64_t                        randomSeed = 1;
    int                             maxSamplePasses = 32;
    int                             splitSampleCount = 32;
    int                             shadowSampleCount = 32;
    bool                            directLighting = true;
    bool                            indirectLighting = true;
    bool                            importanceSampling = true;
    bool                            ambientOcclusion = false;
    bool                            useDOF = false;

    bool                            IsValid() const;
};

BE_INLINE bool Options::IsValid() const {
    const auto validCount = [](int count, int maximum) {
        return count > 0 && count <= maximum && (count & (count - 1)) == 0;
    };
    return validCount(maxSamplePasses, MaxSampleCount) && validCount(splitSampleCount, MaxSampleCount) && validCount(shadowSampleCount, 256);
}
