#pragma once

#include <cstdint>

// SplitMix64 mixing gives each render stream a deterministic, non-zero RNG seed.
constexpr uint64_t MakeRenderSeed(uint64_t seed, uint64_t stream) {
    seed += 0x9e3779b97f4a7c15ULL * (stream + 1);
    seed = (seed ^ (seed >> 30)) * 0xbf58476d1ce4e5b9ULL;
    seed = (seed ^ (seed >> 27)) * 0x94d049bb133111ebULL;
    seed ^= seed >> 31;
    return seed != 0 ? seed : 1;
}
