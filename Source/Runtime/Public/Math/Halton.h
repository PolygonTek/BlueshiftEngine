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

BE_NAMESPACE_BEGIN

/*
-------------------------------------------------------------------------------

    Halton Sequence

-------------------------------------------------------------------------------
*/

template <int N>
class Halton {
public:
    Halton(int baseX = 2, int baseY = 7);
    Halton(int baseX, int baseY, const Vec2 &sampleOffset);

                    /// Casts this to a C array.
                    /// This function simply returns a C pointer view to this data structure.
    const float *   Ptr() const { return (const float *)&samples[0].x; }
    float *         Ptr() { return (float *)&samples[0].x; }

    int             NumSamples() const { return N; }

    const Vec2 &    GetSample(int index) const;

private:
    void            GenerateSamples(int baseX, int baseY, const Vec2 &sampleOffset);

                    /// Radical inverse function for the Halton sequence.
    float           RadicalInverse(int index, int base) const;

    Vec2            samples[N];
    int             baseX;
    int             baseY;
};

template <int N>
BE_INLINE Halton<N>::Halton(int baseX, int baseY) : baseX(baseX), baseY(baseY) {
    GenerateSamples(baseX, baseY, Vec2(RNG::GetRandom(1.0f), RNG::GetRandom(1.0f)));
}

template <int N>
BE_INLINE Halton<N>::Halton(int baseX, int baseY, const Vec2 &sampleOffset) : baseX(baseX), baseY(baseY) {
    GenerateSamples(baseX, baseY, sampleOffset);
}

template <int N>
BE_INLINE void Halton<N>::GenerateSamples(int baseX, int baseY, const Vec2 &sampleOffset) {
    for (int sampleIndex = 0; sampleIndex < N; sampleIndex++) {
        samples[sampleIndex].x = Math::Fract(RadicalInverse(sampleIndex, baseX) + sampleOffset.x);
        samples[sampleIndex].y = Math::Fract(RadicalInverse(sampleIndex, baseY) + sampleOffset.y);
    }
}

template <int N>
BE_INLINE float Halton<N>::RadicalInverse(int index, int base) const {
    float radix = (float)base;
    float x = 0.0f;

    while (index) {
        int a = index % base;
        x += a / radix;
        index = (int)((float)index / (float)base);
        radix *= base;
    }

    return x;
}

template <int N>
BE_INLINE const Vec2 &Halton<N>::GetSample(int index) const {
    return samples[index % N];
}

BE_NAMESPACE_END
