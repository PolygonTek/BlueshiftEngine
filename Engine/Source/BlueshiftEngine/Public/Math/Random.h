#pragma once

/*
-------------------------------------------------------------------------------

    Random number generator

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN
    
class BE_API Random {
public:
    Random(int seed = 0);

    void                SetSeed(int seed);
    int                 GetSeed() const;

                        /// random integer in the range [0, MaxRand]
    int                 RandomInt();
                        /// random integer in the range [0, max]
    int                 RandomInt(int max);
                        /// random number in the range [0.0f, 1.0f]
    float               RandomFloat();
                        /// random number in the range [-1.0f, 1.0f]
    float               CRandomFloat();

    static const int    MaxRand = 0x7fff;

private:
    int                 seed;
};

BE_INLINE Random::Random(int seed) {
    this->seed = seed;
}

BE_INLINE void Random::SetSeed(int seed) {
    this->seed = seed;
}

BE_INLINE int Random::GetSeed() const {
    return seed;
}

BE_INLINE int Random::RandomInt() {
    seed = 69069 * seed + 1;
    return (seed & Random::MaxRand);
}

BE_INLINE int Random::RandomInt(int max) {
    if (max == 0) {
        return 0;   // avoid divide by zero error
    }
    return RandomInt() % max;
}

BE_INLINE float Random::RandomFloat() {
    return (RandomInt() / (float)(Random::MaxRand + 1));
}

BE_INLINE float Random::CRandomFloat() {
    return (2.0f * (RandomFloat() - 0.5f));
}

BE_NAMESPACE_END
