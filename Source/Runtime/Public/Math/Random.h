// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

/*
-------------------------------------------------------------------------------

    RNG (Random Number Generator)

    - lightweight deterministic random generator (xorshift64*)
    - https://en.wikipedia.org/wiki/Xorshift

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class BE_API RNG {
public:
    RNG(uint64_t seed) : state(seed) {}

                            // Seeds the random number generator, seed should be non-zero number
    void                    SetSeed(uint64_t seed) { state = seed; }

                            // Gives an uint in range [0, UINT64_MAX]
    constexpr uint64_t      GetNextUInt();

                            // Gives an uint64 in range [min, max]
    uint64_t                GetNextUInt(uint64_t min, uint64_t max);

                            // Gives an uint32 in range [min, max]
    uint32_t                GetNextUInt(uint32_t min, uint32_t max);

                            // Gives an int64 in range [-INT64_MAX, INT64_MAX]
    constexpr int64_t       GetNextInt();

                            // Gives an int64 in range [min, max]
    int64_t                 GetNextInt(int64_t min, int64_t max);

                            // Gives an int32 in range [min, max]
    int32_t                 GetNextInt(int32_t min, int32_t max);

                            // Gives a float in range [0, 1)
    constexpr float         GetNextFloat();

                            // Gives a float in range [min, max]
    constexpr float         GetNextFloat(float min, float max) { return min + (max - min) * GetNextFloat(); }

    static int              GetRandom(int minValue, int maxValue);
    static int              GetRandom(int maxValue);

    static uint32_t         GetRandom(uint32_t minValue, uint32_t maxValue);
    static uint32_t         GetRandom(uint32_t maxValue);

    static uint64_t         GetRandom(uint64_t minValue, uint64_t maxValue);
    static uint64_t         GetRandom(uint64_t maxValue);

    static float            GetRandom(float minValue, float maxValue);
    static float            GetRandom(float maxValue);

private:
    static uint64_t         MapUInt64(uint64_t value, uint64_t range);
    static constexpr int64_t UInt64ToInt64(uint64_t value);
    static constexpr int32_t UInt32ToInt32(uint32_t value);

    uint64_t                state = 0;
};

BE_INLINE constexpr uint64_t RNG::GetNextUInt() {
    state ^= state >> 12ull;
    state ^= state << 25ull;
    state ^= state >> 27ull;
    return state * 0x2545F4914F6CDD1DULL;
}

BE_INLINE uint64_t RNG::MapUInt64(uint64_t value, uint64_t range) {
    if (range == 0) {
        return value;
    }

#if defined(_MSC_VER) && defined(_M_X64)
    uint64_t high;
    ::_umul128(value, range, &high);
    return high;
#elif defined(__SIZEOF_INT128__)
    return uint64_t((static_cast<__uint128_t>(value) * static_cast<__uint128_t>(range)) >> 64);
#else
    return value % range;
#endif
}

BE_INLINE constexpr int64_t RNG::UInt64ToInt64(uint64_t value) {
    return value <= uint64_t(std::numeric_limits<int64_t>::max()) ? int64_t(value) : -int64_t(~value) - int64_t(1);
}

BE_INLINE constexpr int32_t RNG::UInt32ToInt32(uint32_t value) {
    return value <= uint32_t(std::numeric_limits<int32_t>::max()) ? int32_t(value) : -int32_t(~value) - int32_t(1);
}

BE_INLINE uint64_t RNG::GetNextUInt(uint64_t min, uint64_t max) {
    if (min == max) {
        return min;
    }

    if (min > max) {
        uint64_t temp = min;
        min = max;
        max = temp;
    }

    const uint64_t range = max - min + uint64_t(1);
    return min + MapUInt64(GetNextUInt(), range);
}

BE_INLINE uint32_t RNG::GetNextUInt(uint32_t min, uint32_t max) {
    if (min == max) {
        return min;
    }

    if (min > max) {
        uint32_t temp = min;
        min = max;
        max = temp;
    }

    const uint64_t range = uint64_t(max) - uint64_t(min) + uint64_t(1);
    return min + uint32_t(MapUInt64(GetNextUInt(), range));
}

BE_INLINE constexpr int64_t RNG::GetNextInt() {
    return UInt64ToInt64(GetNextUInt());
}

BE_INLINE int64_t RNG::GetNextInt(int64_t min, int64_t max) {
    if (min == max) {
        return min;
    }

    if (min > max) {
        int64_t temp = min;
        min = max;
        max = temp;
    }

    const uint64_t range = uint64_t(max) - uint64_t(min) + uint64_t(1);
    if (range == 0) {
        return GetNextInt();
    }

    return UInt64ToInt64(uint64_t(min) + MapUInt64(GetNextUInt(), range));
}

BE_INLINE int32_t RNG::GetNextInt(int32_t min, int32_t max) {
    if (min == max) {
        return min;
    }

    if (min > max) {
        int32_t temp = min;
        min = max;
        max = temp;
    }

    const uint64_t range = uint64_t(int64_t(max) - int64_t(min)) + uint64_t(1);
    return UInt32ToInt32(uint32_t(min) + uint32_t(MapUInt64(GetNextUInt(), range)));
}

BE_INLINE constexpr float RNG::GetNextFloat() {
    return float(GetNextUInt() >> 40) * (1.0f / 16777216.0f);
}

BE_NAMESPACE_END
