// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
